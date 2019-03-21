/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}

#define REC_DEFINE_NO_CTX(NAME) void Node::NAME() {for(auto& i:children()){if (i) i->NAME();}}
REC_DEFINE_NO_CTX(AddGlobal);
REC_DEFINE_NO_CTX(CheckType);
#undef REC_DEFINE_NO_CTX


void Node::ResolveConflict(set<Node*>* visited) {
	if (parent) {
		scope = parent->scope;
	}
	for(auto& i:children()){
		if (i) i->ResolveConflict(visited);
	}
}

//recursive
#define REC_DEFINE(NAME) void Node::NAME(Context ctx) {for(auto& i:children()){if(i) i->NAME(ctx);}}
REC_DEFINE(resolve_conflict_decl)
REC_DEFINE(resolve_identifier_type)
REC_DEFINE(resolve_class_fields)
REC_DEFINE(resolve_self_access)
REC_DEFINE(resolve_field_access)
REC_DEFINE(resolve_array_type)
REC_DEFINE(resolve_compound_expr_type)
REC_DEFINE(resolve_call_args)
REC_DEFINE(resolve_improper_statement)
REC_DEFINE(Check)
#undef REC_DEFINE
	 
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
}



