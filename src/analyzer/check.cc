/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Checks the C program accordingly to ANSI C.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#ifdef HAVE_REGEX_H
#include <stddef.h> // for redhat machines which otherwise choke on regex.h
#include <regex.h>  // for OSF alphas with buggy GCC installations
#endif
#include <ctype.h>
#include <string.h>
#include "auxilary.h"
#include "cpgm.h"
#include "options.h"

#define debug debugstream_check
/* -dcheck=n controls how much output we get on the debug stream:
   5  All progress.
   4  
   3  Main structures
   2
   1
   0  No debug info. */

static void type_error(Position pos,Type *t1,Type *t2,char const *msg) {
    Diagnostic d(ERROR,pos) ;
    d << msg ;
    t1->show(d.addline());
    if ( t2 )
        t2->show(d.addline());
}

// Make implicit casts explicit
// This function inserts an implicit cast around e2 if its type is not
// already equal to that of t. No error checking is performed; the caller
// must have checked that the implicit cast really *happens* in this
// context.
void force_type(Type *t,Expr*& e) {
    if ( *t == *e->exprType )
        ; // OK
    else
        e = new CastExpr(t,e,e->pos,true);
}

// This function implements the rules given in the Standard, 6.3.16.1
// It is used for simple assignment, initialization, return statements,
// function parameters.
// Generally this function should ignore qualifiers found at the
// top level of the types, since qualifiers are basically properties
// of *lvalues* - they certainly don`t matter at the right-hand
// expression, and on the left hand side they matter only in the
// special case of an assignment operator. The checking of this is
// left to the caller.
// The function does not return a status: it emits an error message
// itself if the conversion is not valid without an explicit cast.
static void assign_cvt (Type* dest, Expr*& e)
{
  Type* src = e->exprType;
  assert(dest!=NULL && src!=NULL);

  // Catch all rule: It is fine if the types are completely the same
  if ( *dest == *src )
      return ;

  // 6.3.16.1 constraint case I
  // - the left operand has qualified or unqualified arithmetic type
  //   and the right has arithmetic type
  if ( dest->isArithmic() && src->isArithmic())
      return;

  // 6.3.16.1 constraint case II
  // - the left operand has a qualified or unqualified version of a
  //   structure or union type compatible with the type of the right
  // (covered by the catch-all rule above)

  // 6.3.16.1 constraint case III and IV
  // - both operands are pointers to qualified or unqualified versions
  //   of compatible types, and the type pointed to by the left has
  //   all the qualifiers of the type pointed to by the right
  // - one operand is pointer to an object or incomplete type and the
  //   other is a pointer to a qualified or unqualified version of
  //   void, and the type pointed to by the left has all the qualifiers
  //   of the type pointed to by the right
  do {
      Type *pdest = dest->isPointer() ;  if ( pdest == NULL ) break ;
      Type *psrc = src->isPointer() ;    if ( psrc  == NULL ) break ;
      if ( ( pdest->isVoid || psrc->isVoid || *pdest == *psrc ) &&
           psrc->qualifiers().subsetof(pdest->qualifiers()) )
          return ;
  } while (0);

  // 6.3.16.1 constraint case V
  // - the left operand is a pointer and the right is a null pointer
  //   constant
  // (null pointer constants are either constant integral expressions or
  // such one cast to void*. The cast variant has already been handled
  // by case IV. We allow arbitrary constant expressions to fit in this
  // rule, but we warn if they are not obviously zero.
  if ( dest->isPointer() && src->isIntegral() && e->isConst ) {
    if ( !e->valueKnown ) {
      Diagnostic d(WARNING,e->pos) ;
      d << "it is not obvious that `" ;
      e->show(d) ;
      d << "' is a null pointer constant" ;
      d.addline() << "it is being implicitly converted to " ;
      dest->show(d);
    }
    if ( !e->valueKnown || e->knownValue == 0 ) {
      // convert it to a special 'null pointer constant' expression
      // which is treated specially in the subsequent analyses
      e = new NullExpr(e->pos);
      e->setRealType(dest) ;
      return;
    }
  }
  
  Diagnostic d(ERROR,e->pos);
  d << "cannot implicitly convert " ;
  src->show(d);
  d.addline() << "to " ;
  dest->show(d);
}

// Force an expression's type while checking if the conversion
// is legal without an explicit cast
static void
checked_force(Type* dest, Expr*& e)
{
    assign_cvt(dest,e);
    force_type(dest,e);
}

// This function does special for expressions that are used as
// conditions or operands to boolean operators: it makes sure
// that the type is primitive of some sort, either by casting
// (for enum types) or by comparing to a null pointer (for
// pointer types).
static void
boolean_operand(Expr*& e)
{
  switch(e->exprType->realtype) {
  case PtrT: {
    Expr *null = new NullExpr(e->pos) ;
    null->setRealType(e->exprType);
    e = new BinaryExpr(e,null,NEq,e->pos);
    e->setRealType(BaseType::get(Int));
    break ; }
  case BaseT:
    break ;
  default:
    checked_force(BaseType::get(Int),e);
    break ;
  }
}

//////////////////////////////////////

// This function creates a type that is equal to t0, but with the
// specified extra qualifiers. If possible, t0 is reused.

