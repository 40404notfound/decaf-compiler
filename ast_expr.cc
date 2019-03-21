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

void NewExpr::CheckType() {
    auto tmp=cType->CheckTypeHelper(LookingForClass);
    cType = dynamic_cast<NamedType *>(tmp);
}

void NewArrayExpr::CheckType() {
    elemType = elemType->CheckTypeHelper(LookingForType);
}

