#include "scope.h"
#include "ast_decl.h"

/* StackNode* StackNode::root = new StackNode(NULL); */
/* StackNode* StackNode::namedTypeTable = new StackNode(NULL); */

Decl *StackNode::GetSymbol(const char *symbol) {
    StackNode *tmp = this;
    Decl *rv = tmp->symbols->Lookup(symbol);
    if (rv != NULL)
        return rv;
    if (parent)
        return parent->GetSymbol(symbol);
    return NULL;
}

Decl *StackNode::AddSymbol(const char *symbol, Decl *decl) {
    Decl *rv = this->symbols->Lookup(symbol);
	if (rv != NULL) {
		ReportError::DeclConflict(decl, rv);
        return rv;
	}
    symbols->Enter(symbol, decl);
    return NULL;
}

Decl *ClassStackNode::GetSymbol(const char *symbol) {
    Decl *rv = classDecl->GetFields(symbol);
    if (rv)
        return rv;
    Assert(parent != NULL);
    return parent->GetSymbol(symbol);
}

StackNode* StackNode::root = NULL;
StackNode* StackNode::namedTypeTable = NULL;
/* Decl *ClassStackNode::GetFields(const char *symbol) { */
/*     set<ClassDecl *> visited; */
/*     return classDecl->GetExtends(&visited, symbol); */
/* } */

/* Decl *ClassStackNode::GetFieldsHelper(set<ClassDecl *> *visited, */
/*                                       const char *symbol) { */
/*     // visited to resolve circular inheritance */
/*     if (visited->count(classDecl)) */
/*         return NULL; */
/*     Decl *rv = this->symbols->Lookup(symbol); */
/*     if (rv != NULL) */
/*         return rv; */
/*     visited->insert(classDecl); */
/* 	ClassDecl* extends = classDecl->GetExtends(); */
/* 	if (extends ) { */
/* 		return extends->scope-> */
/* 		/1* return extends-> *1/ */
/* 	} */
/*     return NULL; */
/* } */