static Type*
addqualifiers(Type *t0, constvol cv) {
    assert(t0 != NULL);
    if ( cv.subsetof(t0->qualifiers()) )
        return t0 ;
    t0 = t0->copy() ;
    Type *t2 = t0 ;
    while ( t2->realtype == ArrayT ) {
        t2 = ((ArrayType*)t2)->next ;
        assert( t2 != NULL );
    }
    t2->cv += cv ;
    return t0 ;
}

//////////////////////////////////////

// This function checks an expression in a context where pointer decay
// occurs as specified in 6.2.2.1.
// If the decays it is wrapped in an implicit cast.
// The function always returns the normalized (and possibly decayed)
// type of the expression.
static Type*
decaycheck(Expr *&exp)
{
  // First check the expression by virtual call.
  Type *t = exp->check() ;
  assert ( t );

  // "Array to T" decays to "pointer to T"
  if(t->realtype==ArrayT) {
    Type* cast = new PtrType(((ArrayType*)t)->next);
    // Make the cast silent (it wont show up in the src pgm).
    Expr *newexp = new CastExpr(cast,exp,exp->pos,true);
    exp = newexp ;
    exp->isRval = true ;
    return cast ;
  }
  // Functions decay to pointers to themselves
  else if(t->realtype==FunT) {
    Type* cast = new PtrType(t);
    Expr *newexp = new CastExpr(cast,exp,exp->pos,true);
    exp = newexp ;
    exp->isRval = true ;
    return cast ;
  }
  return t ;
}

//////////////////////////////////////
// CHECKING INITIALIZERS
//
// The check phase also tries to make implicit brace levels
// explicit and to remove redundant braces.

static bool
string_initializer(Init *i,Type *t) {
    if ( i->tag != InitElem ) return false ;
    if ( i->expr()->string_literal() == NULL ) return false ;
    // we know now that the initializer IS a naked string
    // literal. If the type is absent, we're checking inside
    // a sloppy-braced subtree, so the safe thing is to
    // keep it as a string
    if ( t == NULL ) return true ;
    if ( t->realtype != ArrayT ) return false ;
    t = ((ArrayType*)t)->next ;
    if ( t->realtype != BaseT ) return false ;
    switch( ((BaseType*)t)->tag ) {
    case Char:
    case UChar:
        return true ;
    default:
        return false ;
    }
}

// This functions does the most of the transformation from
// uninterpreted initializers to strictly fully/sloppy
// braced initializers that match a specific type.
//   It skips past initializers in a list, enough to
// create an initializer for the specified type. The
// return value is a pointer to the final initializer,
// or NULL, indicating that the list is sloppy-braced.
//   There are two basic modes of operation:
//     master: the list we're removing items from was
//             enclosed in braces that has been identified
//             as referring to type t. NULL is never
//             returned from master mode.
//     slave: no braces for type t has been met yet.

static Init*
split_init_list(Plist<Init>::iterator &i,Type *t, bool master)
{
    if ( !master && i->tag == InitList ) {
        Plist<Init>::iterator I = i->begin() ;
        Init *result = split_init_list(I,t,true);
        if ( result == NULL ) {
            Diagnostic(WARNING,i->pos()) << "ambiguous initializer" ;
            i->sloppy = true ;
            return *i++ ;
        }
        if ( I )
            Diagnostic(ERROR,i->pos()) << "extra elements in initializer" ;
        i++ ;
        return result ;
    }

    switch ( t->realtype ) {
    case FunT:
    case BaseT:
    case PtrT:
    case EnumT:
    case AbstractT:
        break;
    case ArrayT: {
        ArrayType *tt = (ArrayType*)t ;
        // try to see if this is a character array initialization
        // by a string
        if ( string_initializer(*i,tt) )
            return *i++ ;

        unsigned long maxelem ;
        if ( master )
            maxelem = - 1ul ;
        else {
            if ( tt->size != NULL &&
                 tt->size->valueKnown &&
                 tt->size->knownValue > 0 )
              maxelem = tt->size->knownValue ;
            else
              return NULL ;
        }
        Plist<Init> *arrinit = new Plist<Init> ;
        while ( i && maxelem ) {
            Init *eltinit = split_init_list(i,tt->next,false);
            if ( eltinit == NULL ) {
                delete arrinit ;
                return NULL ;
            }
            arrinit->push_back(eltinit);
            maxelem-- ;
        }
        return new Init(arrinit); }
    case StructT: {
        UserDecl *def = ((UserType*)t)->def ;
        Plist<Init> *strinit = new Plist<Init> ;
        for ( Plist<MemberDecl>::iterator mi
                  = ((StructDecl*)def)->member_list->begin();
              i && mi ; mi++ ) {
            Init *eltinit = split_init_list(i,mi->var->type,false);
            if ( eltinit == NULL ) {
                delete strinit ;
                return NULL ;
            }
            strinit->push_back(eltinit);
        }
        return new Init(strinit); }
    case UnionT: {
        UserDecl *def = ((UserType*)t)->def ;
        Plist<MemberDecl>::iterator mi
            = ((StructDecl*)def)->member_list->begin();
        Init *eltinit = split_init_list(i,mi->var->type,false);
        if ( eltinit == NULL )
            return NULL ;
        return new Init(eltinit); }
    }

    if ( i->tag == InitList ) {
        Diagnostic(ERROR,i->pos())
            << "too many brace levels in initializer";
        i->sloppy = true ;
    }
    return *i++ ;
}       

