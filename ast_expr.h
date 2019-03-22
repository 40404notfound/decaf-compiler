/* File: ast_expr.h
 * ----------------
 * The Expr class and its subclasses are used to represent
 * expressions in the parse tree.  For each expression in the
 * language (add, call, New, etc.) there is a corresponding
 * node class for that construct. 
 *
 * pp3: You will need to extend the Expr classes to implement 
 * semantic analysis for rules pertaining to expressions.
 */

#ifndef _H_ast_expr
#define _H_ast_expr

#include "ast.h"
#include "ast_decl.h"
#include "ast_stmt.h"
#include "ast_type.h"
#include "list.h"
#include <string>
using std::string;
class NamedType; // for new
class Type;      // for NewArray

class Expr : public Stmt {
protected:
public:
    Type *cachedType = NULL;
    FnDecl *func;

    Expr(yyltype loc) : Stmt(loc) {}
    Expr() : Stmt() {}
    virtual Location *cgen() { return NULL; }
    virtual void Emit() { cgen(); }
};

/* This node type is used for those places where an expression is optional.
 * We could use a NULL pointer, but then it adds a lot of checking for
 * NULL. By using a valid, but no-op, node, we save that trouble */
class EmptyExpr : public Expr {

public:
};

class IntConstant : public Expr {
protected:
    int value;

public:
    void Check(Context ctx) { cachedType = Type::intType; }
    IntConstant(yyltype loc, int val);
    virtual Location *cgen() {
        return CodeGenerator::instance->GenLoadConstant(value);
    }
};

class DoubleConstant : public Expr {
protected:
    double value;

public:
    void Check(Context ctx) { cachedType = Type::doubleType; }

    DoubleConstant(yyltype loc, double val);
    virtual Location *cgen() {
        return CodeGenerator::instance->GenLoadConstant(value);
    }
};

class BoolConstant : public Expr {
protected:
    bool value;

public:
    void Check(Context ctx) { cachedType = Type::boolType; }
    BoolConstant(yyltype loc, bool val);
    virtual Location *cgen() {
        return CodeGenerator::instance->GenLoadConstant(value);
    }
};

class StringConstant : public Expr {
protected:
    char *value;

public:
    void Check(Context ctx) { cachedType = Type::stringType; }
    StringConstant(yyltype loc, const char *val);
    virtual Location *cgen() {
        return CodeGenerator::instance->GenLoadConstant(value);
    }
};

class NullConstant : public Expr {
public:
    void Check(Context ctx) { cachedType = Type::nullType; }
    NullConstant(yyltype loc) : Expr(loc) {}
};

class Operator : public Node {
protected:
    char tokenString[4];

public:
    Operator(yyltype loc, const char *tok);
    friend std::ostream &operator<<(std::ostream &out, Operator *o) {
        return out << o->tokenString;
    }
    char *getToken() { return tokenString; }
};

class CompoundExpr : public Expr {
protected:
    Operator *op;
    Expr *left, *right; // left will be NULL if unary

    vector<Node *> children();

public:
    CompoundExpr(Expr *lhs, Operator *op, Expr *rhs); // for binary
    CompoundExpr(Operator *op, Expr *rhs);            // for unary
    virtual Location *cgen() {
        Location *l = left == NULL ? NULL : left->cgen();
        Location *r = right->cgen();
        if (l == NULL) {
            // unry
            l = CodeGenerator::instance->GenLoadConstant(0);
        }
        char *opStr = op->getToken();
        // {"+", "-", "*", "/", "%", "==", "<", "&&", "||"} + ">"
        return CodeGenerator::instance->GenBinaryOp(op->getToken(), l, r);
    }
};

