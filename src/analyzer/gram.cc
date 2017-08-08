/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Namespace and scope utilities for the parser.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "options.h"
#include "liststack.h"
#include "parser.h"
#include "symboltable.h"
#include <strstream.h>

#define debug debugstream_gram

//////// NAMESPACES ////////
// ANSI C sec. 6.1.2.3 :

//////////// Namespace 1
SymbolTable<Indirection> goto_labels;
// Goto labels (we only need one table, since labels are in scope of the entire
// function they appear in) 

/////////// Namespace 2
Scope<ObjectDecl> names;
// Names of Function definitions, Variables, Enum constants, Function
// declarations and Typedefs.

ListStack<VarDecl> objects;
// List of real objects.

static Plist<VarDecl>* enumconsts = new Plist<VarDecl>();
// List of enumconsts in the enum being declared.

static Plist<FunDef>* functions = new Plist<FunDef>();

/////////// Namespace 3
static Scope<UserDecl> usernames;
// Names of structs/unions/enums.

static ListStack<UserDecl> userstack;
// Stack of user declarations; needed to make user declarations in
// parameter lists visible in the function body.

static Plist<UserDecl> userdecls;

/////////// Namespace 4
static Scope<MemberDecl> membernames;
// Members of the struct or union type currently being declared. Several levels
// are needed because of embedded declarations.

static ListStack<MemberDecl> members;
// List of members.

//////// namespaces end ////////

static void do_parameter_decay(Plist<VarDecl> *);
static void type_mismatch(char const*, Type*, Position, Type*, Position);
static void external_linker(ObjectDecl* ,bool is_defn);

struct Declaration {
  char const* const name ;
  Type *const type ;
  Position const pos ;
  Parse_Storage storage ;

  Declaration(Parse_GeneralType *gt, Parse_Declarator *dcl)
    : name(dcl->id), type(dcl->maketype(gt)), pos(dcl->pos),
      storage(gt->storage) {
    assert(name != NULL );
    delete dcl ;
  }
};

static FunDef *current_function = NULL ;
// set by begin_FunDef, used by end_FunDef

static bool old_function = false ;

static int last_decl_can_have_init = 0;
// Controlled by add_declaration() and used by add_initializer().

static bool
equal_constants(Expr *e1, Expr *e2)
{
  if ( e1->valueKnown && e2->valueKnown )
    return e1->knownValue == e2->knownValue ;
  Diagnostic di(WARNING,e1->pos);
  di << "assuming that `" ;
  e1->show(di);
  di << '\'' ;
  di.addline(e2->pos) << "and `" ;
  e2->show(di);
  di << "' will evaluate to the same constant" ;
  return true ;
}