// This function type checks an initializer once it has been
// converted to fully/sloppy braced form. The t argument can
// be NULL which means that we are checking sloppy initializers.

static void
check_initializer(Init *i,Type *t) {
    // special case for string initialization
    if ( string_initializer(i,t) ) {
        i->sloppy = true ;
        // this is later used as a flag in c2core
        return ;
    }

    if ( i->tag == InitElem ) {
        Expr *e = i->expr() ;
        decaycheck(e);
        if ( t )
            checked_force(t,e);
        i->expr(e);
        return ;
    }

    if ( !i->sloppy && t != NULL )
        switch(t->realtype) {
        case ArrayT: {
            ArrayType *tt = (ArrayType*)t ;
            for(Plist<Init>::iterator it = i->begin() ; it ; it++ )
                check_initializer(*it,tt->next);
            return ; }
        case StructT:
        case UnionT: {
            UserDecl *def = ((UserType*)t)->def ;
            Plist<MemberDecl>::iterator mi
                = ((StructDecl*)def)->member_list->begin();
            Plist<Init>::iterator it = i->begin() ;
            for ( ; it && mi ; it++, mi++ )
                check_initializer(*it,mi->var->type);
            return ; }
        default:
            // If this happens an error should already have been
            // reported, so our job is just to recover enough to
            // complete the typecheck. Falling through and pretending
            // the list is sloppy will do.
            break ;
        }

    i->sloppy = true ;
    for(Plist<Init>::iterator it = i->begin() ; it ; it++ )
        check_initializer(*it,NULL);
    return ;
}

unsigned string_literal_length(const char *s) {
    unsigned l = -1u ; // when the two qoutes are added the length of "" is 1
    assert(s != NULL);
    while ( *s ) {
        l++ ;
        if ( *s++ == '\\' )
            switch(*s++) {
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                if ( isdigit(*s) && *s != '8' && *s != '9' ) s++ ;
                if ( isdigit(*s) && *s != '8' && *s != '9' ) s++ ;
                break; // at most 3 octal digits are scanned
            case 'x':
                while ( isxdigit(*s) ) s++ ;
                break ;
            default:
                break ;
            }
    }
    return l ;
}              

//////////////////////////////////////

void
check_program (CProgram* pgm)
{
  if (debug^1) debug << "Checking program\n";
  pgm->check();
}

void
CProgram::check()
{
  // Run through the user types.
  if (debug^3) debug << "\nChecking user types:";
  if (usertypes==NULL) usertypes = new Plist<UserDecl>();
  foreach( first3, *usertypes, Plist<UserDecl> ) {
    if (debug^3) debug << '[' << first3->name << ']';
    first3->check();
  }
  // Run through the global objects.
  if (debug^3) debug << "\nChecking globals:";
  if (definitions==NULL) definitions = new Plist<VarDecl>();
  foreach( first2, *definitions, Plist<VarDecl> ) {
    if (debug^3) debug << '[' << first2->name << ']';
    first2->check();
  }
  // Run through the functions.
  if (debug^3) debug << "\nChecking functions:";
  if (functions==NULL) functions = new Plist<FunDef>();
  foreach( first, *functions, Plist<FunDef> ) {
    if (debug^3) debug << '[' << first->name << ']';
    first->check();
  }
}

// We need to keep track of the return type of a function to be able to check
// return statements.
Type* currentFunRetType = NULL;

Type*
FunDef::check()
{
  if (isChecked()) return type;
  // Check linkage.
  assert(linkage==External || linkage==Internal);
  assert(type->isFun);
  type->check() ;

  // If this is a function prototype it has to be external.
  if (stmts==NULL) {
    assert(decls==NULL);
    type->check() ;
    if (linkage!=External)
      Diagnostic(ERROR,pos) << name << "() never gets a body" ;
  } else {
    FunType *funt = (FunType*)type ;
    #ifndef NDEBUG
    Plist<Type>::iterator type = funt->params->begin() ;
    foreach(var, *decls, Plist<VarDecl>) {
      assert(var->type == *type);
      type++ ;
      assert((*var)->linkage!=Typedef);
      assert((*var)->linkage!=AbstractType);
      assert((*var)->tag==Vardecl);
      assert(var->init==NULL);
    }
    assert( !type );
    #endif
    assert(currentFunRetType==NULL);
    // Global variable used in ReturnStmt::check().
    currentFunRetType = funt->ret;
    stmts->check();
    currentFunRetType = NULL ;
  }
  return type;
}