class ArithmeticExpr : public CompoundExpr {
public:
    void Check(Context ctx) {
        CompoundExpr::Check(ctx);

        if ((left != NULL && isErrorTypeName(left->cachedType->GetName())) ||
            isErrorTypeName(right->cachedType->GetName())) {
            cachedType = Type::errorType;
            return;
        }
        if ((left == NULL || "int" == (string) left->cachedType->GetName()) &&
            "int" == (string) right->cachedType->GetName())
            cachedType = Type::intType;
        if ((left == NULL ||
             "double" == (string) left->cachedType->GetName()) &&
            "double" == (string) right->cachedType->GetName())
            cachedType = Type::doubleType;

        if (!cachedType) {
            if (left)
                ReportError::IncompatibleOperands(op, left->cachedType,
                                                  right->cachedType);
            else
                ReportError::IncompatibleOperand(op, right->cachedType);

            cachedType = Type::errorType;
        }
    }

    ArithmeticExpr(Expr *lhs, Operator *op, Expr *rhs)
        : CompoundExpr(lhs, op, rhs) {}
    ArithmeticExpr(Operator *op, Expr *rhs) : CompoundExpr(op, rhs) {}
};

class RelationalExpr : public ArithmeticExpr {
public:
    void Check(Context ctx) {
        ArithmeticExpr::Check(ctx);

        cachedType = Type::boolType;

        //most of the code is same except for rettype
    }
    RelationalExpr(Expr *lhs, Operator *op, Expr *rhs)
        : ArithmeticExpr(lhs, op, rhs) {}
    virtual Location *cgen();
};

class EqualityExpr : public CompoundExpr {
public:
    void Check(Context ctx) {
        CompoundExpr::Check(ctx);
        cachedType = Type::boolType;

        if (!isErrorTypeName(left->cachedType->GetName()) &&
            !isErrorTypeName(right->cachedType->GetName())) {
            if (!(typeMatchParent(left->cachedType, right->cachedType) ||
                  typeMatchParent(
                      right->cachedType,
                      left->cachedType))) //((string)left->cachedType->GetName()!=(string)right->cachedType->GetName())()
                ReportError::IncompatibleOperands(op, left->cachedType,
                                                  right->cachedType);
        }
    }
    EqualityExpr(Expr *lhs, Operator *op, Expr *rhs)
        : CompoundExpr(lhs, op, rhs) {}
    const char *GetPrintNameForNode() { return "EqualityExpr"; }
    virtual Location *cgen() {
        Location *l = left->cgen();
        Location *r = right->cgen();
        char *opStr = op->getToken();
        Location *equal = NULL;
        if (left->cachedType == Type::stringType) {
            equal = CodeGenerator::instance->GenBuiltInCall(StringEqual, l, r);
        } else {
            equal = CodeGenerator::instance->GenBinaryOp("==", l, r);
        }
        if (strcmp(opStr, "==") == 0)
            return equal;
        Location *zero = CodeGenerator::instance->GenLoadConstant(0);
        Location *rv = CodeGenerator::instance->GenBinaryOp("==", equal, zero);
        return rv;
    }
};

class LogicalExpr : public CompoundExpr {
public:
    void Check(Context ctx) {
        CompoundExpr::Check(ctx);
        cachedType = Type::boolType;
        if ((left == NULL || !isErrorTypeName(left->cachedType->GetName())) &&
            !isErrorTypeName(right->cachedType->GetName())) {
            if ((left != NULL &&
                 (string) left->cachedType->GetName() != "bool") ||
                (string) right->cachedType->GetName() != "bool") {
                if (left)
                    ReportError::IncompatibleOperands(op, left->cachedType,
                                                      right->cachedType);
                else
                    ReportError::IncompatibleOperand(op, right->cachedType);
            }
        }
    }
    LogicalExpr(Expr *lhs, Operator *op, Expr *rhs)
        : CompoundExpr(lhs, op, rhs) {}
    LogicalExpr(Operator *op, Expr *rhs) : CompoundExpr(op, rhs) {}
    const char *GetPrintNameForNode() { return "LogicalExpr"; }

    virtual Location *cgen() {
        if (left == NULL) {
            Assert(strcmp(op->getToken(), "!") == 0);
            Location *r = right->cgen();
            Location *zero = CodeGenerator::instance->GenLoadConstant(0);
            Location *rv = CodeGenerator::instance->GenBinaryOp("==", r, zero);
            return rv;
        } else {
            return CompoundExpr::cgen();
        }
    }
};

