/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_stmt.h"
#include "ast_type.h"
#include "scope.h"
#include <iostream>
#include <string>
using std::string;

Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id = n)->SetParent(this);
}

void PrintNames(Hashtable<Decl *> *table) {
    Iterator<Decl *> iter = table->GetIterator();
    Decl *decl;
    while ((decl = iter.GetNextValue()) != NULL) {
        printf("%s\n", decl->GetName());
    }
}

void Decl::AddGlobal() {
    Decl *prev = StackNode::root->GetSymbol(id->GetName());
    if (prev) {
        ReportError::DeclConflict(this, prev);
    } else {
        StackNode::root->AddSymbol(id->GetName(), this);
    }
    this->scope = new StackNode(StackNode::root);
    return;
}

VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type = t)->SetParent(this);
}

void VarDecl::CheckType() {
    type = type->CheckTypeHelper(LookingForType);
}

void VarDecl::ResolveConflict(set<Node *> *visited) {
    // this doesn't matter much
    // we don't really need scope for var decl
    /* scope = parent->scope; */
    /* printf("%s\n", GetName()); */
    return;
}

bool VarDecl::CheckDeclMatch(Decl *decl) {
    // Can't have duplicate var decl
    Assert(decl);
    ReportError::DeclConflict(this, decl);
    return false;
}

void VarDecl::Emit() {
    // only funtion local var locations
    Assert(location == NULL);
    int offSet = CodeGenerator::OffsetToFirstLocal -
                 CodeGenerator::instance->localVarNum * 4;
    CodeGenerator::instance->localVarNum++;
    location = new Location(fpRelative, offSet, GetName());
}

ClassDecl::ClassDecl(Identifier *n,
                     NamedType *ex,
                     List<NamedType *> *imp,
                     List<Decl *> *m)
    : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);
    extends = ex;
    if (extends)
        extends->SetParent(this);
    (implements = imp)->SetParentAll(this);
    (members = m)->SetParentAll(this);
}

void ClassDecl::AddGlobal() {
    Decl *prev = StackNode::root->GetSymbol(id->GetName());
    if (prev) {
        ReportError::DeclConflict(this, prev);
    } else {
        StackNode::root->AddSymbol(id->GetName(), this);
        StackNode::namedTypeTable->AddSymbol(id->GetName(), this);
    }
    this->scope = new ClassStackNode(StackNode::root, this);
    return;
}

Decl *ClassDecl::GetFieldsHelper(set<ClassDecl *> *visited,
                                 const char *symbol) {
    if (visited->count(this))
        return NULL;
    Decl *rv = scope->symbols->Lookup(symbol);
    if (rv != NULL)
        return rv;
    visited->insert(this);
    if (extendClass) {
        return extendClass->GetFieldsHelper(visited, symbol);
    }
    return NULL;
}

bool ClassDecl::IfImplementOrExtendHelper(set<ClassDecl *> *visited,
                                          Decl *other) {
    if (visited->count(this))
        return false;
    visited->insert(this);
    for (int i = 0; i < implementInterfaces->NumElements(); i++) {
        InterfaceDecl *decl = implementInterfaces->Nth(i);
        // decl is NULL for undeclared interface
        if (decl && decl == other)
            return true;
    }
    if (extendClass) {
        if (extendClass == other)
            return true;
        return extendClass->IfImplementOrExtendHelper(visited, other);
    }
    return false;
}

void ClassDecl::CheckType() {
    Node::CheckType();
    // Check extend and implements
    if (extends) {
        Decl *tmp = StackNode::namedTypeTable->GetSymbol(extends->GetName());
        extendClass = dynamic_cast<ClassDecl *>(tmp);
        if (extendClass == NULL)
            ReportError::IdentifierNotDeclared(extends->GetId(),
                                               LookingForClass);
        if (extendClass == this)
            extendClass = NULL;
        /* extends = dynamic_cast<NamedType *>(extends->CheckTypeHelper(LookingForClass)); */
    }
    implementInterfaces = new List<InterfaceDecl *>();
    for (int i = 0; i < implements->NumElements(); i++) {
        NamedType *cur = implements->Nth(i);
        Decl *tmp = StackNode::namedTypeTable->GetSymbol(cur->GetName());
        InterfaceDecl *interfaceDecl = dynamic_cast<InterfaceDecl *>(tmp);
        if (interfaceDecl == NULL) {
            ReportError::IdentifierNotDeclared(cur->GetId(),
                                               LookingForInterface);
        }
        // invalid interface is NULL
        implementInterfaces->Append(interfaceDecl);
    }
}