// Convert both types to their common "composite type" (ISO-C 6.1.2.6).
// During this unifications, usertypes may be collapsed.
// return true if the unification succeded, false if it failed.
static bool
unify_types(Type* t1, Type* t2)
{
  assert(t1 && t2);
  // The types have to have the same qualifiers.
  if (t1->cv.cst != t2->cv.cst || t1->cv.vol != t2->cv.vol)
    return false ;
  // Compare the types.
  if (t1->realtype != t2->realtype)
    return false ;
  switch (t1->realtype) {
  case BaseT: {
    BaseType* b1 = (BaseType*) t1;
    BaseType* b2 = (BaseType*) t2;
    return b1->tag == b2->tag ; }
  case PtrT: {
    PtrType* p1 = (PtrType*) t1;
    PtrType* p2 = (PtrType*) t2;
    return unify_types(p1->next,p2->next); }
  case ArrayT: {
    ArrayType* p1 = (ArrayType*) t1;
    ArrayType* p2 = (ArrayType*) t2;
    if (p1->size==NULL)
      p1->size = p2->size ;
    else if (p2->size==NULL)
      p2->size = p1->size ;
    else {
      if ( !equal_constants(p1->size,p2->size) )
        return false ;
    }
    return unify_types(p1->next,p2->next); }
  case FunT: {
    FunType* f1 = (FunType*) t1;
    FunType* f2 = (FunType*) t2;
    // If one of the parameter lists is unspecified, use the other one.
    if ( f2->unspecified() ) {
      f2->params->clear() ;
      f2->params->splice(f2->params->begin(),*f1->params);
      f2->varargs = f1->varargs ;
    } else if ( f1->unspecified() ) {
      f1->params->clear() ;
      f1->params->splice(f1->params->begin(),*f2->params);
      f1->varargs = f2->varargs ;
    } else {
      if ( f1->params->size() != f2->params->size() )
        return false ;
      if ( f1->varargs != f2->varargs )
        return false ;
      double_foreach(p1, p2, *f1->params, *f2->params, Plist<Type>) {
        if ( !unify_types(*p1,*p2) )
          return false ;
      }
    }
    return unify_types(f1->ret,f2->ret); }
  case StructT:
  case UnionT:
  case EnumT: {
    UserDecl* d1 = ((UserType*) t1)->def;
    while ( d1->refer_to != NULL ) d1 = d1->refer_to ;
    UserDecl* d2 = ((UserType*) t2)->def;
    while ( d2->refer_to != NULL ) d2 = d2->refer_to ;

    if ( d1 == d2 )
      return true ;
    assert(d1->tag == d2->tag );

    if ( d1->tag != Enum && ((StructDecl*)d1)->member_list->empty() ) {
      // We do not complete one struct with the member types of
      // another, lest we import member names from a different
      // file and later meet different names in this file.
      // We do, however, make sure that as soon as we meet some
      // definition *with* member types, that is made the top
      // of the definition group, so that all subsequent
      // unifications get a chance at matching the members.
      UserDecl *dt = d1 ; d1 = d2 ; d2 = dt ;
    }
    // Remember to redirect one of the types BEFORE unifying subtypes!
    d2->refer_to = d1 ;
    
    if ( d1->tag != Enum ) {
      StructDecl *s1 = (StructDecl*) d1 ;
      StructDecl *s2 = (StructDecl*) d2 ;
      if( !s2->member_list->empty() ) {
        assert( !s1->member_list->empty() );
	if ( s1->member_list->size() != s2->member_list->size() )
          return false ;
        // XXX: The order of the members of unions should be irrelevant,
        // but we require them to be identical, anyway.
        double_foreach(m1,m2,*s1->member_list,*s2->member_list,
                       Plist<MemberDecl>) {
          if ( m1->bitfield ) {
            if ( m2->bitfield ) {
              if ( !equal_constants(m1->bitfield,m2->bitfield) )
                return false ;
            } else
              return false ;
          } else {
            if ( m2->bitfield )
              return false ;
            else
              ;
          }
          if ( !unify_types(m1->var->type,m2->var->type) )
            return false ;
        }
      }
    } else {
      // unifying two enum definitions
      EnumDecl *e1 = (EnumDecl*) d1 ;
      EnumDecl *e2 = (EnumDecl*) d2 ;
      if ( e1->member_list->size() != e2->member_list->size() )
        return false ;
      double_foreach(m1,m2,*e1->member_list,*e2->member_list,
                     Plist<VarDecl>) {
        m2->refer_to = *m1 ;
        if ( m1->init == NULL )
          m1->init = m2->init ;
        else if ( m2->init == NULL )
          m2->init = m1->init ;
        else {
          assert(m1->init->isSimple());
          assert(m2->init->isSimple());
          if ( !equal_constants(m1->init->expr(),m2->init->expr()) )
            return false ;
        }
      }
    }
    return true ; }
  case AbstractT: {
    AbsType *a1 = (AbsType*) t1 ;
    AbsType *a2 = (AbsType*) t2 ;
    return ( a1->name == a2->name || strcmp(a1->name,a2->name) == 0 ); }
  }
  Diagnostic(INTERNAL,ccchere)
      << "fell through switch in unify_types()" ;
  return false ;
}

void
cccParser :: lone_sue(Parse_UserType* d)
{
  // a lone "enum foobar;" is syntactically allowed, just as
  // "unsigned long;" is, but neither has any meaning.
  if (d->type != Enum)
    introduce_struct_decl(d->type,d->id);
}

