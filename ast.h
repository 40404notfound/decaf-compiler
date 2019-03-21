/* File: ast.h
 * ----------- 
 * This file defines the abstract base class Node and the concrete 
 * Identifier and Error node subclasses that are used through the tree as 
 * leaf nodes. A parse tree is a hierarchical collection of ast nodes (or, 
 * more correctly, of instances of concrete subclassses such as VarDecl,
 * ForStmt, and AssignExpr).
 * 
 * Location: Each node maintains its lexical location (line and columns in 
 * file), that location can be NULL for those nodes that don't care/use 
 * locations. The location is typcially set by the node constructor.  The 
 * location is used to provide the context when reporting semantic errors.
 *
 * Parent: Each node has a pointer to its parent. For a Program node, the 
 * parent is NULL, for all other nodes it is the pointer to the node one level
 * up in the parse tree.  The parent is not set in the constructor (during a 
 * bottom-up parse we don't know the parent at the time of construction) but 
 * instead we wait until assigning the children into the parent node and then 
 * set up links in both directions. The parent link is typically not used 
 * during parsing, but is more important in later phases.
 *
 * Semantic analysis: For pp3 you are adding "Check" behavior to the ast
 * node classes. Your semantic analyzer should do an inorder walk on the
 * parse tree, and when visiting each node, verify the particular
 * semantic rules that apply to that construct.

 */

#ifndef _H_ast
#define _H_ast

#include "location.h"
#include "scope.h"
#include <iostream>
#include <set>
#include <stdlib.h> // for NULL

#include "codegen.h"
#include <string>
#include <vector>
using std::set;
using std::string;

using std::vector;
class Node {
protected:
    yyltype *location;
    Node *parent;
    virtual vector<Node *> children() {
        vector<Node *> result;
        return result;
    }

public:
    Node(yyltype loc);
    Node();

    StackNode *scope = NULL;
    yyltype *GetLocation() { return location; }
    void SetParent(Node *p) { parent = p; }
    Node *GetParent() { return parent; }

    virtual void AddGlobal();
    virtual void CheckType();
    virtual void ResolveConflict(set<Node *> *visited);
    virtual void resolve_conflict_decl(Context ctx);
    virtual void resolve_identifier_type(Context ctx);
    virtual void resolve_class_fields(Context ctx);
    virtual void resolve_self_access(Context ctx);
    virtual void resolve_field_access(Context ctx);
    virtual void resolve_array_type(Context ctx);
    virtual void resolve_compound_expr_type(Context ctx);
    virtual void resolve_call_args(Context ctx);
    virtual void resolve_improper_statement(Context ctx);
    virtual void Check() { assert(false); }
    virtual void Check(Context ctx);

    virtual void Emit() {return;}
};

class Identifier : public Node {
protected:
    char *name;

public:
    Identifier(yyltype loc, const char *name);
    friend std::ostream &operator<<(std::ostream &out, Identifier *id) {
        return out << id->name;
    }
    char *GetName() { return name; }
};

// This node class is designed to represent a portion of the tree that
// encountered syntax errors during parsing. The partial completed tree
// is discarded along with the states being popped, and an instance of
// the Error class can stand in as the placeholder in the parse tree
// when your parser can continue after an error.
class Error : public Node {
public:
    Error() : Node() {}
};

#endif
