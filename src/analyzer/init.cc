/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Initializer finishing in Core C
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <limits.h>
#include "analyses.h"

//*****************************************************************************
//*****************************************************************************
// PHASE I. Locate candidates for initializer shadowing.
// Candidates are those local variables that have proper initializers [1]
// and are also subject to other side effects. [2]
//
// Output: only candidates will still have a nonzero init_where pointer.
//
// [1] "improper" initializers initialize noncompound objects. For local
//     variables, they are always converted to assignments in the c2core
//     phase.
// [2] It is safe to only initialize an object once if there are never
//     explicit side effects to (parts of) it:
//       Clause 6.5.7 constraint III:
//         All the expressions in an initializer for an object that
//         has static storage duration or in an initializer list for
//         an object that has aggregate or union type shall be
//         constant expressions.
//       Clause 6.4 Semantics III ff:
//         More latitude is permitted for constant expressions in
//         initializers. Such a constant expression shall evaluate
//         to one of the following:
//          - an arithmetic constant expression
//          - a null pointer constant
//          - an address constant
//          - an address constant for an object type plus or minus
//            an integral constant expression
//     and neither of these (as witnessed by the rest of clause 6.4)
//     can access the value of objects.

static void
FindCandidates(C_Pgm &pgm,PAresult const &pa)
{
  ALocSet candidates ;
  foreach(fun,pgm.functions,Plist<C_Decl>)
    foreach(var,fun->fun_locals(),Plist<C_Decl>)
    if (var->hasInit()) {
      if ( var->init_where() == NULL ) {
        Diagnostic(WARNING,var->pos)
          << "initialization for " << var->get_name()
          << "never gets executed" ;
        var->init(NULL);
      } else
        candidates.insert(*var);
    }

  // Now we have the set of current candidates. Those candidates
  // that are never side effected do not need to be evacuated.

  foreach(c,candidates,ALocSet)
    if ( pa.sideEffectedObjects.find(*c) )
      (void)0 ; // do nothing now; later it will be evacuated
    else {
      // this variable is never side effected; we can pull it out
      // and make it a global variable [which will prevent it from
      // being memoized].
      C_Decl *fun = c->containedIn() ;
      assert(fun && fun->tag == FunDf );
      Plist<C_Decl>::mutable_iterator i = fun->fun_locals().begin() ;
      while ( *i != *c ) i++ ;
      fun->fun_locals().erase(i) ;

      c->containedIn(NULL);
      c->init_where(NULL);
      pgm.globals.push_back(*c);
    }
}

//*****************************************************************************
//*****************************************************************************
// PHASE II. Wrap any array candidates in a struct
// This is necessary because arrays cannot be assigned, but it is also
// inherently messy because we have to patch up any C_Var expressions
// that refers to the variable into a C_Member(C_Var(..)) construction.
// Luckily any such expressions must be inside the same function as
// the array itself.

static C_Type*
create_new_struct_wrap(C_Type *t,const char *thename,Position pos)
{
  C_UserDef *ud = new C_UserDef(UINT_MAX,thename,pos,false);
  ud->names.push_back(new C_UserMemb("a"));
  C_Type* thetype = new C_Type(ud);
  thetype->user_types().push_back(t);
  return thetype ;
}

// Wrap an existing declaration in a fresh structure type
C_Decl::C_Decl(C_Decl *v)
  : Numbered(Numbered::C_DECL), tag(VarDcl), name(v->name),
    type(create_new_struct_wrap(v->type,v->name,v->pos))
{
  assert(v->tag == VarDcl);
  var.isIn = v->var.isIn ; v->var.isIn = this ;
  if ( v->var.init ) {
    var.init = new C_Init(FullyBraced, new Plist<C_Init>(v->var.init)) ;
    v->var.init = NULL ;
    var.initWhere = v->var.initWhere ; v->var.initWhere = NULL ;
  } else {
    var.init = NULL ;
    var.initWhere = NULL ;
  }
  var.varmode = v->var.varmode ;
  var.varmodeWhy.set(v->var.varmodeWhy);
  membs.push_back(v);
}