static UserDecl *
make_or_reuse_struct(UserDecl *old,UserTag type, char const *name)
{
  assert(type != Enum);
  if ( old != NULL ) {
    if ( old->tag == type )
      return old ;
    else {
      Diagnostic d(ERROR,ccchere) ;
      d << name << " is a " << usertype2str(old->tag) << ", not a "
        << usertype2str(type) ;
      d.addline(old->pos) << "(declaration of " << usertype2str(old->tag)
                          << ' ' << old->name << ')' ;
    }
  }
  StructDecl* d = new StructDecl(name,ccchere,type,NULL);
  userstack.push_back(d);
  userdecls.push_back(d);
  usernames.insert(name, d);
  return d ;
}

// This is meant to introduce struct/union names. Updates of members are done
// later on by update_struct_decl.
void
cccParser::introduce_struct_decl(UserTag type, char const* name)
{
  // Check for _local_ name clashes.
  make_or_reuse_struct(usernames.lookup_local(name),type,name);
}

void
cccParser::find_struct_decl(UserTag type, char const* name)
{
  // If a structure of this name does exists, use that one (if it matches).
  make_or_reuse_struct(usernames.lookup(name),type,name);
}

// This can only be called after introduce_struct().
void
cccParser :: update_struct_decl(char const* name)
{
  // A previous declaration has to be present in local scope.
  UserDecl* oldDecl = usernames.lookup_local(name);
  assert(oldDecl!=NULL);
  StructDecl* old = (StructDecl*) oldDecl;
  if ( !old->member_list->empty() ) {
    Diagnostic d(ERROR,ccchere);
    d << "redeclaration of struct or union " << name ;
    d.addline(old->pos) << "previous declaration here";
  } else {
    Plist<MemberDecl>* memberlst = members.get_list();
    delete old->member_list ;
    old->member_list=memberlst;
    old->assign_defseqnum() ;
    if (debug^4) debug << "[updating: " << name << "]";
  }
}

char const *
cccParser::fresh_anonymous_name()
{
  static unsigned long counter = 0 ;
  char buffer[11] ;
  sprintf(buffer,"$%lu",++counter);
  return stringDup(buffer);
}

void
cccParser::anonymous_struct_decl(UserTag type, char const* name)
{
  if (debug^4) debug << "[anonymous: " << name << "]";
  StructDecl* d = new StructDecl(name,ccchere,type,members.get_list());
  d->assign_defseqnum();
  userdecls.push_back(d);
  usernames.insert(name, d);
}

void
cccParser::add_enum_decl(char const* name)
{
  // Check for name clashes.
  UserDecl* oldDecl = usernames.lookup_local(name);
  if (oldDecl) {
    Diagnostic di(ERROR,ccchere);
    di << usertype2str(oldDecl->tag) << ' ' << name << " redeclared as enum" ;
    di.addline(oldDecl->pos) << "(previous declaration here)";
  }
  // Insert the enum.
  EnumDecl* d = new EnumDecl(name,ccchere,enumconsts);
  userstack.push_back(d);
  userdecls.push_back(d);
  usernames.insert(name,d);
  // "Clear" the global list of enum constants.
  enumconsts = new Plist<VarDecl>();
  if (debug^4) debug << "[add_enum_decl: " << name << "]";
}

void
cccParser :: find_enum_decl(char const* name)
{
  UserDecl* oldDecl = usernames.lookup(name);
  if (!oldDecl)
    Diagnostic(ERROR,ccchere) << "enum " << name << " undeclared" ;
  else if(oldDecl->tag != Enum )
    Diagnostic(ERROR,ccchere)
      << name << " is a " << usertype2str(oldDecl->tag) << ", not an enum" ;
}


///////////////////////////////////////////////


Type*
Parse_Type::gettype()
{
  BaseTypeTag tag = determine_type();
  BaseType *result = new BaseType(tag);
  result->cv = qualifiers;
  return result ;
}

Type*
Parse_TypedefType::gettype()
{
  // Put the qualifiers at the end of an array chain.
  Type* handle = type->type->copy();
  Type* t = handle ;
  while (t->realtype == ArrayT) t = ((ArrayType*)t)->next;
  if ( t->realtype == FunT ) {
    Diagnostic(WARNING,type->pos)
      << "ignoring qualifiers on funcion type (through typedef)" ;
    return handle ;
  }
  // Check for multiple qualifiers.
  if ( qualifiers.cst && t->cv.cst )
    Diagnostic(WARNING,type->pos)
      << "multiple const qualifiers when using this typedef";
  if ( qualifiers.vol && t->cv.vol )
    Diagnostic(WARNING,type->pos)
      << "multiple volatile qualifiers when using this typedef";
  t->cv += qualifiers ;
  return handle;
}

