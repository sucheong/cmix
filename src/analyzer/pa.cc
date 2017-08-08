/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Pointer analysis of Core C.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "diagnostic.h"
#include "analyses.h"
#include "corec.h"
#include "options.h"
#include "strings.h"
#include "typearr.h"
#include "fixiter.h"
#include "pa.h"
#include "directives.h"

#define padebug (debugstream_pa)

PAresult::PAresult()
  : PT(NULL), PTvar(NULL),
    ExtfunReading(NULL), ExtfunWriting(NULL),
    TrulyLocals(NULL)
{
}

struct PA_Set: public Pset<C_Decl>, public Numbered {
  PA_Set();
};

/* ////////////////////////////// Pools //////////////////////////////////// */

class MultiPointsTo ;

class ALocPool : public typearr<C_Decl> {
  MultiPointsTo &owner ;

  virtual C_Decl* create(C_Type *t)
    { return new C_Decl(ExtState,t,"static extern"); }
  virtual void created(C_Type *t,C_Decl *d);
public:
  ALocPool(MultiPointsTo &o): owner(o) {}
};

class MultiPointsTo : public typearr<PA_Set> {
public:
  ALocPool decls ;
  ALocSet *collected ;

  Plist<MultiPointsTo> SideEffectsFrom ;
  bool PointsToItself ;
  
  MultiPointsTo(bool global);
  virtual PA_Set* create(C_Type*) { return new PA_Set; }
  virtual void created(C_Type *,PA_Set *);
};

/* ////////////////////////////// Constraints ////////////////////////////// */

// Successor lists for aloc sets: If, say, the alloc set s gets updated, the
// successor map tells us which constraints that depend on this alloc set (and
// should thus be fired).
class PAconstraint ;
typedef Pset<PAconstraint> PAconstraintSet;
typedef SafePArray<array<PA_Set,PAconstraintSet*>,PAconstraintSet>
           PAconstraintSuccArray;
// Container that holds all constraints (for debugging)
PAconstraintSet* constraintPool = NULL;  
  
class PAEnv {
  C_Decl* currentFun;              // Function being processed
  array<C_Decl,PA_Set*> PTdecl ;
  array<C_Expr,PA_Set*> PTexpr ;
  PAconstraintSuccArray alocSucc;  // Maps aloc sets to constraints.
  static PA_Set *guarded(PA_Set* &var) {
    if ( var == NULL )
      var = new PA_Set ;
    return var ;
  }
  array<C_Decl,PA_Set*> singleton_arr ;
public:
  Plist<C_Expr> freeExpr;          // Freed objects (in form of expressions).
  FixpointSolver PAsolver;         // Holds the constraints.
  // ^^ should be moved to private data after debugging.

  // global_pool maps ExtFun and ExtPool declarations to the respective
  // global pointer pools
  array<C_Decl,MultiPointsTo*> global_pool;
  // local_pool maps ExtFun declarations to the respective local pointer
  // pools. For SERWState calls this is the same as the global pool.
  array<C_Decl,MultiPointsTo*> local_pool ;

  Plist<PA_Set> pa_set_list ;
  Plist<MultiPointsTo> pool_list ; 
  
  // This set is used for expressions that never point to anything.
  PA_Set *PointsToNothing ;
  
  bool addConstraintAllowed ;
  
  PAEnv(PAresult& res)
    : currentFun(NULL), PTdecl(NULL), PTexpr(NULL),
      singleton_arr(NULL),
      global_pool(NULL), local_pool(NULL),
      PointsToNothing(NULL),
      addConstraintAllowed(true)
    {}
  inline void setCurrentFun(C_Decl* f) { currentFun = f; }
  inline C_Decl* current() { return currentFun;}
  void addAssignConstraint(PA_Set *t,C_Expr *);
  void addConstraint(PAconstraint* fpv);
  inline PA_Set* alocset(C_Expr *e) { return guarded(PTexpr[e]); }
  inline PA_Set* alocset(C_Decl *d) { return guarded(PTdecl[d]); }
  inline void alocset(C_Expr *e,PA_Set *s) {
    assert(PTexpr[e]==NULL);
    PTexpr[e] = s ;
  }
  inline void alocset(C_Decl *d,PA_Set *s) {
    assert(PTdecl[d]==NULL);
    PTdecl[d] = s ;
  }
  inline PA_Set *singleton(C_Decl *d) {
    PA_Set* r = singleton_arr[d] ;
    if ( r == NULL ) {
      r = new PA_Set;
      r->insert(d);
      singleton_arr[d] = r ;
    }
    return r ;
  }
  inline PAconstraintSet& succ(PA_Set const& s) { return *alocSucc[s]; }
  inline PAconstraintSet& succ(PA_Set const* s) { return *alocSucc[s]; }
  inline void solve() { PAsolver.solve(); }
};