void ClassDecl::ResolveConflict(set<Node *> *visited) {
    if (visited->count(this)) {
        return;
    }
    visited->insert(this);
    // check extends
    // extendClass has already been solved in Checktype
    if (extendClass) {
        extendClass->ResolveConflict(visited);
    }
    // add members
    for (int i = 0; i < members->NumElements(); i++) {
        Decl *member = members->Nth(i);
        // this is for funtion
        member->ResolveConflict(visited);
        // skip variables
        // check extend class ovrride
        if (extendClass) {
            Decl *prevField = extendClass->GetFields(member->GetName());
            // this is only for circular inheritance
            if (prevField &&
                scope->GetSymbolTable()->Lookup(member->GetName())) {
                ReportError::DeclConflict(member, prevField);
                continue;
            }
            if (prevField && !member->CheckDeclMatch(prevField)) {
                continue;
            }
        }
        // check interfaces override
        bool ifConflict = false;
        if (dynamic_cast<FnDecl *>(member)) {
            for (int i = 0; i < implementInterfaces->NumElements(); i++) {
                InterfaceDecl *decl = implementInterfaces->Nth(i);
                // decl is NULL for undeclared interface
                if (decl) {
                    Decl *prevField = decl->GetFields(member->GetName());
                    if (prevField && !member->CheckDeclMatch(prevField)) {
                        ifConflict = true;
                        break;
                    }
                }
            }
        }
        if (!ifConflict) {
            scope->AddSymbol(member->GetName(), member);
        }
        // var resolveconflict will do nothing
    }
}

void ClassDecl::Check(Context ctx) {
    //cannot call default Check because of duplicate check

    ctx.in_class = true;
    ctx.outer_class = this;
    for (auto &i : members->elems) {
        if (i)
            i->Check(ctx);
    }

    int counter = 0;
    for (auto &i : implementInterfaces->elems) {

        if (i) {
            for (auto &prototype : i->members->elems) {
                auto src_node = GetFields(prototype->GetName());
                if (!src_node || !fnMatch(dynamic_cast<FnDecl *>(src_node),
                                          dynamic_cast<FnDecl *>(prototype))) {
                    ReportError::InterfaceNotImplemented(
                        this, this->implements->Nth(counter));
                    break;
                }
            }
        }
        counter++;
    }
}

char *getMethodLabel(const char *className, const char *methodName) {
    char tmp[128];
    Assert(methodName);
    if (className) {
        sprintf(tmp, "_%s.%s", className, methodName);
    } else {
        sprintf(tmp, "_%s", methodName);
    }
    return strdup(tmp);
}

