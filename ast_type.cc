/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */
#include "ast_type.h"
#include "ast_decl.h"
#include "scope.h"
#include <string.h>
 
/* Class constants
 * ---------------
 * These are public constants for the built-in base types (int, double, etc.)
 * They can be accessed with the syntax Type::intType. This allows you to
 * directly access them and share the built-in types where needed rather that
 * creates lots of copies.
 */

Type *Type::intType    = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType   = new Type("void");
Type *Type::boolType   = new Type("bool");
Type *Type::nullType   = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType  = new Type("#error");//since error could also be a type name we somehow need to separate them

Type::Type(const char *n) {
    Assert(n);
    typeName = strdup(n);
}
	
NamedType::NamedType(Identifier *i) : Type(*i->GetLocation()) {
    Assert(i != NULL);
    (id=i)->SetParent(this);
} 

Type* NamedType::CheckTypeHelper(reasonT reason) {

    if(reason==reasonT::LookingForClass)
    {
        if (dynamic_cast<ClassDecl*>(StackNode::namedTypeTable->GetSymbol(GetName())) == NULL) {
            ReportError::IdentifierNotDeclared(id, reason);
            return errorType;
        }

    }else if (StackNode::namedTypeTable->GetSymbol(GetName()) == NULL) {
        ReportError::IdentifierNotDeclared(id, reason);
        return errorType;
    }
    return this;
}

/* bool NamedType::IsCompatible(Type *other) { */
/*     NamedType* otherType = dynamic_cast<NamedType*>(other); */
/*     if (otherType == NULL) return false; */
/*     if (classDecl == NULL && interfaceDecl == NULL) return true; */
/*     ClassDecl* otherDecl = otherType->classDecl; */
/*     if (otherDecl == NULL) return (otherType->interfaceDecl); */
/*     return classDecl->IsCompatible(otherDecl); */
/* } */

ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}

/* bool ArrayType::IsCompatible(Type *other) { */
/*     ArrayType* otherType = dynamic_cast<ArrayType*>(other); */
/*     if (otherType == NULL) return false; */
/*     return elemType->IsCompatible(otherType->elemType); */
/* } */

Type* ArrayType::CheckTypeHelper(reasonT reason) {
    elemType = elemType->CheckTypeHelper(reason);
    return this;
}