class AssignExpr : public CompoundExpr {
public:
    void Check(Context ctx) {
        CompoundExpr::Check(ctx);
        cachedType = left->cachedType;
        if (!isErrorTypeName(left->cachedType->GetName()) &&
            !isErrorTypeName(right->cachedType->GetName())) {
            if (!typeMatchParent(right->cachedType, left->cachedType))
                ReportError::IncompatibleOperands(op, left->cachedType,
                                                  right->cachedType);
        }
    }
    AssignExpr(Expr *lhs, Operator *op, Expr *rhs)
        : CompoundExpr(lhs, op, rhs) {}
    const char *GetPrintNameForNode() { return "AssignExpr"; }
    virtual Location *cgen() {
        Assert(false);
        return NULL;
    }
    virtual void Emit();
};

class LValue : public Expr {
public:
    LValue(yyltype loc) : Expr(loc) {}
    virtual void getAssign(Expr *expr) = 0;
};

class This : public Expr {
public:
    This(yyltype loc) : Expr(loc) {}

    void Check(Context ctx) {
        Expr::Check(ctx);
        if (!ctx.in_class) {
            ReportError::ThisOutsideClassScope(this);
            cachedType = Type::errorType;

        } else {
            cachedType = new NamedTypeNoLoc(
                new Identifier({0, 0, 0, 0, 0, 0}, ctx.outer_class->GetName()));
        }
    }
    Location *cgen() { return new Location(fpRelative, 4, "this"); }
};

class ArrayAccess : public LValue {
protected:
    Expr *base, *subscript;

    vector<Node *> children();

    // for IR
    Location *finalLocation = NULL;
    Location *baseLocation = NULL;
    int offSet = 0;
    void genBaseAndOffSet();
    void genFinalLocation();

public:
    void Check(Context ctx) {
        LValue::Check(ctx);
        if (!isErrorTypeName(base->cachedType->GetName())) {
            if (!isArrayTypeName(base->cachedType->GetName())) {
                ReportError::BracketsOnNonArray(base);
                cachedType = Type::errorType;
            } else {
                cachedType =
                    dynamic_cast<ArrayType *>(base->cachedType)->elemType;
            }
        } else {
            cachedType = Type::errorType;
        }

        if (!isErrorTypeName(subscript->cachedType->GetName()) &&
            (string) subscript->cachedType->GetName() != "int") {
            ReportError::SubscriptNotInteger(subscript);
        }
    }
    ArrayAccess(yyltype loc, Expr *base, Expr *subscript);

    Location *cgen();
    virtual void getAssign(Expr *expr);
};

/* Note that field access is used both for qualified names
 * base.field and just field without qualification. We don't
 * know for sure whether there is an implicit "this." in
 * front until later on, so we use one node type for either
 * and sort it out later. */
class FieldAccess : public LValue {
protected:
    Expr *base; // will be NULL if no explicit base
    Identifier *field;