void ClassDecl::generateLocations() {
    if (methodLabels)
        return;
    methodLabels = new List<const char *>;
    methodTable = new Hashtable<FnDecl *>;
    methodNames = new List<const char *>;
    if (extendClass) {
        extendClass->generateLocations();
        numVar = extendClass->numVar;
        Assert(extendClass->methodLabels->NumElements() ==
               extendClass->methodNames->NumElements());
        for (int i = 0; i < extendClass->methodLabels->NumElements(); i++) {
            const char *methodLabel = extendClass->methodLabels->Nth(i);
            const char *name = extendClass->methodNames->Nth(i);
            char *methodDup = strdup(methodLabel);
            methodLabels->Append(methodDup);
            methodNames->Append(name);
            FnDecl *parentMethod = extendClass->methodTable->Lookup(name);
            Assert(parentMethod);
            methodTable->Enter(name, parentMethod);
        }
    }

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *decl = members->Nth(i);
        VarDecl *varDecl = dynamic_cast<VarDecl *>(decl);
        FnDecl *fnDecl = dynamic_cast<FnDecl *>(decl);
        if (varDecl) {
            // should not be a global or local variable
            Assert(varDecl->location == NULL);
            // must be a new variable cuz no conflict
            varDecl->offset =
                CodeGenerator::OffsetToFirstMember + 4 * (numVar++);
        } else if (fnDecl) {
            const char *methodName = fnDecl->GetName();
            char *methodLabel = getMethodLabel(GetName(), methodName);
            fnDecl->label = methodLabel;
            FnDecl *parentMethod = methodTable->Lookup(methodName);
            int newOffSet = 0;
            if (parentMethod) {
                // insert the new overload method label at the original index
                newOffSet = parentMethod->offset;
                Assert(newOffSet != -1);
                // printf("%d\n", newOffSet);
                int index = newOffSet / CodeGenerator::VarSize;
                methodLabels->RemoveAt(index);
                methodLabels->InsertAt(methodLabel, index);
                methodTable->Enter(methodName, fnDecl);
                // dont need to update methodName cuz the same
            } else {
                newOffSet =
                    methodLabels->NumElements() * CodeGenerator::VarSize;
                methodLabels->Append(methodLabel);
                methodNames->Append(methodName);
                methodTable->Enter(methodName, fnDecl);
            }
            Assert(fnDecl->offset == -1);
            fnDecl->offset = newOffSet;
        } else {
            Assert(false);
        }
    }
}

void ClassDecl::Emit() {
    for (auto &i : members->elems) {
        if (dynamic_cast<FnDecl *>(i))
            i->Emit();
    }
    CodeGenerator::instance->GenVTable(GetName(), methodLabels);
}

/* bool InterfaceDecl::checkClassImplementThis(ClassDecl* classDecl) { */
/* 	int numMethodsToImplent = member->; */
/* } */

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl *> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members = m)->SetParentAll(this);
}