Type*
VarDecl::check()
{
  if (isChecked()) {
    assert(type!=NULL);
    return type;
  }
  type->check() ;
  if ( init != NULL ) {
      if ( init->tag == InitList ) {
          Plist<Init> pseudolist(init);
          Plist<Init>::iterator i = pseudolist.begin() ;
          init = split_init_list(i,type,false);
      }
      check_initializer(init,type);

      // If an array of unknown type is initialized, its size is determined
      // by the number of initializers provided for its elements. At the
      // end of its initializer list, the array no longer has incomplete
      // type.
      if ( type->realtype == ArrayT && ((ArrayType*)type)->size == NULL ) {
          if ( init->tag == InitList && !init->sloppy )
              ((ArrayType*)type)->size = makeConstExpr(init->size(),pos);
          else if ( init->tag == InitElem && init->sloppy )
              ((ArrayType*)type)->size = makeConstExpr(
                  string_literal_length(init->expr()->string_literal()),pos);
      }
  }
  switch (linkage) {
  case External:
    if ( !is_definition() )
      break ;
  case NoLinkage:
  case Internal:
    if ( !type->isObject() )
      Diagnostic(ERROR,pos) << "definition of object with incomplete type";
    break ;
  case Typedef:
  case AbstractType:
    assert(tag==Vardecl);
    break ;
  }
  return type;
}

///////////////////////////

bool BaseType::isObject() {
    return tag != Void ;
}

void BaseType::check()
{
  isChecked();
  assert(!isFun);  
  // void is not a complete type.
}

bool AbsType::isObject() {
    return true ;
}

void AbsType::check() {
    isChecked();
}

bool FunType::isObject() {
    return false ;
}

void FunType::check()
{
  if (isChecked()) return;
  assert(!(!isFun || isVoid || cv.vol || cv.cst));

  foreach(i,*params,Plist<Type>)
    i->check();
  ret->check() ;
  switch(ret->realtype) {
  case BaseT:
  case PtrT:
  case StructT:
  case UnionT:
  case EnumT:
    break ;
  case AbstractT:
    if ( strchr(((AbsType*)ret)->properties,'p') || ret->isArithmic() )
      break ;
    // else fall through
  case ArrayT:
  case FunT:
    Diagnostic(ERROR,pos) << "function returning " << ret ;
    break ;
  }
}

bool ArrayType::isObject() {
    return size != NULL && next->isObject() ;
}

void ArrayType::check()
{
  if (isChecked()) return;
  assert(!(isFun || isVoid));
  next->check();
  if (size != NULL) {
    Type *stype = decaycheck(size);
    if( !stype->isIntegral() )
      Diagnostic(ERROR,size->pos) << "non-integral array size" ;
  }
}

bool PtrType::isObject() {
    return true ;
}

void PtrType::check()
{
  if (isChecked()) return ;
  assert(!(isFun || isVoid));
  next->check();
}

bool EnumType::isObject() {
    return def->complete() ;
}

void EnumType::check()
{
  if (isChecked()) return ;
  assert(!(isFun || isVoid));  
  def->check() ;
}

bool UnionType::isObject() {
    return def->complete() ;
}

void UnionType::check()
{
  if (isChecked()) return;
  assert(!(isFun || isVoid));  
  def->check() ;
}

bool StructType::isObject() {
    return def->complete() ;
}

void StructType::check()
{
  if (isChecked()) return;
  assert(!(isFun || isVoid));  
  def->check();
}


/////////////////////////////////////

void
EnumDecl::check()
{
  if (isChecked()) return;
  // undeclared enums would have been caught in the parser phase
  assert(!member_list->empty()) ;
  // Check enumeration constants
  foreach(i,*member_list,Plist<VarDecl>)
    i->check() ;
}

bool EnumDecl::complete() {
  assert(!member_list->empty()) ;
  return true ;
}

void
StructDecl::check()
{
  if (isChecked()) return;
  foreach(i,*member_list,Plist<MemberDecl>)
    i->var->check() ;
}

bool StructDecl::complete() {
    return ! member_list->empty() ;
}

///////////////////////////////////////////////////


void
LabelStmt::check()
{
  assert(label != NULL);
  if (debug^5) debug << '[' << label << ']';
  assert(stmt != NULL);
  stmt->check();
}

void
CaseStmt::check()
{
  // The case label has to be a constant expression.
  assert(exp != NULL);
  decaycheck(exp);
  assert(stmt != NULL);
  stmt->check();
  // A case cannot appear outside a switch. This is checked in c2core.cc.
}

void
DefaultStmt::check()
{    
  assert(stmt != NULL);
  stmt->check();
  // A default label cannot appear outside a switch. This is checked in c2core.cc.
}

void
CompoundStmt::check()
{
  foreach(i,*objects,Plist<VarDecl>)
    i->check();
  foreach(i,*stmts,Plist<Stmt>)
    i->check();
}

void
ExprStmt::check()
{    
  assert(exp != NULL);
  decaycheck(exp);
}

void
IfStmt::check()
{
  assert(exp != NULL);
  decaycheck(exp) ;
  boolean_operand(exp);
  thn->check();
  els->check();
}

void
DummyStmt::check()
{
  return;
}

void
SwitchStmt::check()
{
  assert(exp != NULL);
  decaycheck(exp);
  // The expression must have integral type.
  if (!exp->type()->isIntegral())
    Diagnostic(ERROR,exp->pos) << "expression must be of integral type";
  // These checks are not implemented:
  // Check for dead code? (if it must be checked, it should be in c2core)
  // XXX Check that no case values are the same (after conversion) ?
  // Check for at most one default? (checked in c2core)
  stmt->check();
}

