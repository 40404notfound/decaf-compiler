/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp3: You will need to extend the Decl classes to implement 
 * semantic processing including detection of declaration conflicts 
 * and managing scoping issues.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "ast_stmt.h"
#include "ast_type.h"
#include "list.h"

class Identifier;
class Stmt;

class Decl : public Node {
protected:
    Identifier *id;

    vector<Node *> children() {
        vector<Node *> result = Node::children();
        result.push_back(id);
        return result;
    }

public:
    Decl(Identifier *name);
    friend std::ostream &operator<<(std::ostream &out, Decl *d) {
        return out << d->id;
    }
    char *GetName() { return id->GetName(); }
    virtual void AddGlobal();
    virtual bool CheckDeclMatch(Decl *decl) { return true; }
    // only for funDecl and VarDecl
};

class VarDecl : public Decl {
    friend class FieldAccess;
    friend class Call;

protected:
    Type *type;

    vector<Node *> children() {
        vector<Node *> result = Decl::children();
        result.push_back((Node *) (type));
        return result;
    }

public:
    Location *location = NULL;
    int offset = -1; // only for member variables
    VarDecl(Identifier *name, Type *type);
    Type *GetType() { return type; }
    bool CheckDeclMatch(Decl *decl);
    void CheckType();
    void ResolveConflict(set<Node *> *visited);
    void Emit();
};

class InterfaceDecl;
class ClassDecl : public Decl {
protected:
    List<Decl *> *members;
    NamedType *extends;
    List<NamedType *> *implements;

    // for semantic checking
    ClassDecl *extendClass = NULL;
    List<InterfaceDecl *> *implementInterfaces;

    Decl *GetFieldsHelper(set<ClassDecl *> *visited, const char *symbol);
    bool IfImplementOrExtendHelper(set<ClassDecl *> *visited, Decl *other);

    vector<Node *> children() {
        vector<Node *> result = Decl::children();
        result.insert(result.end(), members->elems.begin(),
                      members->elems.end());
        if (extends != NULL) {
            result.push_back((Node *) (extends));
        }
        result.insert(result.end(), implements->elems.begin(),
                      implements->elems.end());

        return result;
    }

    void Check(Context ctx);

public:
    ClassDecl(Identifier *name,
              NamedType *extends,
              List<NamedType *> *implements,
              List<Decl *> *members);
    /* virtual bool IsCompatible(ClassDecl *other) {return true;} */
    ClassDecl *GetExtends() { return extendClass; }
    void AddGlobal();
    Decl *GetFields(const char *symbol) {
        set<ClassDecl *> visited;
        return GetFieldsHelper(&visited, symbol);
    }
    bool IfImplementOrExtend(Decl *parent) {
        set<ClassDecl *> visited;
        return IfImplementOrExtendHelper(&visited, parent);
    }
    static ClassDecl *GetClass(const char *symbol) {
        // utility function to get classdecl associated with symbol
        Decl *decl = StackNode::namedTypeTable->GetSymbol(symbol);
        if (decl)
            return dynamic_cast<ClassDecl *>(decl);
        return NULL;
    }
    void CheckType();
    void ResolveConflict(set<Node *> *visited);

    // for IR
    List<const char *> *methodLabels=NULL;
    // index for methods 
    Hashtable<int> *methodTable=NULL;
    // numVar for variableOffsets of children
    int numVar = 0;
    void generateLocations();
    void Emit();
};

class InterfaceDecl : public Decl {
    friend class ClassDecl;

protected:
    List<Decl *> *members;
    vector<Node *> children() {
        vector<Node *> result = Decl::children();
        result.insert(result.end(), members->elems.begin(),
                      members->elems.end());
        return result;
    }

public:
    InterfaceDecl(Identifier *name, List<Decl *> *members);
    void AddGlobal();
    Decl *GetFields(const char *symbol) {
        return scope->GetSymbolTable()->Lookup(symbol);
    }
    static InterfaceDecl *GetInterface(const char *symbol) {
        // utility function to get classdecl associated with symbol
        Decl *decl = StackNode::namedTypeTable->GetSymbol(symbol);
        if (decl)
            return dynamic_cast<InterfaceDecl *>(decl);
        return NULL;
    }
    void ResolveConflict(set<Node *> *visited) { return; }
};

class FnDecl : public Decl {
    friend class Call;

protected:
    List<VarDecl *> *formals;
    Type *returnType;
    Stmt *body;
    vector<Node *> children();

public:
    void Check(Context ctx) {
        ctx.rettype = returnType;
        Decl::Check(ctx);
    }
    FnDecl(Identifier *name, Type *returnType, List<VarDecl *> *formals);
    void SetFunctionBody(Stmt *b);
    bool CheckDeclMatch(Decl *decl);
    // void AddSymbols(Scope *scope);
    void CheckType();
    friend bool fnMatch(FnDecl *, FnDecl *);
    void ResolveConflict(set<Node *> *visited);

    void Emit();
    char *label = NULL;
    int offset = -1; // only for member funcs
};

#endif