void InterfaceDecl::AddGlobal() {
    Decl *prev = StackNode::root->GetSymbol(id->GetName());
    if (prev) {
        ReportError::DeclConflict(this, prev);
    } else {
        StackNode::root->AddSymbol(id->GetName(), this);
        StackNode::namedTypeTable->AddSymbol(id->GetName(), this);
    }
    this->scope = new StackNode(StackNode::root);
    // we need to check interface conflict methods here
    // because we need this to report override conflict in resolveconflict
    for (int i = 0; i < members->NumElements(); i++) {
        Decl *cur = members->Nth(i);
        scope->AddSymbol(members->Nth(i)->GetName(), cur);
    }
    return;
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl *> *d) : Decl(n) {
    Assert(n != NULL && r != NULL && d != NULL);
    (returnType = r)->SetParent(this);
    (formals = d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) {
    (body = b)->SetParent(this);
}

void FnDecl::CheckType() {
    Node::CheckType();
    // parent method will not check returnType
    if (returnType) {
        returnType = returnType->CheckTypeHelper(LookingForType);
    }
}

// helper for check decl match
bool typeMatch(Type *l, Type *r) {
    // check l and r are exactly the same
    if (l == Type::errorType || r == Type::errorType ||
        isErrorTypeName(l->GetName()) || isErrorTypeName(r->GetName()))
        return true;
    return (strcmp(l->GetName(), r->GetName()) == 0);
}

Decl *getTypeDecl(Type *t) {
    Decl *rv = StackNode::namedTypeTable->GetSymbol(t->GetName());
    return rv;
}

// helper for check decl match
bool typeMatchParent(Type *children, Type *parent) {
    // check if parent = children is compatible
    // e.g. [children] is the same or the subclass of [parent]
    // exactly the same

    //not likelyto happen

    //std::cout<<children->GetName()<<parent->GetName()<<std::endl;
    if ((string) parent->GetName() == "null")
        return false;
    if ((string) children->GetName() == "null" &&
        dynamic_cast<NamedType *>(parent))
        return true;

    if (typeMatch(children, parent))
        return true;
    // check if children is subclass of parent
    Decl *parentDecl = getTypeDecl(parent);
    Decl *childrenDeclTmp = getTypeDecl(children);
    ClassDecl *childrenDecl = dynamic_cast<ClassDecl *>(childrenDeclTmp);
    // no sublcass of basic types and children must be a class
    if (parentDecl == NULL || childrenDecl == NULL)
        return false;
    return childrenDecl->IfImplementOrExtend(parentDecl);
}

bool fnMatch(FnDecl *thisFn, FnDecl *otherFn) {
    if (!thisFn)
        return false;
    if (!otherFn)
        return false;

    if (!typeMatchParent(thisFn->returnType, otherFn->returnType))
        return false;
    if (thisFn->formals->NumElements() != otherFn->formals->NumElements())
        return false;
    for (int i = 0; i < thisFn->formals->NumElements(); i++) {
        if (!typeMatchParent(otherFn->formals->Nth(i)->GetType(),
                             thisFn->formals->Nth(i)->GetType()))
            return false;
    }
    return true;
}

bool FnDecl::CheckDeclMatch(Decl *decl) {
    // check if other decl is exactly the same as current one;
    Assert(decl);
    FnDecl *otherFn = dynamic_cast<FnDecl *>(decl);
    if (otherFn == NULL) {
        ReportError::DeclConflict(this, decl);
        return false;
    }
    if (!fnMatch(this, otherFn)) {
        ReportError::OverrideMismatch(this);
        return false;
    }
    // check formals & return type;
    return true;
}

void FnDecl::ResolveConflict(set<Node *> *visited) {
    if (scope == NULL) {
        scope = new StackNode(parent->scope);
    }
    /* printf("fun: %s\n", GetName()); */
    /* printf("%d\n", formals->NumElements()); */
    for (int i = 0; i < formals->NumElements(); i++) {
        VarDecl *decl = formals->Nth(i);
        if (decl) {
            scope->AddSymbol(decl->GetName(), decl);
        }
    }
    if (body)
        body->ResolveConflict(visited);
}

vector<Node *> FnDecl::children() {
    vector<Node *> result = Decl::children();
    result.insert(result.end(), formals->elems.begin(), formals->elems.end());
    result.push_back(returnType);
    result.push_back(body);

    return result;
}
void PrintLocation(VarDecl *var) {
    printf("%s: loc:%d ", var->GetName(), var->location->GetOffset());
    printf("seg: %s\n",
           var->location->GetSegment() == gpRelative ? "global" : "local");
}

void FnDecl::generateLabel() {
    const char *fnName = GetName();
    if (strcmp(fnName, "main") == 0) {
        label = strdup("main");
        CodeGenerator::instance->ifMain = true;
    } else {
        label = getMethodLabel(NULL, fnName);
    }
}

void FnDecl::Emit() {
    int cnt = 0;
    for (int i = 0; i < formals->NumElements(); i++) {
        VarDecl *decl = formals->Nth(i);
        if (decl) {
            int offSet = CodeGenerator::OffsetToFirstParam + (cnt++) * 4;
            decl->location = new Location(fpRelative, offSet, decl->GetName());
        }
    }
    if (scope->parent == StackNode::root) {
        // global
        Assert(label);
        CodeGenerator::instance->GenLabel(label);
    } else {
        // class
        Assert(label);
        CodeGenerator::instance->GenLabel(label);
    }
    BeginFunc *beginFunc = CodeGenerator::instance->GenBeginFunc();
    // CodeGenerator::instance->localVarNum = cnt;
    // printf("%d\n", cnt);
    if (body)
        body->Emit();
    beginFunc->SetFrameSize(4 * CodeGenerator::instance->localVarNum);
    CodeGenerator::instance->GenEndFunc();
}
