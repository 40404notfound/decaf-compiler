/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "ast_type.h"
#include "errors.h"
#include <string.h>

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r)
    : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op = o)->SetParent(this);
    (left = l)->SetParent(this);
    (right = r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r)
    : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL;
    (op = o)->SetParent(this);
    (right = r)->SetParent(this);
}

vector<Node *> CompoundExpr::children() {
    vector<Node *> result = Expr::children();
    result.push_back(op);
    result.push_back(left);
    result.push_back(right);
    return result;
}

Location *genLessOrEqual(Location *l, Location *r) {
    Location *lessThan = CodeGenerator::instance->GenBinaryOp("<", l, r);
    Location *equal = CodeGenerator::instance->GenBinaryOp("==", l, r);
    Location *rv = CodeGenerator::instance->GenBinaryOp("||", lessThan, equal);
    return rv;
}

Location *RelationalExpr::cgen() {
    Location *l = left->cgen();
    Location *r = right->cgen();
    char *opStr = op->getToken();
    if (strcmp(opStr, "<=") == 0) {
        return genLessOrEqual(l, r);
    } else if (strcmp(opStr, ">=") == 0) {
        return genLessOrEqual(r, l);
    }
    return CodeGenerator::instance->GenBinaryOp(op->getToken(), l, r);
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base = b)->SetParent(this);
    (subscript = s)->SetParent(this);
}

vector<Node *> ArrayAccess::children() {
    vector<Node *> result = LValue::children();
    result.push_back(base);
    result.push_back(subscript);
    return result;
}

FieldAccess::FieldAccess(Expr *b, Identifier *f)
    : LValue(b ? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base)
        base->SetParent(this);
    (field = f)->SetParent(this);
}

vector<Node *> FieldAccess::children() {
    vector<Node *> result = LValue::children();
    result.push_back(base);
    result.push_back(field);
    return result;
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr *> *a) : Expr(loc) {
    Assert(f != NULL &&
           a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base)
        base->SetParent(this);
    (field = f)->SetParent(this);
    (actuals = a)->SetParentAll(this);
}

vector<Node *> Call::children() {
    vector<Node *> result = Expr::children();
    result.push_back(base);
    result.push_back(field);
    result.insert(result.end(), actuals->elems.begin(), actuals->elems.end());
    return result;
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) {
    Assert(c != NULL);
    (cType = c)->SetParent(this);
}

vector<Node *> NewExpr::children() {
    vector<Node *> result = Expr::children();
    result.push_back(cType);
    return result;
}

void NewExpr::CheckType() {
    auto tmp = cType->CheckTypeHelper(LookingForClass);
    cType = dynamic_cast<NamedType *>(tmp);
}

Location *NewExpr::cgen() {
    int typeSize = cType->GetTypeSize();
    Location *szLoc = CodeGenerator::instance->GenLoadConstant(typeSize);

    Location *rv = CodeGenerator::instance->GenBuiltInCall(Alloc, szLoc);
    Location *vtableLocation =
        CodeGenerator::instance->GenLoadLabel(cType->GetName());
    CodeGenerator::instance->GenStore(rv, vtableLocation, 0);
    return rv;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size = sz)->SetParent(this);
    (elemType = et)->SetParent(this);
}

vector<Node *> NewArrayExpr::children() {
    vector<Node *> result = Expr::children();
    result.push_back(size);
    result.push_back(elemType);
    return result;
}

void NewArrayExpr::CheckType() {
    elemType = elemType->CheckTypeHelper(LookingForType);
}