Type*
Parse_UserType::gettype()
{
  UserDecl* def = usernames.lookup(id);
  UserType* result ;
  assert(def != NULL);
  switch (type) {
  case Struct: result = new StructType(def); break;
  case Union:  result = new UnionType(def); break;
  case Enum:   result = new EnumType(def); break;
  default: 
    Diagnostic(INTERNAL,RIGHT_HERE) << "Parse_UserType::gettype";
    return NULL;
  }
  result->cv = qualifiers ;
  return result ;
}

void
cccParser::enter_scope (void)
{
  if (debug^5) debug << "[enter scope]";
  names.enter_scope();
  objects.enter();
  usernames.enter_scope();
  userstack.enter();
}

void
cccParser::leave_scope (void)
{
  names.leave_scope();
  objects.leave();

  if (debug^5) debug << "[leave scope]";
  
  usernames.leave_scope();
  userstack.leave();
}

Plist<Parse_Typemod>*
cccParser::leave_parameter_scope(bool varargs)
{
  Plist<VarDecl> *params = objects.get_list() ;
  Plist<UserDecl> *userdefs = userstack.get_list() ;
  leave_scope() ;
  
  // A single anonymous void parameter is removed, unless there are varargs
  if( !varargs && params->size() == 1 && params->front()->type->isVoid ) {
    delete params->front() ;
    params->pop_front() ;
  }
  do_parameter_decay(params);
  
  // Warn about usertypes declared here
  foreach(i,*userdefs,Plist<UserDecl>) {
    Diagnostic(WARNING,i->pos) << usertype2str(i->tag) << " " << i->name
                               << " declared in parameter list" ;
  }

  return new Plist<Parse_Typemod>
    (new Parse_Funtype(params,varargs,userdefs,ccchere));
}

void
cccParser::enter_old_parameter_scope()
{
  enter_scope() ;
  old_function = true ;
}

void
cccParser::leave_old_parameter_scope(Parse_Typemod *tm)
{
  Parse_Funtype *fun = tm->me_as_function() ;
  assert(fun);

  // move the usertype declarations (there are none in
  // an old-style parameter list)
  assert(fun->usrdecls->empty());
  delete fun->usrdecls ;
  fun->usrdecls = userstack.get_list();
  
  // produce the final parameter declaration list
  Plist<VarDecl> *result = new Plist<VarDecl> ;
  Plist<VarDecl> *typed = objects.get_list() ;
  do_parameter_decay(typed);
  foreach(iname,*fun->params,Plist<VarDecl>) {
    for ( Plist<VarDecl>::mutable_iterator ityped = typed->begin() ;
          true ; ityped++ ) {
      if( !ityped ) {
        result->push_back(*iname);
        break ;
      }
      if( strcmp(ityped->name,iname->name) == 0 ) {
        result->push_back(*ityped);
        delete *iname ;
        typed->erase(ityped);
        break ;
      }
    }
  }

  // complain about parameters that have not yet been moved to
  // the final parameter list
  // Add them to the parameters anyway, so the lack of them won't
  // cause trouble during the rest of the parsing phase.
  foreach(i,*typed,Plist<VarDecl>) {
    Diagnostic(ERROR,i->pos) << i->name
                             << " is not mentioned in the parameter list" ;
    result->push_back(*i);
  }

  // clean up and return
  delete typed ;
  delete fun->params ;
  fun->params = result ;

  old_function = false ;
  leave_scope();
}

static void
do_parameter_decay(Plist<VarDecl> *parlist)
{
  foreach(i,*parlist,Plist<VarDecl>) {
    Type *t = i->type ;
    switch(t->realtype) {
    case ArrayT:
      i->type = new PtrType(((ArrayType*)t)->next) ;
      break ;
    case FunT:
      i->type = new PtrType(t);
      break ;
    default:
      break ;
    }
  }
}

void
cccParser::members_enter_scope (void)
{
  membernames.enter_scope();
  members.enter();
}

void
cccParser::members_leave_scope (void)
{
  membernames.leave_scope();
  members.leave();
}