    void Check(Context ctx) {
        LValue::Check(ctx);

        if (!base) {
            Assert(scope != NULL);
            auto src =
                dynamic_cast<VarDecl *>(scope->GetSymbol(field->GetName()));
            if (!src) {
                ReportError::IdentifierNotDeclared(field,
                                                   reasonT::LookingForVariable);
                cachedType = Type::errorType;

            } else {
                cachedType = src->type;
            }

        } else {
            //looks like need warp to another scope

            if (isErrorTypeName(base->cachedType->GetName())) {
                cachedType = Type::errorType;
            } else {

                if (dynamic_cast<NamedType *>(base->cachedType) ||
                    dynamic_cast<NamedTypeNoLoc *>(base->cachedType)) {
                    //named type
                    Decl *decl_node = StackNode::namedTypeTable->GetSymbol(
                        base->cachedType->GetName());
                    Assert(decl_node);

                    if (dynamic_cast<InterfaceDecl *>(decl_node)) {
                        ReportError::FieldNotFoundInBase(field,
                                                         base->cachedType);
                        cachedType = Type::errorType;
                    } else if (dynamic_cast<ClassDecl *>(decl_node)) {
                        auto field_decl_node =
                            dynamic_cast<ClassDecl *>(decl_node)->GetFields(
                                field->GetName());
                        if (dynamic_cast<FnDecl *>(field_decl_node)) {
                            ReportError::FieldNotFoundInBase(field,
                                                             base->cachedType);
                            cachedType = Type::errorType;
                        } else if (dynamic_cast<VarDecl *>(field_decl_node)) {
                            cachedType =
                                dynamic_cast<VarDecl *>(field_decl_node)->type;

                            if (!ctx.in_class)
                                ReportError::InaccessibleField(
                                    field, base->cachedType);
                            else {
                                Identifier tmp({0, 0, 0, 0, 0, 0},
                                               ctx.outer_class->GetName());
                                NamedTypeNoLoc outer_type(&tmp);
                                if (!typeMatchParent(&outer_type,
                                                     base->cachedType))
                                    ReportError::InaccessibleField(
                                        field, base->cachedType);
                            }
                        } else {
                            ReportError::FieldNotFoundInBase(field,
                                                             base->cachedType);
                            cachedType = Type::errorType;
                        }
                    }

                } else if (dynamic_cast<ArrayType *>(base->cachedType)) {
                    //array type
                    ReportError::FieldNotFoundInBase(field, base->cachedType);
                    cachedType = Type::errorType;
                } else {
                    //primitive

                    ReportError::FieldNotFoundInBase(field, base->cachedType);
                    cachedType = Type::errorType;
                }
            }
        }
    }

    vector<Node *> children();

    // For IR
    Location *baseLocation = NULL;
    int offSet = 0;
    void genBaseAndOffSet();

public:
    FieldAccess(Expr *base, Identifier *field); //ok to pass NULL base
    Location *cgen();
    void getAssign(Expr *expr);
};

/* Like field access, call is used both for qualified base.field()
 * and unqualified field().  We won't figure out until later
 * whether we need implicit "this." so we use one node type for either
 * and sort it out later. */
class Call : public Expr {
protected:
    Expr *base; // will be NULL if no explicit base
    Identifier *field;
    List<Expr *> *actuals;

