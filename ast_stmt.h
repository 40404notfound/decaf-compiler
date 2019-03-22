/* File: ast_stmt.h
 * ----------------
 * The Stmt class and its subclasses are used to represent
 * statements in the parse tree.  For each statment in the
 * language (for, if, return, etc.) there is a corresponding
 * node class for that construct. 
 *
 * pp3: You will need to extend the Stmt classes to implement
 * semantic analysis for rules pertaining to statements.
 */

#ifndef _H_ast_stmt
#define _H_ast_stmt

#include "ast.h"
#include "ast_decl.h"
#include "list.h"

class Decl;
class VarDecl;
class Expr;

class Program : public Node {
protected:
    List<Decl *> *decls;
    vector<Node *> children();

public:
    Program(List<Decl *> *declList);
    void Check();
    void Emit();
};

class Stmt : public Node {
public:
    Stmt() : Node() {}
    Stmt(yyltype loc) : Node(loc) {}
};

class StmtBlock : public Stmt {
protected:
    List<VarDecl *> *decls;
    List<Stmt *> *stmts;
    vector<Node *> children();

public:
    StmtBlock(List<VarDecl *> *variableDeclarations, List<Stmt *> *statements);
    void ResolveConflict(set<Node *> *visited);
    virtual void Emit();
};

class ConditionalStmt : public Stmt {
protected:
    Expr *test;
    Stmt *body;
    vector<Node *> children();

public:
    void Check(Context ctx);

    ConditionalStmt(Expr *testExpr, Stmt *body);
};

class LoopStmt : public ConditionalStmt {
public:
    void Check(Context ctx) {
        ctx.in_loop = true;
        ConditionalStmt::Check(ctx);
    }
    LoopStmt(Expr *testExpr, Stmt *body) : ConditionalStmt(testExpr, body) {}
};

class ForStmt : public LoopStmt {
protected:
    Expr *init, *step;
    vector<Node *> children();

public:
    ForStmt(Expr *init, Expr *test, Expr *step, Stmt *body);
    virtual void Emit();
};

class WhileStmt : public LoopStmt {
public:
    WhileStmt(Expr *test, Stmt *body) : LoopStmt(test, body) {}
    virtual void Emit();
};

class IfStmt : public ConditionalStmt {
protected:
    Stmt *elseBody;

    vector<Node *> children();

public:
    IfStmt(Expr *test, Stmt *thenBody, Stmt *elseBody);
    virtual void Emit();
};

class BreakStmt : public Stmt {
public:
    BreakStmt(yyltype loc) : Stmt(loc) {}
    void Check(Context ctx) {
        Stmt::Check(ctx);
        if (!ctx.in_loop)
            ReportError::BreakOutsideLoop(this);
    }
    virtual void Emit();
};

class ReturnStmt : public Stmt {
protected:
    Expr *expr;
    vector<Node *> children();

public:
    void Check(Context ctx);
    ReturnStmt(yyltype loc, Expr *expr);
    virtual void Emit();
};

class PrintStmt : public Stmt {
protected:
    List<Expr *> *args;
    vector<Node *> children();

public:
    void Check(Context ctx);
    void Emit();

    PrintStmt(List<Expr *> *arguments);
};

#endif