void
C_Expr::add_member_level()
{
  assert(tag == C_Var);
  C_Decl* d2 = var()->containedIn() ;
  assert(d2->type->tag == StructUnion);
  assert(d2->type->user_types().front() == type->ptr_next());
  tag = C_Member ;
  subexpr1 = new C_Expr(d2,pos);
  member_nr = 1 ;
}

static void wraptraverse(C_Decl*,PAresult&);
static void wraptraverse(C_Expr*,PAresult&);
static void wraptraverse(C_Type*,PAresult&);

static void
WrapArrays(C_Decl* fun,PAresult &pa,Plist<C_UserDef> &usertypes)
{
  ALocSet wrapped ;

  for ( Plist<C_Decl>::mutable_iterator v = fun->fun_locals().begin(); v; v++)
    if ( v->hasInit() && v->init_where() && v->type->tag == Array ) {
      wrapped.insert(*v);
      v << new C_Decl(*v);
      usertypes.push_back(v->type->user_def());
      // temporarily use the PA array to store a reference to the
      // new wrapper object. We'll use it when redirecting expressions.
      pa.PTvar[*v] = new ALocSet(*v);
    }

  if ( !wrapped.empty() ) {
    wraptraverse(fun,pa);

    // restore proper points-to sets
    foreach(w,wrapped,ALocSet) {
      assert(pa.PTvar[*w]->empty());
      pa.PTvar[w->containedIn()] = pa.PTvar[*w] ;
    }
  }
}

static Pset<C_Type> stoptyperec ;

static void
wraptraverse(C_Decl* fun,PAresult &pa)
{
  foreach(v,fun->fun_locals(),Plist<C_Decl>) {
    wraptraverse(v->type,pa);
    stoptyperec.clear() ;
    if ( v->hasInit() ) {
      Plist<C_Init> worklist(v->init()) ;
      while(!worklist.empty()) {
        C_Init *i = worklist.back() ;
        worklist.pop_back() ;
        switch(i->tag) {
        case Simple:
          wraptraverse(i->simple_init(),pa);
          stoptyperec.clear() ;
          break ;
        case FullyBraced:
        case SloppyBraced: {
          foreach(i2,i->braced_init(),Plist<C_Init>)
            worklist.push_back(*i2);
          break ; }
        case StringInit:
          break ;
        }
      }
    }
  }
  foreach(bb,fun->blocks(),Plist<C_BasicBlock>) {
    foreach(s,bb->getBlock(),Plist<C_Stmt>) {
      if (s->hasTarget())
        wraptraverse(s->target(),pa);
      switch(s->tag) {
      case C_Assign:
        wraptraverse(s->assign_expr(),pa);
        break ;
      case C_Call: {
        wraptraverse(s->call_expr(),pa);
        foreach(a,s->call_args(),Plist<C_Expr>)
          wraptraverse(*a,pa);
        break ; }
      case C_Alloc:
        wraptraverse(s->alloc_objects()->type,pa);
        break ;
      case C_Free:
        wraptraverse(s->free_expr(),pa);
        break ;
      case C_Sequence:
        break ;
      }
      stoptyperec.clear() ;
    }
    switch(bb->exit()->tag) {
    case C_Goto:
      break ;
    case C_If:
      wraptraverse(bb->exit()->cond_expr(),pa);
      break ;
    case C_Return:
      if ( bb->exit()->hasExpr() )
        wraptraverse(bb->exit()->return_expr(),pa);
      break ;
    }
    stoptyperec.clear() ;
  }
}

static void
wraptraverse(C_Type* t,PAresult &pa)
{
  switch(t->tag) {
  case Primitive: case Abstract: case EnumType:
    break;
  case FunPtr: case Pointer:
    wraptraverse(t->ptr_next(),pa);
    break;
  case Array:
    if ( t->hasSize() )
      wraptraverse(t->array_size(),pa);
    wraptraverse(t->array_next(),pa);
    break;
  case Function: {
    foreach(arg,t->fun_params(),Plist<C_Type>)
      wraptraverse(*arg,pa);
    wraptraverse(t->fun_ret(),pa);
    break; }
  case StructUnion:
    if ( stoptyperec.insert(t) )
      return ;
    foreach(member,t->user_types(),Plist<C_Type>)
      wraptraverse(*member,pa);
    break;
  }
}