void
WhileStmt::check()
{
  assert(exp != NULL);
  decaycheck(exp) ;
  boolean_operand(exp) ;
  stmt->check();
}

void
DoStmt::check()
{
  assert(exp != NULL);
  decaycheck(exp);
  boolean_operand(exp);
  stmt->check();
}

void
ForStmt::check()
{
  // Expression can be NULL.
  if (e1 != NULL) decaycheck(e1);
  if (e2 != NULL) {
    decaycheck(e2);
    boolean_operand(e2);
  }
  if (e3 != NULL) decaycheck(e3);
  stmt->check();
}

void
Indirection::check()
{
  if(stmt == NULL)
    Diagnostic(ERROR,pos) << "label not found in goto statement";
}

void
GotoStmt::check()
{
  assert(ind != NULL);
  ind->check();
}

void
BreakStmt::check()
{
  // May only appear in a switch or loop body: checked in c2core.cc
  return;
}

void
ContStmt::check()
{
  // May only appear in a loop body: checked in c2core.cc
  return;
}

void
ReturnStmt::check()
{
  // Return stmt with expr shall not appear in function returning void.
  assert(currentFunRetType!=NULL);
  if (exp==NULL) {
    if (currentFunRetType->isVoid) return;
    Diagnostic(WARNING,pos) << "return without a value in non-void function";
  }
  else {
    decaycheck(exp);
    checked_force(currentFunRetType,exp);
  }
}


///////////////////////////////////////////////////////////////////
//                 EXPRESSIONS                                   //
///////////////////////////////////////////////////////////////////

// This function is supposed to implement the "usual arithmetic
// conversions" described in the Standard, clause 6.2.1.5
// If the types are not arithmetic, it is assumed that the function
// is called from a conditional expression and the rules in
// clause 6.3.15 are followed.
static Type* arith_cvt (Type* t1, Type * t2)
{
  // We convert enumerations to integers before calculating with them
  if ( t1->realtype == EnumT )
      t1 = BaseType::get(Int);
  if ( t2->realtype == EnumT )
      t2 = BaseType::get(Int);
    
  // Abstract types always win.
  // This is an assumption and not invariably safe, but it is the best
  // approximation we can give. ???
  if ( t1->realtype == AbstractT )
      return t1 ;
  if ( t2->realtype == AbstractT )
      return t2 ;
  
  // If the expressions are of equal type apart from qualifiers,
  // it is OK.
  if ( *t1 == *t2 )
      return t1 ;

  // Cast a "smaller" type to a "larger" if necessary.
  // XXX this algorithm is a hack and its correctness is dubious
  if ( t1->realtype == BaseT && t2->realtype == BaseT ) {
      // Base types. Examine the tags.
      BaseTypeTag tag1 = ((BaseType*)t1)->tag;
      BaseTypeTag tag2 = ((BaseType*)t2)->tag;
      if (tag1 > tag2)
          return t1;
      else if (tag1 < tag2)
          return t2;
      assert(NULL);
  }

  Type *pt1 = t1->isPointer() ;
  Type *pt2 = t2->isPointer() ;
  if ( pt1 && pt2 ) {
      if ( *pt1 == *pt2 ) {
          // clause 6.3.5 .. the result type is a pointer to a type
          // qualified with all the type qualifiers of the types
          // pointed-to by both operands
          return new PtrType(addqualifiers(pt1,pt2->qualifiers())) ;
      }
      // else one of them must be void*
      if ( pt1->isVoid )
          return t2 ;
      if ( pt2->isVoid )
          return t1 ;
  } else if ( t1->isIntegral() )
      return t2 ;
  else if ( t2->isIntegral() )
      return t1 ;

  // Nothing worked. Just return *something* and let checked_force() display
  // an error message later.
  return t1 ;
}

// this funcion returns true if its argument is a pointer to an object type
static bool isObjPtr(Type *t) {
    Type *pt = t->isPointer() ;
    return pt != NULL && pt->isObject() ;
}

Type*
ConstExpr::check()
{
  isChecked();
  assert(literal != NULL);
  assert(exprType!=NULL);
  return exprType;
}

Type*
NullExpr::check()
{
  isChecked();
  assert(exprType!=NULL);
  assert(exprType->isPointer());
  return exprType;
}

Type*
VarExpr::check()
{
  if (isChecked()) return exprType;
  assert(decl != NULL);
  Type* t = decl->type;
  setRealType(t);
  // enumeration and magic constants are rvals
  if ( decl->tag == Vardecl )
      switch( ((VarDecl*)decl)->varmode ) {
      case VarConstant:
      case VarEnum:
          isRval = true ;
          break ;
      case VarMu:
          assert(0) ;
      default:
          isRval = false ;
      }
  else
      isRval = false ; // "&somefunction" is allowed.
  assert(exprType!=NULL);
  return exprType;
}