// The global environment
PAEnv* env;

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Create pools for external functions
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

static void
makepools(C_Pgm const &pgm)
{
  MultiPointsTo *static_pool = new MultiPointsTo(true) ;
  MultiPointsTo *dynamic_pool = new MultiPointsTo(true) ;

  // first create the pools themselves
  foreach(ef,pgm.exfuns,Plist<C_Decl>) {
    MultiPointsTo *global_pool ;
    if ( ef->calltime() && ef->calltime()->time == CTSpectime )
      global_pool = static_pool ;
    else
      global_pool = dynamic_pool ;

    SideEffects const se = ef->effects() ? ef->effects()->state : SERWState ;
    
    MultiPointsTo *local_pool ;
    switch(se) {
    case SERWState:
      local_pool = global_pool ;
      // side effects are implicit for local pools.
      break ;
    case SEROState:
      local_pool = new MultiPointsTo(false);
      local_pool->SideEffectsFrom.push_back(local_pool);
      local_pool->SideEffectsFrom.push_back(global_pool);
      break ;
    case SEStateless:
      local_pool = new MultiPointsTo(false);
      local_pool->SideEffectsFrom.push_back(local_pool);
      global_pool = local_pool ;
      break ;
    case SEPure:
      local_pool = new MultiPointsTo(false);
      global_pool = local_pool ;
      break ;
    }
    env->global_pool[*ef] = global_pool ;
    env->local_pool[*ef] = local_pool ;
  }

  // then seed the pools with the addresses of global variables
  foreach(gv,pgm.globals,Plist<C_Decl>) {
    switch(gv->varmode()) {
    case VarIntAuto:
    case VarIntResidual:
    case VarIntSpectime:
      break ;
    case VarExtSpectime:
    case VarVisSpectime:
      (*static_pool)[gv->type]->insert(*gv);
      break ;
    case VarExtDefault:
    case VarExtResidual:
    case VarVisResidual:
      (*dynamic_pool)[gv->type]->insert(*gv);
      break ;
    case VarEnum:
    case VarConstant:
    case VarMu:
      assert(0);
    }
  }
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Auxiliary constraint functions
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

PAconstraint::PAconstraint()
{
  if(constraintPool)
    constraintPool->insert(this);
}

PAconstraint::~PAconstraint()
{
  if(constraintPool)
    constraintPool->erase(this);
}

void
PAconstraint::depend_on(PA_Set const*set)
{
  env->succ(set).insert(this);
}

Pset<PA_Set> PAconstraint::changedsets ;

void
PAconstraint::Solve(FixpointSolver &fps)
{
  changedsets.clear() ;
  solve() ;
  foreach(set,changedsets,Pset<PA_Set>)
    foreach(i,env->succ(**set),Pset<PAconstraint>)
    fps += *i ;
  changedsets.clear() ;
}

// The fundamental operation is to tranfer an object d to another set S:
// [ S <= d ].
void
PAconstraint::insertOne(PA_Set *to, C_Decl* d) {
  if (!to->insert(d))
    changedsets.insert(to) ;
}
  
void
PAconstraint::insertMany(PA_Set *to, PA_Set const* from) {
  bool changed = false;
  depend_on(from);
  foreach( i, *from, PA_Set) {
    if ( !to->insert(*i) )
      changed = true ;
  }
  if ( changed )
    changedsets.insert(to) ;
}

// Transfer a set of objects from one decl to another.
void
PAconstraint::assignmentTransfer( C_Decl* target, C_Decl* source )
{
  // Recursively traverse structures.
  if (target->type->tag == StructUnion || target->type->tag == Array) { 
    // Tranfer the member's PA info.
    double_foreach( tm, sm,
                    target->members(), source->members(),
                    Plist<C_Decl> )
      assignmentTransfer( *tm, *sm );
  } else {
    // Non-structure objects: just transfer the PA info.
    insertMany( env->alocset(target), env->alocset(source) );
  }
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Constraint classes
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

// *t = s
class AssignConstraint1 : public PAconstraint
{
  PA_Set* target;
  PA_Set* source;
public:
  AssignConstraint1(PA_Set* t, PA_Set* s) : target(t), source(s) {
    assert(target && source);
    if ( padebug^4 ) padebug << '[' << (void*)this << ":*" << (void*)t
                             << '=' << (void*)s << ']' << endl ;
    depend_on(target);
  } 
  virtual void solve() {
    // depen_on target has been set up by the constructor
    foreach(d, *target, PA_Set) {
      if (padebug^3) {
        padebug << '[';
        if (padebug^4) padebug << (void*)this << ":";
        padebug << **d << " <- " << *source;
      }
      PA_Set* PTd = env->alocset(*d);
      insertMany(PTd,source);
      if (padebug^3) padebug  << ']' << endl ;
    }
  }
};

// *t = *s
class AssignConstraint2 : public PAconstraint
{
  PA_Set* target;
  PA_Set* source;
public:
  AssignConstraint2(PA_Set* t, PA_Set* s) : target(t), source(s) {
    assert(target && source);
    if ( padebug^4 ) padebug << '[' << (void*)this << ":*" << (void*)t
                             << "=*" << (void*)s << ']' << endl ;
    depend_on(target);
    depend_on(source);
  } 
  virtual void solve() {
    // depend_on target has been set up by the constructor
    foreach(d, *target, PA_Set) {
      if (padebug^3) {
        padebug << '[';
        if (padebug^4) padebug << (void*)this << ":";
        padebug << **d << " <- ";
      }
      // depend_on source has been set up by the constructor
      foreach( s, *source, PA_Set )
        assignmentTransfer( *d, *s );
      if (padebug^3) padebug  << ']' << endl;
    }
  }
};

void
PAEnv::addAssignConstraint(PA_Set *target,C_Expr *e)
{
  switch(e->type->tag) {
  case Pointer: case FunPtr:
    addConstraint(new AssignConstraint1(target,alocset(e)));
    break ;
  case StructUnion:
    assert(e->tag == C_DeRef);
    addConstraint(new AssignConstraint2(target,alocset(e->subexpr())));
    break ;
  default:
    break ; // no pointers here
  }
}

// t subset of s
class SubsetConstraint : public PAconstraint
{
  PA_Set* target ;
  PA_Set* source ;
public:
  SubsetConstraint(PA_Set *t, PA_Set *s) : target(t), source(s) {
    assert(target && source);
    if ( padebug^4 ) padebug << '[' << (void*)this << ':' << (void*)t
                             << '=' << (void*)s << ']' << endl ;
  }
  virtual void solve() {
    insertMany(target,source);
  }
};

// t == *s
class DerefConstraint : public PAconstraint
{
  PA_Set* target;
  PA_Set* source;
public:
  DerefConstraint(PA_Set* t, PA_Set* s) : target(t), source(s) {
    assert(t && s);
    if ( padebug^4 ) padebug << "[" << (void*)this << ":"
                             << (void*)t << " is *" << (void*)s;
    // If the subexpression's alocset changes, fire this constraint.
    if ( padebug^5 ) padebug << "<-" << (void*)s;
    depend_on(s);
    if ( padebug^4 ) padebug << "]" << endl ;
  } 
  virtual void solve() {
    if (padebug^3) {
      padebug << "[";
      if (padebug^4) padebug << (void*)this << ":";
      padebug  << "* <- ";
    }
    // depend_on(source) set up by constructor
    foreach(dd, *source, PA_Set)
      insertMany( target, env->alocset(*dd) );
    if (padebug^3) padebug  << ']' << endl;
  }
};

// t == &(s->i)
class MemberConstraint : public PAconstraint
{
  PA_Set* target;
  PA_Set* source;
  unsigned member;
public:
  MemberConstraint(PA_Set* t, PA_Set* s, unsigned n)
    : target(t), source(s), member(n) {
    assert(t && s);
    if ( padebug^4 ) padebug << "[" << (void*)this << ":" << (void*)t <<
                       " is " << (void*)s << "." << n;
    // If the subexpression's alocset changes, fire this constraint.
    if ( padebug^5 ) padebug << "<-" << (void*)s;
    depend_on(s);
    if ( padebug^4 ) padebug << "]" << endl ;
  } 
  virtual void solve() {
    if (padebug^3) {
      padebug  << "[";
      if (padebug^4) padebug << (void*)this << ":";
      padebug << "." << member << " <- ";
    }
    // depend_on(source) set up by constructor
    foreach(dd, *source, PA_Set)
      insertOne( target, dd->getMember(member) );
    if (padebug^3) padebug  << ']' << endl;
  }
};

// t == &s[0]
class ArrayConstraint : public PAconstraint
{
  PA_Set* target;
  PA_Set* source;
public:
  ArrayConstraint(PA_Set* t, PA_Set* s) : target(t), source(s) {
    assert(t && s);
    if ( padebug^4 ) padebug << "[" << (void*)this << ":" << (void*)t <<
                       " is " << (void*)s;
    // If the subexpression's alocset changes, fire this constraint.
    if ( padebug^5 ) padebug << "<-" << (void*)source;
    depend_on(source);
    if ( padebug^4 ) padebug << "]" << endl ;
  } 
  virtual void solve() {
    if (padebug^3) {
      padebug << "[";
      if (padebug^4) padebug << (void*)this << ":";
      padebug << " [0] <- ";
    }
    // depend_on(source) set up by constructor
    foreach(dd, *source, PA_Set) {              // source:{a,b}
      // Get the pseudo-constents of the array.
      assert( dd->type->tag == Array );
      insertOne( target, dd->members().front() );
    }
    if (padebug^3) padebug  << ']' << endl;
  }
};

static void
return_from_extfun(PA_Set *target, C_Type *rettype, MultiPointsTo *pool)
{
  if ( target == env->PointsToNothing )
    return ;
  switch( rettype->tag ) {
  case Pointer:
  case FunPtr: {
    PA_Set *poolpart = (*pool)[rettype->ptr_next()] ;
    env->addConstraint(new AssignConstraint1(target,poolpart));
    break ; }
  case StructUnion: {
    PA_Set *poolpart = env->singleton(pool->decls[rettype]);
    env->addConstraint(new AssignConstraint2(target,poolpart));
    break ; }
  default:
    break ; // no pointers in there
  }
}

// t = f(exp,exp,exp...)
class CallConstraint : public PAconstraint
{
  PA_Set* target;
  PA_Set* function;
  Plist<C_Expr> &args;
  
  Pset<C_Decl> cache;
public:
  CallConstraint(PA_Set *t,PA_Set *f, Plist<C_Expr> &a)
    : target(t), function(f), args(a) {
    assert(t && f);
    if ( padebug^4 ) padebug << '[' << (void*)this << ":" << (void*)t
                             << '=' << (void*)f << "()" ;
    depend_on(function);
    if ( padebug^4 ) padebug << ']' << endl ;
  }
  virtual void solve() {
    // depend_on(function) has been set up by the constructor
    foreach(fun,*function, PA_Set)
      if( !cache.insert(*fun) ) {
        if ( padebug^3 ) padebug << '{' << (void*)this << " may call "
                                 << **fun << endl ;
        // the function has not been seen before. Add subsidiary constraints
        // according to whether it is internal or external
        switch(fun->tag) {
        case VarDcl:
          assert(0);
        case FunDf: {
          // return value
          if ( padebug^4 ) padebug << "return: " ;
          switch( fun->type->fun_ret()->tag ) {
          case StructUnion:
            env->addConstraint( new AssignConstraint2(target,
                                                      env->alocset(*fun) ));
            break ;
          case Pointer:
          case FunPtr:
            env->addConstraint( new AssignConstraint1(target,
                                                      env->alocset(*fun) ));
            break ;
          default:
            break ; // no pointers in there
          }
          // each of the arguments
          Plist<C_Expr>::iterator ai = args.begin();
          foreach(pi,fun->fun_params(),Plist<C_Decl>) {
            if ( padebug^4 ) padebug << "arg: " ;
            env->addAssignConstraint(env->singleton(*pi),*ai);
            ai++ ;
          }
          break ; }
        case ExtFun: {
          MultiPointsTo *global_pool = env->global_pool[*fun] ;
          MultiPointsTo *local_pool = env->local_pool[*fun] ;
          assert( global_pool && local_pool );
          assert( fun->type->tag == FunPtr );
          // return value
          if ( padebug^4 ) padebug << "return: " ;
          C_Type *rettype = fun->type->ptr_next()->fun_ret() ;
          return_from_extfun(target,rettype,global_pool);
          if ( global_pool != local_pool )
            return_from_extfun(target,rettype,local_pool);
          
          // each of the arguments
          foreach(ai,args,Plist<C_Expr>) {
            if ( padebug^4 ) padebug << "arg: " ;
            switch( ai->type->tag ) {
            case Pointer:
            case FunPtr: {
              PA_Set *poolpart = (*local_pool)[ai->type->ptr_next()] ;
              PA_Set *ptarg = env->alocset(*ai);
              env->addConstraint(new SubsetConstraint(poolpart,ptarg));
              break ; }
            case StructUnion: {
              assert( ai->tag == C_DeRef );
              PA_Set *poolpart = env->singleton(local_pool->decls[ai->type]);
              PA_Set *ptarg = env->alocset(ai->subexpr());
              env->addConstraint(new AssignConstraint2(poolpart,ptarg));
              break ; }
            default:
              break ; // no pointers here
            }
          }
          break; }
        case ExtState:
          // XXX what do we do if external functions return function pointers?
          break ;
        }
        if ( padebug^3 ) padebug << '}' << endl ;
      }
  }
};

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Pool construction & Maintenance
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

PA_Set::PA_Set() : Numbered(Numbered::PA_SET)
{
  env->pa_set_list.push_back(this);
}

void
ALocPool::created(C_Type *t,C_Decl *d)
{
  switch(t->tag) {
  case Pointer:
  case FunPtr:
    env->alocset(d,owner[t->ptr_next()]);
    return ;
  case Array:
    d->subsidiary( (*this)[t->array_next()] );
    break ;
  case StructUnion: {
    foreach(i,t->user_types(),Plist<C_Type>)
      d->subsidiary( (*this)[*i] );
    break ; }
  case Function:
    assert(0);
  case Abstract:
  case Primitive:
  case Enum:
    break ;
  }
  env->alocset(d,env->PointsToNothing);
}

MultiPointsTo::MultiPointsTo(bool global) : decls(*this)
{
  env->pool_list.push_back(this);
  PointsToItself = global ;
  if ( global )
    SideEffectsFrom.push_back(this);
  collected = NULL ;
}

void
MultiPointsTo::created(C_Type *t, PA_Set *set)
{
  // a pool is transitive
  switch( t->tag ) {
  case Pointer:
  case FunPtr:
    env->addConstraint(new DerefConstraint( (*this)[t->ptr_next()], set ));
    break ;
  case Array:
    env->addConstraint(new ArrayConstraint( (*this)[t->array_next()], set ));
    break ;
  case StructUnion: {
    unsigned m = 1 ;
    foreach(i,t->user_types(),Plist<C_Type>)
      env->addConstraint(new MemberConstraint( (*this)[*i], set, m++ ));
    break; }
  case Function:
    break ;
  case Abstract:
  case Primitive:
  case Enum:
    break ;
  }
                       
  // a global set points to its own decls
  if ( PointsToItself && t->tag != Function )
    set->insert( decls[t] );

  // side effects on variables the pool knows
  if ( t->tag == Pointer || t->tag == FunPtr )
    foreach(op, SideEffectsFrom, Plist<MultiPointsTo>)
      env->addConstraint(new AssignConstraint1( set, (**op)[t->ptr_next()] ));
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Constraint generation
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

void
PAEnv::addConstraint(PAconstraint* fpv) {
  assert(addConstraintAllowed);
  PAsolver += fpv;
}

static PA_Set*
paExpr(C_Expr* e)
{
  if ( padebug^4 ) padebug << "pa for expr " << (void*)e << " ";
  PA_Set* eset = env->PointsToNothing ;
  // Expression cases that do NOT always point to nothing must change
  // eset before using it. We don't install the eset into the map
  // until after the case statement: this is ok because expressions
  // are not recursive, so the recursive constraint generation will
  // not need to find it in the map.
  switch (e->tag) {
  case C_ExtFun:
  case C_FunAdr:
  case C_Var:
    // The address of a variable points to the declaration of the variable.
    eset = env->singleton(e->var());
    break;
  case C_EnumCnst:
  case C_Null:
  case C_SizeofT:
    break;
  case C_Cast:
    // benign casts must not lose points-to information
    if ( e->isBenign() ) {
        eset = paExpr(e->subexpr());
        break ;
    } // else fall through
  case C_Unary:
  case C_SizeofE:
    paExpr(e->subexpr());
    break;
  case C_PtrCmp:
  case C_Binary:
    paExpr(e->binary_expr1());
    paExpr(e->binary_expr2());
    break;
  case C_Cnst:
    // XXX is it wise to let 'magic words' of pointer type point nowhere???
    break;
  case C_PtrArith:
    paExpr(e->binary_expr2());
    // The first subexpression is the pointer.
    eset = paExpr(e->binary_expr1());
    break;
  case C_Member:
    eset = new PA_Set ;
    env->addConstraint(new MemberConstraint(eset, paExpr(e->subexpr()),
                                            e->struct_nr()));  
    break;
  case C_Array:
    eset = new PA_Set ;
    env->addConstraint(new ArrayConstraint(eset, paExpr(e->subexpr()))); 
    break;
  case C_DeRef:
    eset = new PA_Set ;
    env->addConstraint(new DerefConstraint(eset, paExpr(e->subexpr()))); 
  }
  // Map the expression to its alocset.
  env->alocset( e, eset );
  return eset;
}

static void
paStmt(C_Stmt* s)
{
  assert(s);
  if (padebug%2) padebug << '.';
  if ( padebug^4 ) padebug  << endl << "pa for stmt " << (void*)s << "@" << s->pos;
  PA_Set* target = env->PointsToNothing ;
  if (s->hasTarget()) {    
    target = paExpr(s->target());
  }
  switch (s->tag) {
  case C_Assign: // *e = e'
    paExpr(s->assign_expr());
    env->addAssignConstraint(target, s->assign_expr());
    break;
  case C_Call: { // *e = f(e_1,...,e_n)
    foreach( a, s->call_args(), Plist<C_Expr> ) paExpr(*a);
    env->addConstraint(new CallConstraint(target, paExpr(s->call_expr()),
                                          s->call_args()));
    break; }
  case C_Alloc: {   // allocation of form *e = calloc(e,sizeof(...))
    C_Decl* objects = s->alloc_objects() ;
    if (objects->type->hasSize())
      paExpr(objects->type->array_size());
    C_Decl* object = objects->members().front() ;
    env->addConstraint(new AssignConstraint1(target,env->singleton(object)));
    break; }
  case C_Free: {    // deallocation of the form free(e).
    C_Expr* e = s->free_expr();
    paExpr(e);
    env->freeExpr.push_back(e);
    break; }
  case C_Sequence:
    break;
  }
}

static void
paJump(C_Jump* s)
{
  assert(s);
  switch (s->tag) {
  case C_If: 
    paExpr( s->cond_expr() );
    break;
  case C_Goto:
    break;
  case C_Return: 
    if (s->hasExpr()) {
      C_Expr *rexpr = s->return_expr() ;
      paExpr( rexpr );
      switch( rexpr->type->tag ) {
      case StructUnion:
        // when returning a struct, the PT set for the function records the
        // addressed where the struct may be copied from by the return
        // action.
        assert( rexpr->tag == C_DeRef );
        rexpr = rexpr->subexpr() ;
        // fall through
      case Pointer:
      case FunPtr:
        env->addConstraint( new SubsetConstraint( env->alocset(env->current()),
                                                  env->alocset(rexpr) ));
        break ;
      default:
        break ;
      }
    }
    break; 
  }
}

static void
paBB(Plist<C_BasicBlock>& bbs)
{
  foreach(i, bbs, Plist<C_BasicBlock>) {
    foreach(s, i->getBlock(), Plist<C_Stmt>) {
      paStmt(*s);
    }
    paJump(i->exit());
  }
}


static void
paFun(C_Decl* d)
{
  // The "return location" is in the members().
  assert(d && d->tag == FunDf && !d->blocks().empty());
  env->setCurrentFun(d);
  paBB(d->blocks());
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Result normalization and cleanup
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

class PAtransferr {
  PAresult &result ;
  ALocSet *emptyset ;
  C_Decl *external ;
  array<C_Decl,ALocSet*> singletons ;
  array<PA_Set,ALocSet*> metbefore ;
  Pset <C_Type> typesmet ;
public:
  PAtransferr(PAresult &r)
    : result(r),
      emptyset(new ALocSet),
      external(new C_Decl(ExtState,NULL,NULL)),
      singletons(NULL), metbefore(NULL)
    { result.PTvar[external] = emptyset; }
  
  ALocSet *replace(PA_Set* setp) {
    if ( setp == NULL )
      return emptyset ;
    else if ( setp->size() == 0 )
      return emptyset ;
    else if ( setp->size() == 1 ) {
      C_Decl *contents = *setp->begin();
      if ( contents->tag == ExtState )
        contents = external ;
      if ( singletons[contents] == NULL )
        singletons[contents] = new ALocSet(contents);
      return singletons[contents] ;
    } else {
      if ( metbefore[setp] == NULL ) {
        ALocSet *newset = new ALocSet ;
        foreach(i,*setp,PA_Set) {
          if ( i->tag == ExtState )
            newset->insert(external);
          else
            newset->insert(*i);
        }
        // XXX some kind of hashing should probably be used to
        // share these sets more aggressively
        metbefore[setp] = newset ;
      }
      return metbefore[setp] ;
    }
  }
  void collect_pool(MultiPointsTo*);
  void side_effects_on(ALocSet*);
  
  void transfer(C_Type*);
  void transfer(C_UserDef*);
  void transfer(C_EnumDef*);
  void transfer(C_Decl*);
  void transfer(C_Init*);
  void transfer(C_Jump*);
  void transfer(C_Stmt*);
  void transfer(C_Expr*);
  void transfer(C_Pgm const&);

  ~PAtransferr();
};

void
PAtransferr::side_effects_on(ALocSet *s)
{
  foreach(i,*s,ALocSet) {
    C_Decl *d = *i ;
    if ( d->tag != VarDcl )
      break ;
    while ( d->isContained() && d->containedIn()->tag == VarDcl )
      d = d->containedIn() ;
    result.sideEffectedObjects.insert(d);
  }
}

void
PAtransferr::collect_pool(MultiPointsTo* pool)
{
  assert(pool->collected == NULL);
  ALocSet *collected = new ALocSet ;
  foreach(t,pool->known_types,Plist<C_Type>) {
    foreach(i,*(*pool)[*t],PA_Set) {
      if ( i->tag == ExtState )
        collected->insert(external);
      else
        collected->insert(*i);
    }
  }
  if ( collected->empty() ) {
    pool->collected = emptyset ;
    delete collected ;
  } else
    pool->collected = collected ;
}
    
////// Types //////

void
PAtransferr::transfer(C_Type* t)
{
  switch(t->tag) {
  case Primitive: case Abstract: case EnumType:
    break;
  case FunPtr: case Pointer:
    transfer(t->ptr_next());
    break;
  case Array:
    if ( t->hasSize() )
      transfer(t->array_size());
    transfer(t->array_next());
    break;
  case Function: {
    foreach(arg,t->fun_params(),Plist<C_Type>)
      transfer(*arg);
    transfer(t->fun_ret());
    break; }
  case StructUnion:
    if ( typesmet.insert(t) )
      return ; // OK if we've already been here
    foreach(member,t->user_types(),Plist<C_Type>)
      transfer(*member);
    break;
  }
}

////// User declarations //////

void
PAtransferr::transfer(C_UserDef* ud)
{
  foreach(m,*ud,C_UserDef) {
    if ( m.name()->hasValue() )
      transfer(m.name()->value());
  }
}

void
PAtransferr::transfer(C_EnumDef* ed)
{
  foreach(m,ed->members(),Plist<C_UserMemb>) {
    if ( m->hasValue() )
      transfer(m->value());
  }
}
    
/////// Declarations ///////

void
PAtransferr::transfer(C_Decl* d)
{
  transfer(d->type);
  foreach(subsiduary,d->members(),Plist<C_Decl>)
    transfer(*subsiduary);
  result.PTvar[d] = replace(env->alocset(d));
  switch (d->tag) {
  case VarDcl:
    if ( d->hasInit())
      transfer(d->init());
    break ;
  case FunDf: {
    foreach(par,d->fun_params(),Plist<C_Decl>)
      transfer(*par);
    foreach(local,d->fun_locals(),Plist<C_Decl>)
      transfer(*local);
    foreach(bb,d->blocks(),Plist<C_BasicBlock>) {
      foreach(stmt,bb->getBlock(),Plist<C_Stmt>)
        transfer(*stmt);
      transfer(bb->exit());
    }
    break ; }
  case ExtFun: {
    MultiPointsTo *global_pool = env->global_pool[d] ;
    MultiPointsTo *local_pool = env->local_pool[d] ;
    if ( global_pool == local_pool )
      result.ExtfunReading[d] = global_pool->collected ;
    else {
      ALocSet *mayread = new ALocSet ;
      *mayread += *global_pool->collected ;
      *mayread += *local_pool->collected ;
      result.ExtfunReading[d] = mayread ;
    }
    if ( d->effects() && d->effects()->state == SEPure )
      result.ExtfunWriting[d] = emptyset ;
    else {
      result.ExtfunWriting[d] = local_pool->collected ;
      side_effects_on(local_pool->collected);
    }
    break ; }
  case ExtState:
    assert(0);
    break;
  }
}

void
PAtransferr::transfer(C_Init* i)
{
  switch(i->tag) {
  case Simple:
    transfer(i->simple_init());
    break ;
  case FullyBraced:
  case SloppyBraced: {
    foreach(j,i->braced_init(),Plist<C_Init>)
      transfer(*j);
    break ; }
  case StringInit:
    break ;
  }
}

////// Control statements //////

void
PAtransferr::transfer(C_Jump* j)
{
  switch (j->tag) {
  case C_If:
    transfer(j->cond_expr());
    break;
  case C_Goto:
    break;
  case C_Return:
    if ( j->hasExpr() )
      transfer(j->return_expr());
    break;
  }
}

////// Statements //////

void
PAtransferr::transfer(C_Stmt* s)
{
  if ( s->hasTarget() ) {
    transfer(s->target());
    side_effects_on(result.PT[s->target()]) ;
  }
  switch (s->tag) {
  case C_Assign:
    transfer(s->assign_expr());
    break;
  case C_Call: {  // call of form [x =] (e_0)(e_1,...,e_n)
    transfer(s->call_expr());
    foreach(arg,s->call_args(),Plist<C_Expr>)
      transfer(*arg);
    break; }
  case C_Alloc:
    transfer(s->alloc_objects());
    break ;
  case C_Free:    // deallocation.
    transfer(s->free_expr());
    break;
  case C_Sequence: // sequence point
    break;
  }
}

////// CoreC Expressions //////

void
PAtransferr::transfer(C_Expr* e)
{
  result.PT[e] = replace(env->alocset(e));
  switch (e->tag) {
  case C_Cnst:            // Constants - NB! can have complex types
  case C_Null:            // Null pointer constants of various types
    transfer(e->type);
    break;
  case C_ExtFun:
  case C_EnumCnst:
  case C_FunAdr:
  case C_Var:
    break;
  case C_Member:
  case C_DeRef:
  case C_Unary:
  case C_Array:
  case C_SizeofE:
    transfer(e->subexpr());
    break;
  case C_Binary:          // arith @ arith ==> arith
  case C_PtrCmp:          // pointer @ pointer ==> integral
  case C_PtrArith:        // pointer @ integral ==> pointer
    transfer(e->binary_expr1());
    transfer(e->binary_expr2());
    break;
  case C_SizeofT:         // sizeof (type)
    transfer(e->sizeof_type());
    break;
  case C_Cast:            // random cast. Possibly arithmetic, possibly sick.
    transfer(e->type);
    transfer(e->subexpr());
    break;
  }
}

////// CoreC program //////

void
PAtransferr::transfer(C_Pgm const &pgm)
{
  foreach(ud,pgm.usertypes,Plist<C_UserDef>)
    transfer(*ud);
  foreach(ed,pgm.enumtypes,Plist<C_EnumDef>)
    transfer(*ed);
  foreach(var,pgm.globals,Plist<C_Decl>)
    transfer(*var);
  foreach(exf,pgm.exfuns,Plist<C_Decl>)
    transfer(*exf);
  foreach(fun,pgm.functions,Plist<C_Decl>)
    transfer(*fun);
  foreach(gen,pgm.generators,Plist<C_Decl>)
    transfer(*gen);
}

PAtransferr::~PAtransferr()
{
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Main routine
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

void
pointsToAnalysis(C_Pgm const& pgm, PAresult& result)
{
  // Initialize the sets and maps that constitute the result of the analysis.
  // (We could delete existing things, ie from prior runs of this analysis).
  env = new PAEnv(result);
  env->PointsToNothing = new PA_Set ;
  constraintPool = new PAconstraintSet; // For debugging

  makepools(pgm);
  
  if (padebug^1) padebug << "  Generating PA constraints ";
  // The PT info is implemented as a safe array, and thus each declaration will
  // get a new set the first time it is touch (in the sense that the set is
  // required). 
  foreach(ds, pgm.functions, Plist<C_Decl>) {
    paFun(*ds);
  }
  foreach(ds2, pgm.generators, Plist<C_Decl>) {
    paFun(*ds2);
  }
  // Solve the constraints.
  while(1) {
    if (padebug^1) padebug << "  Solving PA constraints";
    env->solve();
    // Debug start
    if (padebug^1) padebug << "... checking solution\n";
    env->addConstraintAllowed = false ;
    // Fire all constraints to see if further changes have to be made.
    foreach(fpv, *constraintPool, PAconstraintSet)
      ((FixpointPureVertex*)(*fpv))->Solve(env->PAsolver);
    if (env->PAsolver.empty() )
      break ;
    Diagnostic(WARNING,Position())
      << "PA constraint dependencies not correct: iterating again";
  }
  // dispose of the constraints
  while ( !constraintPool->empty() )
    delete *constraintPool->begin() ;
  
  // transfer all of the collected sets to new "external sets" (so that
  // we can dispose of those we use internally).
  {
    PAtransferr transferr(result) ;
    foreach(pool,env->pool_list,Plist<MultiPointsTo>)
      transferr.collect_pool(*pool);
    transferr.transfer(pgm);
  }
   
  foreach(pool,env->pool_list,Plist<MultiPointsTo>) {
    foreach(t,pool->decls.known_types,Plist<C_Type>)
      delete pool->decls[*t] ;
    delete *pool ;
  }
  foreach(paset,env->pa_set_list,Plist<PA_Set>) {
    delete *paset ;
  }

  // Transfer all PA from all free constructs to the result
  foreach( e, env->freeExpr, Plist<C_Expr>) {
    result.freedHeapObjects += *result.PT[*e];
  }
  if (padebug^2) padebug << "[freed: " << result.freedHeapObjects << ']' << endl;

  delete env ;
}