    vector<Node *> children();
    void Check(Context ctx) {
        Expr::Check(ctx);
        bool check_sig_match = false;
        List<VarDecl *> *target = NULL;
        List<VarDecl *> emptyFormal;

        if (!base) {
            auto src =
                dynamic_cast<FnDecl *>(scope->GetSymbol(field->GetName()));
            if (!src) {
                ReportError::IdentifierNotDeclared(field,
                                                   reasonT::LookingForFunction);
                cachedType = Type::errorType;
            } else {
                cachedType = src->returnType;
                check_sig_match = true;
                target = src->formals;
            }

        } else {
            //looks like need warp to another scope
            if (isErrorTypeName(base->cachedType->GetName())) {
                cachedType = Type::errorType;
            } else {

                if (dynamic_cast<NamedType *>(base->cachedType) ||
                    dynamic_cast<NamedTypeNoLoc *>(base->cachedType)) {
                    //named type
                    Decl *decl_node = StackNode::namedTypeTable->GetSymbol(
                        base->cachedType->GetName());
                    Assert(decl_node);

                    if (dynamic_cast<InterfaceDecl *>(decl_node)) {
                        FnDecl *field_decl_node = dynamic_cast<FnDecl *>(
                            dynamic_cast<InterfaceDecl *>(decl_node)->GetFields(
                                field->GetName()));

                        if (field_decl_node) {
                            cachedType = field_decl_node->returnType;
                            check_sig_match = true;
                            target = field_decl_node->formals;
                        } else {
                            ReportError::FieldNotFoundInBase(field,
                                                             base->cachedType);
                            cachedType = Type::errorType;
                        }

                    } else if (dynamic_cast<ClassDecl *>(decl_node)) {
                        auto field_decl_node =
                            dynamic_cast<ClassDecl *>(decl_node)->GetFields(
                                field->GetName());
                        if (dynamic_cast<FnDecl *>(field_decl_node)) {
                            cachedType = dynamic_cast<FnDecl *>(field_decl_node)
                                             ->returnType;
                            check_sig_match = true;
                            target = dynamic_cast<FnDecl *>(field_decl_node)
                                         ->formals;
                        } else if (dynamic_cast<VarDecl *>(field_decl_node)) {
                            ReportError::FieldNotFoundInBase(field,
                                                             base->cachedType);
                            cachedType = Type::errorType;

                        } else {
                            ReportError::FieldNotFoundInBase(field,
                                                             base->cachedType);
                            cachedType = Type::errorType;
                        }
                    }

                } else if (dynamic_cast<ArrayType *>(base->cachedType)) {
                    //array type
                    if ((string) field->GetName() != "length") {
                        ReportError::FieldNotFoundInBase(field,
                                                         base->cachedType);
                        cachedType = Type::errorType;
                    } else {
                        cachedType = Type::intType;
                        check_sig_match = true;
                        target = &emptyFormal;
                    }
                } else {
                    //primitive
                    ReportError::FieldNotFoundInBase(field, base->cachedType);
                    cachedType = Type::errorType;
                }
            }
        }

        if (check_sig_match) {
            if (target->NumElements() != actuals->NumElements())
                ReportError::NumArgsMismatch(field, target->NumElements(),
                                             actuals->NumElements());
            else {
                for (int i = 0; i < target->NumElements(); i++) {
                    auto curActual = actuals->Nth(i)->cachedType;
                    auto curTarget = target->Nth(i)->type;

                    if (!typeMatchParent(curActual, curTarget)) {
                        ReportError::ArgMismatch(actuals->Nth(i), i + 1,
                                                 curActual, curTarget);
                    }
                }
            }
        }
    }

    void genPushParams();
    void genPopParams();
    bool ifAcall = false;
    // Location* baseLocation = NULL;
    // List<Location*>* params = NULL;
public:
    Call(yyltype loc, Expr *base, Identifier *field, List<Expr *> *args);

    Location *cgen();
};

class NewExpr : public Expr {
protected:
    NamedType *cType;
    vector<Node *> children();

public:
    void Check(Context ctx) {
        Expr::Check(ctx);
        cachedType = cType;
        if (!cachedType)
            cachedType = Type::errorType;
    }
    NewExpr(yyltype loc, NamedType *clsType);
    void CheckType();
    Location *cgen();
};

class NewArrayExpr : public Expr {
protected:
    Expr *size;
    Type *elemType;

    vector<Node *> children();

public:
    void Check(Context ctx) {
        Expr::Check(ctx);
        if ((!isErrorTypeName(size->cachedType->GetName())) &&
            (string) size->cachedType->GetName() != "int") {
            ReportError::NewArraySizeNotInteger(size);
        }

        cachedType = new ArrayTypeNoLoc({0, 0, 0, 0, 0, 0}, elemType);
    }
    NewArrayExpr(yyltype loc, Expr *sizeExpr, Type *elemType);
    void CheckType();
    Location *cgen();
};

class ReadIntegerExpr : public Expr {
public:
    void Check(Context ctx) {
        Expr::Check(ctx);
        cachedType = Type::intType;
    }
    ReadIntegerExpr(yyltype loc) : Expr(loc) {}
    Location *cgen() {
        return CodeGenerator::instance->GenBuiltInCall(ReadInteger);
    }
};

class ReadLineExpr : public Expr {
public:
    void Check(Context ctx) {
        Expr::Check(ctx);
        cachedType = Type::stringType;
    }
    ReadLineExpr(yyltype loc) : Expr(loc) {}
    Location *cgen() {
        return CodeGenerator::instance->GenBuiltInCall(ReadLine);
    }
};

#endif