Type*
CallExpr::check()
{
  if (isChecked()) return exprType;
  assert(fun != NULL);
  // fun should, after decay, result in a pointer to function.
  // (if fun happens to be a function designator, then whatever casts
  // get inserted are going to be ignored by the c2core translation).
  // After the decay the isPointer method gives us the type pointed
  // to (which should be a regular pointer) or NULL if the expression
  // does not have pointer type (which is an error)
  Type* t = decaycheck(fun)->isPointer() ;

  if (t && t->isFun) {
    FunType *const funt = (FunType*)t ;
    // Check arguments against the formal parameters.
    Plist<Type>::iterator firstPar = funt->params->begin() ;
    Plist<Expr>::mutable_iterator firstArg = args->begin();
    while (firstArg) {
      Expr *argument = *firstArg ;
      assert(argument!=NULL);
      decaycheck(argument);
      if( firstPar ) {
        // Check that types agree, convert if necessary.
        checked_force((*firstPar),argument);
        firstPar++;
      }
      firstArg << argument ;
      firstArg++;
    }
    unsigned nparms = funt->params->size();
    if ( !funt->varargs && args->size() > nparms )
      Diagnostic(ERROR,pos) << "too many arguments in function "
                            << "call (expecting " << nparms << ')' ;
    // Check that the function does not expect more arguments.
    if ( firstPar )
      Diagnostic(ERROR,pos) << "function call needs " << nparms
                            << " arguments" ;
    setRealType(funt->ret);
  }
  else {
      type_error(pos,fun->exprType,NULL,
                 "cannot 'call' an expression of this type:");
      setRealType(fun->exprType); // anyting will do
  }
  assert(exprType!=NULL);
  return exprType;
}

Type*
ArrayExpr::check()
{
  if (isChecked()) return exprType;
  assert(left != NULL);
  assert(right != NULL);
  // One of the expressions must have type "pointer to obj" and the
  // other must have integral type. Normalize so the former is the
  // left and the latter is the right expression.
  Type* t1 = decaycheck(left);
  Type* t2 = decaycheck(right);
  Type* t1p = t1->isPointer() ;
  Type* t2p = t2->isPointer() ;
  if ( t1p && t1p->isObject() && t2->isIntegral() ) {
      // Ok, standard order
      setRealType(t1p);
  } else if ( t1->isIntegral() && t2p && t2p->isObject() ) {
      // Swap them
      Expr* tmp = right;
      right = left;
      left = tmp;
      setRealType(t2p);
  } else {
      type_error(pos,t1,t2,"cannot use [] on these two types:");
      setRealType( t1p ? t1p : t2p ? t2p : t1 ); // anything goes!
  }
  assert(exprType!=NULL);
  isRval = false ;
  return exprType;
}

Type*
DotExpr::check()
{
  if (isChecked()) return exprType;
  assert(left!=NULL);
  // Check that the expression is a structure.
  Type* t = decaycheck(left);
  if (t->realtype == StructT || t->realtype == UnionT ) {
    UserDecl* d = ((UserType*)t)->def;
    d->check();
    StructDecl* sd = (StructDecl*)d;
    assert(sd!=NULL);
    // Search for the member.
    memindex = 1 ;
    foreach(it,*sd->member_list,Plist<MemberDecl>) {
      VarDecl* var = it->var;
      assert(var!=NULL);
      if (strcmp(var->name,member)==0) {
        // Member found
        setRealType(addqualifiers(var->type,t->qualifiers()));
        assert(exprType!=NULL);
        isRval = false ;
        return exprType;
      }
      memindex++;
    }
    // The loop should not exit through the main door...
    Diagnostic(ERROR,pos) << member
                          << " is not a member of struct " << sd->name ;
    memindex = 0 ;
  } else
      type_error(pos,t,NULL,"cannot select member from this type:");
  setRealType(t);
  return exprType;
}

Type*
UnaryExpr::check()
{
  if (isChecked()) return exprType;
  assert(exp!=NULL);
  Type* et = NULL;
  switch (op)
    {
    case Addr:
      // The address of something is a (rval) pointer to it.
      et = exp->check(); // just here things don`t decay
      if ( exp->isRval && !et->isFun ) {
          Diagnostic(ERROR,pos) << "the operand to & must be lvalue" ;
      }
      setRealType(new PtrType(et));
      break; 
    case DeRef:
      // et has to be a pointer type.
      et = decaycheck(exp);
      if (1) {
          Type *pt = et->isPointer() ;
          if ( pt )
              setRealType(pt);
          else {
              type_error(exp->pos,pt,NULL,"cannot dereference this type:");
              setRealType(et); // any type will do
          }
      }
      isRval = false ;
      break;
    case Pos:
    case Neg:
    case Not:
      // et has to be an arithmic rval type.
      et = decaycheck(exp);
      if (et->isIntegral() || op != Not && et->isArithmic() )
          switch(et->realtype) {
          case BaseT:
              if (((BaseType*)et)->tag >= Int)
                  break ;
              // else fall through
          case AbstractT:
              // Integral promotion of char, etc.
              force_type(et=BaseType::get(Int),exp);
              break ;
          default:
              // If other arithmetic types are invented, nothing happens
              // to them.
              break ;
          }
      else
          type_error(pos,et,NULL,"operand to unary operatior has wrong type:");
      setRealType(et);
      break;
    case Bang:
      // et has to be a scalar type.
      decaycheck(exp);
      boolean_operand(exp);
      setRealType(BaseType::get(Int));
      break;
    }
  assert(exprType!=NULL);
  return exprType;
}

