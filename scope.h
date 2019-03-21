#ifndef _H_scope
#define _H_scope
#include "errors.h"
#include "hashtable.h"
#include "utility.h"
#include <set>
using std::set;

class Decl;
class ClassDecl;
class StackNode {
public:
    StackNode *parent;
    // store vardecl
    Hashtable<Decl *> *symbols;

    StackNode(StackNode *parent) : parent(parent) {
        symbols = new Hashtable<Decl *>();
    }

    virtual Decl *GetSymbol(const char *symbol);

    Decl *AddSymbol(const char *symbol, Decl *decl);

    void SetSymbolTable(Hashtable<Decl *> *table) { symbols = table; }

    Hashtable<Decl *> *GetSymbolTable() { return symbols; }

    static StackNode *root;
    static StackNode *namedTypeTable;
};

class ClassStackNode : public StackNode {
public:
    ClassDecl *classDecl;
    ClassStackNode(StackNode *parent, ClassDecl *classDecl)
        : StackNode(parent), classDecl(classDecl) {
        Assert(classDecl != NULL);
    }

    Decl *GetSymbol(const char *symbol);
};

class Context{
public:
    bool in_class=false;
    ClassDecl* outer_class=NULL;

    bool in_loop=false;


    Type* rettype= nullptr;

};  


/* class ClassTable { */
/* public: */
/*     Hashtable<Decl *> *NamedTypeTable; // interface and parent */
/*     TypeTable() : NamedTypeTable(new Hashtable<Decl *>()) {} */
/* }; */

#endif