static void
wraptraverse(C_Expr* e,PAresult &pa)
{
  switch (e->tag) {
  case C_Cnst: case C_Null: case C_Cast:
    wraptraverse(e->type,pa);
    break;
  case C_EnumCnst: case C_Unary: case C_PtrCmp: case C_Binary: case C_SizeofT:
  case C_SizeofE:
    // these own their type too, but the type never contains expressions
    break ;
  case C_FunAdr: case C_Member: case C_DeRef: case C_PtrArith: case C_Array:
  case C_ExtFun:
    break; // these share their types
  case C_Var:
    C_Decl* v = e->var() ;
    if ( !v->isContained() || v->containedIn()->tag != VarDcl )
      return ;
    // A C_Var that points to a subsidiary must mean one that
    // has just been converted to a struct.
    e->add_member_level();
    C_Expr* e2 = e->subexpr() ;
    pa.PT[e2] = pa.PTvar[e2->var()] ;
    assert(pa.PT[e2]);
    return ;
  }
  // then traverse subexpressions
  switch(e->tag) {
  case C_EnumCnst: case C_Cnst: case C_Null: case C_Var:
  case C_FunAdr: case C_ExtFun:
    break ;
  case C_Member: case C_DeRef: case C_Unary: case C_Array: case C_Cast:
  case C_SizeofE:
    wraptraverse(e->subexpr(),pa);
    break ;
  case C_PtrArith: case C_PtrCmp: case C_Binary:
    wraptraverse(e->binary_expr1(),pa);
    wraptraverse(e->binary_expr2(),pa);
    break ;
  case C_SizeofT:
    wraptraverse(e->sizeof_type(),pa);
    break ;
  }
}

static void
WrapArrays(C_Pgm &pgm,PAresult &pa)
{
  foreach(fun,pgm.functions,Plist<C_Decl>)
    WrapArrays(*fun,pa,pgm.usertypes);
}

//*****************************************************************************
//*****************************************************************************
// PHASE III. Actually do the initializer shadowing.
//
// This is relatively uncomplicated. Most of the work goes with maintaining
// the points-to information

static void
pacpy(C_Decl *tgt, C_Decl *src, PAresult &pa)
{
  pa.PTvar[tgt] = pa.PTvar[src] ;
  double_foreach(t,s,tgt->members(),src->members(),Plist<C_Decl>)
    pacpy(*t,*s,pa);
}

static void
ShadowInitializers(C_Pgm &pgm,PAresult &pa)
{
  foreach(fun,pgm.functions,Plist<C_Decl>) {
    foreach(v,fun->fun_locals(),Plist<C_Decl>)
      if ( v->hasInit() && v->init_where() != NULL ) {
        // Make the shadow copy and transfer the initializer
        C_Decl *shadow = new C_Decl(VarDcl,v->type->copy(),v->get_name());
        pacpy(shadow,*v,pa);
        C_BasicBlock *thebb = v->init_where() ;
        shadow->init(v->init()); v->init(NULL);
                
        // Make the assignment
        C_Expr *e = new C_Expr(shadow,v->pos);
        pa.PT[e] = new ALocSet(shadow);
        e = new C_Expr(C_DeRef,e,v->pos);
        pa.PT[e] = new ALocSet();
        C_Stmt *assgn = new C_Stmt(C_Assign,e,v->pos);
        e = new C_Expr(*v,v->pos);
        pa.PT[e] = new ALocSet(*v);
        assgn->target(e);
        thebb->getBlock().push_front(assgn);
                
        // Insert the new shadow in the globals list
        pgm.globals.push_back(shadow);
      }
  }
}

//*****************************************************************************
//*****************************************************************************
// PUTTING IT ALL TOGETHER

void
SubstInitializers(C_Pgm &pgm, PAresult &pa)
{
  FindCandidates(pgm,pa);
  WrapArrays(pgm,pa);
  ShadowInitializers(pgm,pa);
}