Location *NewArrayExpr::cgen() {
    // IntConstant* intConstant = dynamic_cast<IntConstant*>(size);
    // DoubleConstant* doubleConstant = dynamic_cast<DoubleConstant*>(size);
    // printf("%s\n",size->cachedType->GetName());
    // Assert(intConstant);
    Location *numElement = size->cgen();
    // check size ifz numElement < 1 goto label
    Location *one1 = CodeGenerator::instance->GenLoadConstant(1);
    Location *szLessOrEqualZero =
        CodeGenerator::instance->GenBinaryOp("<", numElement, one1);
    const char *label = CodeGenerator::instance->NewLabel();
    CodeGenerator::instance->GenIfZ(szLessOrEqualZero, label);
    CodeGenerator::instance->GenError(ArraySizeNeg);
    // correct label below
    CodeGenerator::instance->GenLabel(label);
    Location *one = CodeGenerator::instance->GenLoadConstant(1);
    Location *totalSz =
        CodeGenerator::instance->GenBinaryOp("+", one, numElement);
    Location *four = CodeGenerator::instance->GenLoadConstant(4);
    totalSz = CodeGenerator::instance->GenBinaryOp("*", totalSz, four);
    Location *alloc = CodeGenerator::instance->GenBuiltInCall(Alloc, totalSz);
    CodeGenerator::instance->GenStore(alloc, numElement, 0);
    Location *rv = CodeGenerator::instance->GenBinaryOp("+", alloc, four);
    return rv;
}

void AssignExpr::Emit() {
    // assign to class member var or store to local
    // Location *rhs = right->cgen();
    LValue *lvalue = dynamic_cast<LValue *>(left);
    lvalue->getAssign(right);
}

void FieldAccess::genBaseAndOffSet() {
    VarDecl *fieldDecl = NULL;
    if (base == NULL) {
        fieldDecl = dynamic_cast<VarDecl *>(scope->GetSymbol(field->GetName()));
        Assert(fieldDecl);
        if (fieldDecl->location) {
            // local
            baseLocation = fieldDecl->location;
            offSet = -1;
        } else {
            // class member
            baseLocation = new Location(fpRelative, 4, "this");
            offSet = fieldDecl->offset;
        }
    } else {
        Decl *baseDeclNode =
            StackNode::namedTypeTable->GetSymbol(base->cachedType->GetName());
        Assert(baseDeclNode);
        ClassDecl *baseDecl = dynamic_cast<ClassDecl *>(baseDeclNode);
        Assert(baseDecl);
        fieldDecl =
            dynamic_cast<VarDecl *>(baseDecl->GetFields(field->GetName()));
        Assert(fieldDecl);
        Assert(fieldDecl->offset);
        Assert(fieldDecl->location == NULL);
        baseLocation = base->cgen();
        offSet = fieldDecl->offset;
    }
}

Location *FieldAccess::cgen() {
    // load from baseLocation & offSet
    Assert(baseLocation == NULL && offSet == 0);
    genBaseAndOffSet();
    if (offSet == -1)
        return baseLocation;
    return CodeGenerator::instance->GenLoad(baseLocation, offSet);
}

void FieldAccess::getAssign(Expr *expr) {
    Assert(baseLocation == NULL && offSet == 0);
    Location *rhs = expr->cgen();
    genBaseAndOffSet();
    if (offSet == -1) {
        CodeGenerator::instance->GenAssign(baseLocation, rhs);
        return;
    }
    CodeGenerator::instance->GenStore(baseLocation, rhs, offSet);
}

void ArrayAccess::genFinalLocation() {
    Location *baseLocation = base->cgen();
    Location *sub = subscript->cgen();
    Location *zero = CodeGenerator::instance->GenLoadConstant(0);
    Location *lessZero = CodeGenerator::instance->GenBinaryOp("<", sub, zero);
    Location *arrayLength = CodeGenerator::instance->GenLoad(baseLocation, -4);
    Location *lessLength =
        CodeGenerator::instance->GenBinaryOp("<", sub, arrayLength);
    Location *greaterOrEqualLength = CodeGenerator::instance->GenBinaryOp(
        "==", lessLength, zero); // lessLength == 0
    Location *outOfBoundTest = CodeGenerator::instance->GenBinaryOp(
        "||", lessZero, greaterOrEqualLength);
    const char *correctLabel = CodeGenerator::instance->NewLabel();
    CodeGenerator::instance->GenIfZ(outOfBoundTest, correctLabel);
    CodeGenerator::instance->GenError(ArrayOutOfBound);
    CodeGenerator::instance->GenLabel(correctLabel);

    // get index offSet and generate finalLocation
    Location *four = CodeGenerator::instance->GenLoadConstant(4);
    Location *totalOffSet =
        CodeGenerator::instance->GenBinaryOp("*", four, sub);
    finalLocation =
        CodeGenerator::instance->GenBinaryOp("+", baseLocation, totalOffSet);
}