Parse_TypedefType*
cccParser::make_typedef (const char* name)
{
  VarDecl* tdef = (VarDecl*) names.lookup(name);
  assert(tdef != NULL);
  return new Parse_TypedefType(tdef);
}

void
cccParser::add_member(Parse_GeneralType *gt, Parse_MemberId *mid)
{
  if( mid->p_id == NULL )
    mid->p_id = new Parse_Declarator(fresh_anonymous_name(),ccchere);
  const Declaration d(gt,mid->p_id);
  // Check that the name was not seen before.
  if (membernames.lookup_local(d.name)) {
    Diagnostic(ERROR,ccchere) << "redeclaration of member " << d.name ;
  }
  VarDecl* var = new VarDecl(d.name,d.pos,d.type,NoLinkage,VarMu);
  MemberDecl* decl = new MemberDecl(var,mid->bits);
  // Make the name known.
  membernames.insert(d.name, decl) ;
  // Put the actual definition in the back of the list of members. This list
  // is collected in update_struct_decl() and anonymous_struct_decl(). 
  members.push_back(decl); 
  if (debug^4) {
    debug << "add_member: ";
    decl->show(debug);
    debug << "\n";
  }
  delete mid;
}

// Construct an enum constant and put it in current scope.
VarDecl*
cccParser::add_enumconst (char* n, Position pos, Expr *value)
{
  assert(n != NULL);
  // try to construct a default value if one is not given
  if ( value == NULL ) {
    if ( enumconsts->empty() )
      value = new ConstExpr("0",Integer,pos);
    else if ( enumconsts->back()->init &&
              enumconsts->back()->init->expr()->valueKnown ) {
      // Only generate an enumeration value if we could evaluate
      // the previous one. We do not trust our own constant folding
      // abilities absolutely, so we like to keep the original
      // expression in the new value specification.
      // However, if that would result in an unevaluatable expression
      // we would gain little and subject the poor user to numerous
      // "assuming this and that evaluates to the same" warnings
      // when unifying such enum definitions.
      value = enumconsts->back()->init->expr()->copy_plusone() ;
    }
  }
  // An enum constant is always an int.
  VarDecl* decl = new VarDecl(n,pos,BaseType::get(Int),NoLinkage,VarEnum);
  if ( value )
    decl->set_initializer(new Init(value));
  if (debug^5) {
    debug << "add_enumconst: ";
    decl->show(debug);
    debug << "\n";
  }
  // Check that the name was not seen before.
  ObjectDecl* old = names.lookup_local(n);
  if (old) {
    // Typedefs are allowed to be overwritten (which sucks).
    // Abstract types are usually typedef in Real Life
    if (old->linkage != Typedef &&
        old->linkage != AbstractType)
      Diagnostic(ERROR,ccchere) << n << " redeclared as enum constant";
  }
  names.insert(n, decl) ;
  enumconsts->push_back(decl);
  return decl;
}

static bool
findPrototype(Linkage link, char const* id, Type* type, Position pos)
{
  ObjectDecl *proto = names.lookup(id) ;
  if( proto == NULL )
    return false ;

  if ( link == Internal && proto->linkage == External ) {
    Diagnostic d(ERROR,pos);
    d << id << " redeclared as static" ;
    d.addline(proto->pos) << "(previous declaration here)" ;
  }

  // Check that the declarations match.
  if (!unify_types(type,proto->type)) {
    type_mismatch(id,proto->type,proto->pos,type,pos);
    return false ;
  }
  assert(proto->tag==Fundef);
  current_function = (FunDef*) proto;
  if (current_function->stmts != NULL) {
    Diagnostic d(ERROR,pos);
    d << "redefinition of function " << id;
    d.addline(current_function->pos) << "(previous definition here)" ;
    return false ;
  }
  
  // Use the existing object but transfer the type and position
  current_function->type = type;
  current_function->pos = pos;
  assert(current_function->decls == NULL);

  // if this prototype is not the lead element in the union, promote it
  if( current_function->refer_to != NULL ) {
    current_function->refer_to = NULL ;
    external_linker(current_function,true);
    assert(current_function->refer_to == NULL);
  }
  return true ;
}