Type*
PreExpr::check()
{
  if (isChecked()) return exprType;
  assert(exp!=NULL);
  Type* t = decaycheck(exp);
  // The expression has to be an lval.
  if (exp->isRval) 
    Diagnostic(ERROR,exp->pos) << "operand of pre-stepping is not lvalue";
  if (!t->isModifiable() || !isObjPtr(t) && !t->isArithmic())
      type_error(exp->pos,t,NULL,"cannot increment/decrement this type:");
  setRealType(t);
  assert(exprType!=NULL);
  return exprType;
}

Type*
PostExpr::check()
{
  if (isChecked()) return exprType;
  assert(exp!=NULL);
  Type* t = decaycheck(exp);
  // The expression has to be an lval.
  if (exp->isRval) 
    Diagnostic(ERROR,exp->pos) << "operand of post-stepping is not lvalue";
  if (!t->isModifiable() || !isObjPtr(t) && !t->isArithmic())
      type_error(exp->pos,t,NULL,"cannot incrment/decrement this type:");
  setRealType(t);
  assert(exprType!=NULL);
  return exprType;
}

Type*
TypeSize::check()
{
  if (isChecked()) return exprType;
  assert(typesz!=NULL);
  typesz->check();
  if ( !typesz->isObject())
    Diagnostic(ERROR,pos) << "cannot take size of this type";
  setRealType(new AbsType("size_t","i"));
  assert(exprType!=NULL);
  return exprType;
}

Type*
ExprSize::check()
{
  if (isChecked()) return exprType;
  assert(exp!=NULL);
  Type* t = exp->check(); // just here things don''t decay
  if ( !t->isObject())
    Diagnostic(ERROR,exp->pos)
      << "cannot take size this type of expression";
  setRealType(new AbsType("size_t","i"));
  assert(exprType!=NULL);
  return exprType;
}

Type*
CastExpr::check()
{
  if (isChecked()) return exprType;
  assert(exp!=NULL);
  Type* t1 = decaycheck(exp);
  assert(exprType!=NULL);
  Type* t2 = exprType ;
  Type *t1p = t1->isPointer() ;
  Type *t2p = t2->isPointer() ;

  // Casts between equal types are OK
 if ( *t1 == *t2 )
     return exprType;
  
  // anything can be cast to void
  if ( t2->isVoid )
      return exprType;

  // Conversions between arithmetic types are OK
  if ( t1->isArithmic() && t2->isArithmic() )
      return exprType;

  if ( t1p && t2p ) {
      if ( *t1p == *t2p )
          // pointer to almost equal types, perhaps even implicitly legal ;
          return exprType ;
      if ( t1p->isVoid || t2p->isVoid )
          return exprType;
      Diagnostic d(WARNING,pos) ;
      d << "suspicious pointer cast" ;
      t1->show(d.addline() << "(from ");
      t2->show(d.addline() << "to ");
      d << ')' ;
      return exprType;
  }

  if ( t1p && t2->isIntegral() ) {
      Diagnostic(WARNING,pos)
          << "C-Mix doesn't like casts from pointer to integer" ;
      return exprType;
  }
  if ( t2p && t1->isIntegral() ) {
      if ( !exp->valueKnown && exp->knownValue != 0 ) {
          Diagnostic(WARNING,pos)
              << "C-Mix doesn't like casts from integer to pointer" ;
      } else {
          // an integral constant 0 converted to a pointer type is
          // a null pointer constant. We make it one here; an object
          // cannot replace itself with another, but it is enough to
          // make the operand to the cast a 'null' expression - then
          // the cast(s) that enclose it are removed in the c2core phase
          exp = new NullExpr(pos);
          exp->setRealType(exprType);
      }
      return exprType ;
  }

  Diagnostic d(ERROR,pos) ;
  d << "cannot cast between " ;
  t1->show(d) ;
  d << " and " ;
  t2->show(d) ;
  return exprType ;
}