Location *ArrayAccess::cgen() {
    Assert(baseLocation == NULL && offSet == 0);
    genFinalLocation();
    return CodeGenerator::instance->GenLoad(finalLocation, 0);
}

void ArrayAccess::getAssign(Expr *expr) {
    Assert(baseLocation == NULL && offSet == 0);
    genFinalLocation();
    Location *rhs = expr->cgen();
    CodeGenerator::instance->GenStore(finalLocation, rhs, 0);
}

void Call::genPopParams() {
    int numParams = actuals->NumElements();
    if (ifAcall) {
        numParams += 1;
    }
    CodeGenerator::instance->GenPopParams(numParams * CodeGenerator::VarSize);
}

Location *Call::cgen() {
    if (base && dynamic_cast<ArrayType *>(base->cachedType)) {
        // length
        Assert((string) field->GetName() == "length");
        Location *baseLocation = base->cgen();
        return CodeGenerator::instance->GenLoad(baseLocation, -4);
    }

    FnDecl *fnDecl = NULL;
    Location *baseLocation = NULL;
    List<Location *> *params = new List<Location *>;
    int offSet = -1;
    // get base location of acall inlcuding implied "this" and class object as base
    if (base == NULL) {
        fnDecl = dynamic_cast<FnDecl *>(scope->GetSymbol(field->GetName()));
        Assert(fnDecl);
        if (fnDecl->offset != -1) {
            // implied this
            ifAcall = true;
            baseLocation = new Location(fpRelative, 4, "this");
            offSet = fnDecl->offset;
        }
    } else {
        ifAcall = true;
        Decl *baseDeclNode =
            StackNode::namedTypeTable->GetSymbol(base->cachedType->GetName());
        // printf("%s\n", base->cachedType->GetName());
        Assert(baseDeclNode);
        ClassDecl *baseDecl = dynamic_cast<ClassDecl *>(baseDeclNode);
        Assert(baseDecl);
        fnDecl = dynamic_cast<FnDecl *>(baseDecl->GetFields(field->GetName()));
        Assert(fnDecl);
        Assert(fnDecl->offset != -1);
        baseLocation = base->cgen();
        offSet = fnDecl->offset;
    }
    for (int i = 0; i < actuals->NumElements(); i++) {
        params->Append(actuals->Nth(i)->cgen());
    }
    Location *callLocation = NULL;
    if (ifAcall) {
        Location *vtableLocation =
            CodeGenerator::instance->GenLoad(baseLocation, 0);
        Assert(offSet != -1);
        callLocation = CodeGenerator::instance->GenLoad(vtableLocation, offSet);
    }
    Location *rv = NULL;
    int numElements = params->NumElements();
    for (int i = 0; i < numElements; i++) {
        CodeGenerator::instance->GenPushParam(params->Nth(numElements - i - 1));
    }
    if (ifAcall) {
        Assert(baseLocation);
        CodeGenerator::instance->GenPushParam(baseLocation);
        rv = CodeGenerator::instance->GenACall(callLocation,
                                               cachedType != Type::voidType);
    } else {
        Assert(fnDecl->offset == -1);
        Assert(fnDecl->label);
        rv = CodeGenerator::instance->GenLCall(fnDecl->label,
                                               cachedType != Type::voidType);
    }
    genPopParams();
    return rv;
}