void
cccParser::begin_FunDef(Parse_GeneralType *gt, Parse_Declarator *name)
{
  assert(current_function == NULL);

  // Make sure the type of the declaration is a function, and
  // extract the function modifier
  Parse_Funtype *funmod = NULL ;
  if( !name->typemods.empty() )
    funmod = name->typemods.back()->me_as_function() ;
  if( funmod == NULL ) {
    Diagnostic err(ERROR,name->pos);
    err << name->id
        << " is not a function, so it can't have a body" ;
    funmod = new Parse_Funtype(new Plist<VarDecl>(),false,
                               new Plist<UserDecl>,name->pos);
    name->typemods.push_back(funmod);
  }
  funmod->pos = name->pos ;

  // The function head "foo()" has been parsed as "foo(...)" because
  // that is what that kind of prototypes mean. It is different for
  // function *definitions* -- adjust.
  if ( funmod->params->empty() )
    funmod->varargs = false ;

  // Create the function's type
  Type* t = name->maketype(gt);

  // Select the linkage
  Linkage link = External;
  switch (gt->storage) {
  case P_NoStorageClass:
  case P_Extern:          link=External; break;
  case P_Static:          link=Internal; break;
  case P_Typedef:         
  case P_Register:
  case P_Auto:
    Diagnostic(ERROR,name->pos) << name->id <<
      "() has a bad storage class specifier" ;
  }

  // Create a FunDef for the new function, or reuse a prototype
  if( findPrototype(link,name->id,t,name->pos) ) {
    current_function->decls = funmod->params ;
  } else {
    current_function
      = new FunDef(name->id,name->pos,t,link,funmod->params);
    names.insert(name->id,current_function);
    functions->push_back(current_function);
    if( current_function->linkage == External )
      external_linker(current_function,true);
  }

  // Open a scope for the function body, and insert the parameters into
  // it (checking that they all have names and none of them have
  // initializers)
  enter_scope();
  bool metAnonymous = false ;
  foreach(i,*funmod->params,Plist<VarDecl>) {
    if( i->init )
      Diagnostic(ERROR,i->pos) << "Parameter " << i->name
                               << " cannot have initializer" ;
    if( i->name[0] == '$' )
      metAnonymous = true ;
    else
      names.insert(i->name,*i);
  }
  if( metAnonymous )
    Diagnostic(WARNING,name->pos)
      << "Standard C does not allow anonymous parameters" ;
  foreach(i,*funmod->usrdecls,Plist<UserDecl>)
    usernames.insert(i->name,*i);

  // protect the parameters from being deallocated
  name->typemods.pop_back();
  funmod->params = NULL ;
  delete funmod ;
  delete name ;
  delete gt ;
}

void
cccParser::end_FunDef(Stmt* stmt)
{
  assert(current_function != NULL);
  current_function->stmts = stmt ;
  leave_scope();
  goto_labels.clear();
  current_function = NULL ;
}

void
cccParser::add_Label (LabelStmt* stmt)
{
  Indirection* i = goto_labels.lookup(stmt->label);
  if (i==NULL) {
    // The label has not been mentioned before. Insert it into
    // the symboltable, pointing to the right statement.
    goto_labels.insert(stmt->label,new Indirection(stmt->pos,stmt));
  }
  else {
    // The label has been seen before. Check whether it was a
    // real declaration or just a forward reference.
    if (i->stmt == NULL) {
      // Backpatch with the real statement.
      i->stmt = stmt;
    }
    else
      Diagnostic(ERROR,ccchere) << "redeclaration of label "
                                << stmt->label ;
  }
}

Indirection*
cccParser::add_goto (char* label, Position p)
{
  Indirection* i = goto_labels.lookup(label);
  if (i==NULL) {
    // The label has not been mentioned before. Insert it into
    // the symboltable, pointing to nothing.
    Indirection* ind = new Indirection(p, NULL);
    goto_labels.insert(label,ind);
    return ind;
  }
  else {
    // The label has been seen before.
    return i;
  }
}

static void
add_noclass (Declaration const& d)
{
  // this is only used for *local* noclass definitions of *objects*.
  VarDecl* decl = new VarDecl(d.name, d.pos,d.type,NoLinkage,VarIntAuto);
  if (debug^4) {
    debug << "add_noclass: ";
    decl->show(debug);
    debug << "\n";
  }
  names.insert(d.name, decl);
  objects.push_back(decl);
}