Type*
BinaryExpr::check()
{
  if (isChecked()) return exprType;
  assert(left!=NULL && right!=NULL);
  // Both expressions have to be rvals.
  Type* rhtt = decaycheck(right);
  Type* lftt = decaycheck(left);
  switch (op) {
  case Add:
      if ( lftt->isIntegral() && isObjPtr(rhtt) ) {
          setRealType(rhtt);
          break ;
      }
      goto AddOrSub ;
  case Sub:
      if (1) {
          Type *lfp = lftt->isPointer() ;
          Type *rhp = rhtt->isPointer() ;
          if ( lfp && rhp ) {
              if ( *lfp == *rhp && lfp->isObject() )
                  ; // OK
              else
                  type_error(pos,lftt,rhtt,
                             "cannot subtract these pointer types:");
              setRealType(BaseType::get(LongInt));
                  // but continue as if we could
              break ;
          }
      }
      // else fall through
  AddOrSub:
      if ( isObjPtr(lftt) && rhtt->isIntegral() ) {
          setRealType(lftt);
          break ;
      }
      // else fall through
  case Mul:
  case Div:
      // Both expressions have to be arithmic types.
      if (!(lftt->isArithmic() && rhtt->isArithmic()))
          type_error(pos,lftt,rhtt,"cannot do arithmetic on these types:");
      setRealType(arith_cvt(lftt,rhtt));
      force_type(exprType,left);
      force_type(exprType,right);
      break ;
  case BAnd:
  case BEOr:
  case BOr:
  case Mod:
      if (!(lftt->isIntegral() && rhtt->isIntegral()))
          type_error(pos,lftt,rhtt,op==Mod ?
                     "cannot take modulus with these types:" :
                     "cannot do bitwise operations with these types:");
      setRealType(arith_cvt(lftt,rhtt));
      force_type(exprType,left);
      force_type(exprType,right);
      break ;
  case LShift:
  case RShift:
      if (!(lftt->isIntegral() && rhtt->isIntegral()))
          type_error(pos,lftt,rhtt,"cannot shift bitwise with these types:");
      setRealType(lftt);
      break ;
  case LT:
  case GT:
  case LEq:
  case GEq:
      if (!(lftt->isArithmic() && rhtt->isArithmic()) &&
          !(lftt->isPointer() && rhtt->isPointer() ) ) {
          type_error(pos,lftt,rhtt,"cannot compare these types:");
          setRealType(BaseType::get(Int));
          break ;
      }
      // else fall through
  case Eq:
  case NEq:
      // If the operands are pointers we need to check if they can be
      // made compatible. That is taken care of in checked_force()
      if (1) {
          Type *commontype=arith_cvt(lftt,rhtt);
          checked_force(commontype,left);
          checked_force(commontype,right);
          setRealType(BaseType::get(Int));
      }
      break ;
  case And:
  case Or:
      boolean_operand(left);
      boolean_operand(right);
      setRealType(BaseType::get(Int));
      break;
  }
  assert(exprType != NULL);
  return exprType;
}

Type*
CommaExpr::check()
{
  if (isChecked()) return exprType;
  assert(left!=NULL && right!=NULL);
  decaycheck(left);
  setRealType(decaycheck(right)) ;
  assert(exprType != NULL);
  return exprType;
}

Type*
CondExpr::check()
{
  if (isChecked()) return exprType;
  assert(cond!=NULL && left!=NULL && right!=NULL);
  decaycheck(cond);
  boolean_operand(cond);
  setRealType(arith_cvt(decaycheck(left),decaycheck(right)));
  assert(exprType!=NULL);
  checked_force(exprType,left);
  checked_force(exprType,right);
  return exprType;
}

Type*
AssignExpr::check()
{
  if (isChecked()) return exprType; 
  assert(left!=NULL && right!=NULL);
  Type* lftt = decaycheck(left);
  Type* rhtt = decaycheck(right);
  if (left->isRval)
    Diagnostic(ERROR,left->pos)
        << "left operand to assignment is not lvalue";
  if (!lftt->isModifiable())
    Diagnostic(ERROR,left->pos)
        << "left operand to assignment is not modifiable";
  switch (op) {
  case AddAsgn: // these two can be pointer arithmetic; so they are
  case SubAsgn: // treated specially
      if ( isObjPtr(lftt) && rhtt->isIntegral() )
          break ; // pointer arithmetic does not need cast
      // else fall through to the other arithmetic operators
  case MulAsgn:
  case DivAsgn:
  case ModAsgn:
      if (lftt->isArithmic() && rhtt->isArithmic())
          force_type(lftt,right);
      else
          type_error(pos,lftt,rhtt,"cannot do arithmetic on these types:");
      break ;
  case LSAsgn:
  case RSAsgn:
      if (lftt->isIntegral() && rhtt->isIntegral())
          ; // no cast takes place
      else
          type_error(pos,lftt,rhtt,"cannot use <<= or >>= on these types:");
      break;
  case AndAsgn:
  case EOrAsgn:
  case OrAsgn:
      if (lftt->isIntegral() && rhtt->isIntegral())
          force_type(lftt,right); // to possibly insert implicit cast
      else
          type_error(pos,lftt,rhtt,"cannot do bitwise logic on these types:");
      break ;
  case Asgn:
      checked_force(lftt,right);
      break ;
  }
  setRealType(lftt);
  return exprType;
}

///////////////// LVALUES ////////////////////////

bool StructType::isModifiable()
{
  assert(def!=NULL);
  StructDecl* strct = (StructDecl*)def;
  assert(strct!=NULL);
  if (cv.cst) return false ;
  if (!isObject())  return false;
  Plist<MemberDecl>::iterator first = strct->member_list->begin();
  Plist<MemberDecl>::iterator last  = strct->member_list->end();
  while (first!=last) {
    if (!(*first)->var->type->isModifiable()) return false;
    first++;
  }
  return true;
  // This value could be cached!
}

bool UnionType::isModifiable()
{
  assert(def!=NULL);
  StructDecl* strct = (StructDecl*)def;
  assert(strct!=NULL);
  if (cv.cst) return false ;
  if (!isObject())  return false;
  Plist<MemberDecl>::iterator first = strct->member_list->begin();
  Plist<MemberDecl>::iterator last  = strct->member_list->end();
  while (first!=last) {
    if (!(*first)->var->type->isModifiable()) return false;
    first++;
  }
  return true;
  // This value could be cached!
}



