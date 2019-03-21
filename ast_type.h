/* File: ast_type.h
 * ----------------
 * In our parse tree, Type nodes are used to represent and
 * store type information. The base Type class is used
 * for built-in types, the NamedType for classes and interfaces,
 * and the ArrayType for arrays of other types.  
 *
 * pp3: You will need to extend the Type classes to implement
 * the type system and rules for type equivalency and compatibility.
 */

#ifndef _H_ast_type
#define _H_ast_type

#include "ast.h"
#include "list.h"
#include <iostream>
#include <string>

using std::string;

class Type : public Node {
protected:
    char *typeName;

public:
    static Type *intType, *doubleType, *boolType, *voidType, *nullType,
        *stringType, *errorType;

    Type(yyltype loc) : Node(loc) {}
    Type(const char *str);

    virtual void PrintToStream(std::ostream &out) {
        if (typeName && typeName[0] == '#')
            out << (typeName + 1);
        else
            out << typeName;
    }
    friend std::ostream &operator<<(std::ostream &out, Type *t) {
        t->PrintToStream(out);
        return out;
    }
    // this means we can use this = other
    /* virtual bool IsCompatible(Type *other) { return this == other; } */
    virtual const char *GetName() { return typeName; }
    virtual Type *CheckTypeHelper(reasonT) { return this; }
};

class NamedType : public Type {
protected:
    Identifier *id;
    vector<Node *> children() {
        vector<Node *> result = Type::children();
        result.push_back(id);
        return result;
    }

    /*void Check(Context ctx) {
        Node::Check(ctx);
        auto src=id->scope->GetSymbol(id->GetName());

        if(src==NULL)
            ;//ReportError::IdentifierNotDeclared(id,reasonT::LookingForType); OK ALREADY CHECKED BY YXY SO NO NEED CHECK FOR IT AGAIN


    }*/

public:
    NamedType(Identifier *i);
    void PrintToStream(std::ostream &out) { out << id; }
    /* virtual bool IsCompatible(Type *other); */
    const char *GetName() { return id->GetName(); }
    Identifier *GetId() { return id; }
    Type *CheckTypeHelper(reasonT);
};

//INVISIBLE TYPE NODE FOR CLASS AND INTERFACE
class NamedTypeNoLoc : public NamedType {
public:
    NamedTypeNoLoc(Identifier *i) : NamedType(i) {}
    void Check(
        Context ctx) { /*I dont need any check,so override the default Check()*/
    }
};

class ArrayType : public Type {
    friend class ArrayAccess;

protected:
    char *cachedName = NULL;
    Type *elemType;

    vector<Node *> children() {
        vector<Node *> result = Type::children();
        result.push_back(elemType);
        return result;
    }

public:
    ArrayType(yyltype loc, Type *elemType);

    void PrintToStream(std::ostream &out) { out << elemType << "[]"; }
    const char *GetName() {
        if (!cachedName) {
            cachedName = new char[strlen(elemType->GetName()) + 2];
            strcpy(cachedName, elemType->GetName());
            strcat(cachedName, "[]");
        }
        return cachedName;
    } //I believe this is a bug
    /* virtual bool IsCompatible(Type *other); */
    Type *CheckTypeHelper(reasonT);
};

class ArrayTypeNoLoc : public ArrayType {
public:
    ArrayTypeNoLoc(yyltype loc, Type *elemType) : ArrayType(loc, elemType) {}
    void Check(
        Context ctx) { /*I dont need any check,so override the default Check()*/
    }
};

bool typeMatchParent(Type *children, Type *parent);
#endif