static void
external_linker(ObjectDecl *d,bool is_defn)
{
  static SymbolTable<ObjectDecl> externals;

  assert(d->linkage==External);
  ObjectDecl *prev = externals.lookup(d->name) ;
  if ( prev == NULL ) {
    externals.insert(d->name,d);
    return ;
  }
  assert(prev->refer_to == NULL);
  assert(d->refer_to == NULL);
  if ( is_defn ) {
    if ( prev->is_definition() ) {
      Diagnostic di(ERROR,d->pos) ;
      di << "multiple definitions of " << d->name ;
      di.addline(prev->pos) << "(previous definition here)" ;
      return ;
    } else {
      // this is a definition; prev is not. Swap them
      ObjectDecl *temp = prev ; prev = d ; d = temp ;
      externals.remove(prev->name);
      externals.insert(prev->name,prev);
    }
  }
  if ( !unify_types(prev->type,d->type) )
    type_mismatch(d->name,prev->type,prev->pos,d->type,d->pos) ;
  d->refer_to = prev ;
}

static void
add_extern(Declaration const& d, bool tentative_defn)
{
  ObjectDecl* ex = names.lookup_global(d.name);
  if (ex != NULL) {
    // If there is a visible file scope definition we use it and
    // inherit its linkage.
    // Check that the declarations match.
    if ( !unify_types(ex->type,d.type) ) {
      type_mismatch(d.name,ex->type,ex->pos,d.type,d.pos);
      return ;
    }
    // if this is a tentative definition (which only occurs
    // for redeclarations with file scope) and the declaration
    // we already know is not a definition, we make this
    // definition the main one.
    if ( tentative_defn ) {
      assert(ex->tag==Vardecl) ;
      VarDecl *v = (VarDecl*) ex ;
      if ( v->varmode == VarExtDefault ) {
        v->varmode = VarIntAuto ;
        v->pos = ccchere ;
      }
    }
    if (debug^5) {
      debug << "add_extern: using existing object for " << d.name << endl ;
    }
    // Insert a placeholder (pointing to the global).
    names.insert(d.name,ex);
  } else {
    // There is no visible global definition for this. Make a new
    // one; with external linkage.
    ObjectDecl* decl;
    // Function prototypes should eventually be a real function,
    // so they are inserted as a function.
    if (d.type->isFun) {
      FunDef* fun = new FunDef(d.name, d.pos,d.type,External,NULL);
      functions->push_back(fun);
      decl = fun;
    } else {
      VariableMode thevarmode = tentative_defn ? VarIntAuto : VarExtDefault ;
      VarDecl* var
        = new VarDecl(d.name, d.pos,d.type,External,thevarmode);
      objects.push_back_global(var);
      decl = var;
    }
    if (debug^4) {
      debug << "add_extern: ";
      decl->show(debug);
      debug << "\n";
    }
    names.insert(d.name, decl);
    external_linker(decl,tentative_defn);
  }
}

static void
add_static(Declaration const& d)
{
  ObjectDecl* decl;
  if (d.type->isFun) {
    FunDef* fun = new FunDef(d.name,d.pos,d.type,Internal,NULL);
    functions->push_back(fun);
    decl = fun;
  } else {
    VarDecl* var = new VarDecl(d.name,d.pos,d.type,Internal,VarIntAuto);
    var->static_local = true ;
    objects.push_back(var);
    decl = var;
  }
  if (debug^4) {
    debug << "add_static: ";
    decl->show(debug);
    debug << "\n";
  }
  names.insert(d.name, decl);
}

// Construct a final declaration, check the integrity of it,
// and put it in current scope.
void
cccParser::add_declaration (Parse_GeneralType *gt, Parse_Declarator *dcl)
{
  // the name can be NULL if the "declaration" is a prototype parameter
  if( dcl->id == NULL )
    dcl->id = fresh_anonymous_name();

  Declaration d(gt,dcl);
  
  last_decl_can_have_init = 1;
  if( old_function ) {
    last_decl_can_have_init = false ;
    if( d.storage != P_Register && d.storage != P_NoStorageClass )
      Diagnostic(ERROR,ccchere)
        << "storage classes are not allowed for parameters" ;
  }

  // Find a previous declaration if such exists.
  ObjectDecl* oldDecl = names.lookup_local(d.name);
  // If we're at anything but file scope, it is not allowed to
  // redeclare a name.
  if ( names.get_level() > 0 && oldDecl ) {
    Diagnostic di(ERROR,ccchere) ;
    di << "redeclaration of " << d.name ;
    di.addline(oldDecl->pos) << "(previous declaration here)"  ;
    return ;
  }

  if (d.type != NULL && d.type->isFun) {
    last_decl_can_have_init = 0;
    // Function prototypes in non-global scope can only have extern or no
    // storage specifier.
    if ( names.get_level() > 0 && d.storage != P_Extern ) {
      Diagnostic(ERROR,ccchere) << "prototype can only have external linkage";
      d.storage = P_Extern ;
    }
  }
  // If the declaration is extern or static we put it in the
  // global list of declarations. 
  switch (d.storage) {
  case P_Extern:
    add_extern(d,false);
    break;
  case P_Static:
    add_static(d);
    break;
  case P_Typedef:
    last_decl_can_have_init = 0;
    if (oldDecl)
      Diagnostic(ERROR,ccchere) << "redeclaration of " << d.name;
    else {
      VarDecl *decl = new VarDecl(d.name, d.pos,d.type,Typedef,VarMu);
      if (debug^4) {
        debug << "add_typedef: ";
        decl->show(debug);
        debug << "\n";
      }
      names.insert(d.name, decl);
      objects.push_back(decl);
    }
    break;
  case P_Register: // We dont care about register/auto.
  case P_Auto:
    if (names.get_level() == 0)
      Diagnostic(ERROR,ccchere) << "register or auto specified for "
                                   "top-level declaration " << d.name ;
    add_noclass(d);
    break;
  case P_NoStorageClass:
    // external declarations with no explicit storage class are
    // considered external. When an object is declared, the
    // declaration counts as a (tentative) definition; see
    // the Standard, 6.7.2
    if ( names.get_level()==0 ) {
      // NULL types should occur only in old-style parameter lists.
      assert(d.type != NULL);
      add_extern(d,!d.type->isFun);
    } else
      add_noclass(d);
    break;
  default:
    Diagnostic(INTERNAL,d.pos) << "unknown storage class";
  }
}

void
cccParser :: add_initializer (Init* init)
{
  if ( init == NULL )
    return ;
  ObjectDecl* lastOne = names.last_added();
  if (!last_decl_can_have_init) {
    Diagnostic(ERROR,ccchere) << "initializer not allowed here";
    return ;
  }
  assert(lastOne->tag == Vardecl);
  lastOne->set_initializer(init);
  // now the declaration is a definition. If it has external linkage
  // we need to make it the main copy of the definition.
  // We can see that by the refer_to pointer which is NULL unless
  // the external linker has been around.
  if ( lastOne->refer_to != NULL ) {
    // the external_linker will do the job for us if we temporarily
    // untie this declaration from the tree.
    lastOne->refer_to = NULL ;
    external_linker(lastOne,true);
  }
}

static void
type_mismatch(char const *n,
              Type *t1,Position p1,
              Type *t2, Position p2)
{
  Diagnostic d(ERROR,Position());
  d << "conflicting types given for " << n << ':' ;
  t1->show(d.addline(p1));
  t2->show(d.addline(p2));
}

VarExpr*
cccParser :: add_var_ref (char* name, Position pos)
{
  ObjectDecl* d = names.lookup(name);
  if (d==NULL) {
    Diagnostic(ERROR,ccchere) << "unknown variable " << name ;
    // XXX this should be an implicit function declaration ...
    d = new VarDecl(name,pos,BaseType::get(Int),NoLinkage,VarMu);
  }
  return new VarExpr(d,pos);
}

void
cccParser :: clean_up_between_files()
{
  names.restart();
  usernames.restart();
  membernames.restart();
}

CProgram*
cccParser :: get_program()
{
  CProgram* pgm = new CProgram;
  pgm->functions = functions;
  pgm->definitions = objects.get_list();
  pgm->usertypes = &userdecls ;
  return pgm;
}
