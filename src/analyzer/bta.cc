/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Constraint generator for binding-time analysis
 * History:  Derived from code by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

// ===========================================================================
// Support for varargs are not yet implemented
// ===========================================================================

// ===========================================================================
// The binding-time analysis is split into two phases: a constraint generation
// phase and a constraint solving phase.
// ===========================================================================

#include <cmixconf.h>
#include "diagnostic.h"
#include "bta.h"
#include "analyses.h"
#include "options.h"
#include "commonout.h"
#include "outcore.h"
#include "auxilary.h"
#include "ALoc.h"
#include "directives.h"

#define btadebug debugstream_bta

#define NOPARTIALSTRUCTS

Output *
BTanonymousVar::show(Outcore *)
{
  return new Output("This is residual",OAnnoText);
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  C A U S E    C L A S S E S
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

Numbered *current_cause = NULL ;

class StandardCause : public BTcause {
  Numbered *num ;
  Position pos ;
  char const *text ;
public:
  StandardCause(Position p,char const *t)
    : num(current_cause), pos(p), text(t)
    { assert(current_cause != NULL); }
  virtual void show(Diagnostic &d)
    { d.addline(pos) << text ; }
  virtual Output *show(Outcore *oc)
    { return oc->crosslink(new Output(text,OAnnoText),num); }
};

static inline BTcause *
mkExprCause(Position p)
{
  return new StandardCause(p,"expression sanity");
}

class ControlFlowCause : public StandardCause {
public:
  ControlFlowCause() : StandardCause(Position(),"control flow") {}
  virtual bool hasOldName() { return false; }
};

class UnionRule : public BTcause {
  C_Decl *owner ;
public:
  UnionRule(C_Decl *d) : owner(d) {}
  virtual void show(Diagnostic &d)
    { d.addline(owner->pos) << "the union rule for " << *owner; }
  virtual Output *show(Outcore *oc)
    {
      Output *o = new Output("The union rule for",OAnnoText);
      return oconcat(o,outputDeclLink(owner,oc));
    }
};

class UserAnnoCause : public BTcause {
  Position pos ;
public:
  UserAnnoCause(Position p) : pos(p) {}
  virtual void show(Diagnostic &d)
    { d.addline(pos) << "explicit user annotation"; }
  virtual Output *show(Outcore *oc)
    { return new Output("Explicit user annotation",OAnnoText); }
};

class CallOrderCause : public BTcause {
  Position pos ;
  C_Decl *fun ;
public:
  CallOrderCause(Position p,C_Decl *f): pos(p), fun(f) {}
  virtual void show(Diagnostic &d) {
    d.addline(pos) << "call to sensitive external function "
                   << fun->get_name() << " under dynamic control" ;
  }
  virtual Output *show(Outcore *oc) {
    Plist<Output> *os = new Plist<Output> ;
    os->push_back(new Output("call to sensitive external function",OAnnoText));
    os->push_back(BREAK);
    os->push_back(oc->crosslink(new Output(fun->get_name(),OFunname),fun));
    os->push_back(BREAK);
    os->push_back(new Output("under dynamic control",OAnnoText));
    return new Output(Output::Inconsistent,os);
  }
};
                  
class NonLocalSE : public BTcause {
  C_Stmt *here ;
  C_Decl *on ;
public:
  NonLocalSE(C_Stmt *s, C_Decl *d) : here(s), on(d) {}
  virtual void show(Diagnostic &d)
    {
      d.addline(here->pos) << "non-local side effect";
      C_Decl *namefinder = on;
      while ( namefinder->isContained() &&
              namefinder->containedIn()->tag != FunDf )
        namefinder = namefinder->containedIn() ;
      if ( namefinder->hasName() )
        d.addline(on->pos) << "(on " << *on << ')' ;
    }
  virtual Output *show(Outcore *oc)
    {
      Output *o ;
      o = new Output("Non-local side effect",OAnnoText);
      o = oc->crosslink(o,here);
      Plist<Output> *os = new Plist<Output> ;
      os->push_back(o);
      os->push_back(INDENT);
      os->push_back(new Output("on",OAnnoText));
      os->push_back(blank);
      os->push_back(outputDeclLink(on,oc));
      return new Output(Output::Consistent,os);
    }
};

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  E N V I R O N M E N T   A N D   P R O T O T Y P E S
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

struct BTenv {
  BTmap &map ;
  PAresult const &pa ;
  Pset<C_Type> has_constraints ; // recursion stopper
  Pset<C_Type> made_faithful ; // ditto

  BTenv(BTmap &,PAresult const &);
    
  bool all_in_scope(C_Decl *f, ALocSet *pt, bool finishing);

  void well_form_type(C_Type* t, BTcause*);
  void make_dynamic(C_Type* t, BTcause*);
  void add_dep_constraint(C_Type *t1, C_Type *t2, BTcause*);
  void force_equal(C_Type *t1, C_Type *t2, BTcause*);
  void lift_into(C_Decl *f, C_Type *t1, ALocSet *pt1, C_Type *t2, BTcause*);
  void lift_into(C_Decl *f, C_Expr *e1, C_Type *t2, BTcause*);
  void lift_faithful(C_Decl *f, C_Expr *e1, C_Type *t2, BTcause*);
  void faithful(C_Type *t, BTcause*);

  void solve() ;
private:
  void faithful(C_Type *, BTvariable*, BTcause*);
  BTvariable *dynamic ;
  BTequalizer eql ;
};

static void gen_type_constraints(BTenv&, C_Type*, BTcause *);
static void gen_decl_constraints(BTenv&, C_Decl*);
static void gen_bb_constraints(BTenv&, C_BasicBlock*, C_Decl* fun);
static void gen_stmt_constraints(BTenv&, C_Stmt*, C_Decl* fun, C_BasicBlock*);
static void gen_jump_constraints(BTenv&, C_Jump*, C_Decl* fun, C_BasicBlock*);
static void gen_expr_constraints(BTenv&, C_Expr*, C_Decl* fun, BTcause *);
static void gen_program_constraints(BTenv&, C_Pgm const&);

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  G E N E R A T I N G     C O N S T R A I N T S
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

static void
gen_program_constraints(BTenv &env, C_Pgm const &pgm)
{
  foreach(i,pgm.usertypes,Plist<C_UserDef>) {
    foreach(m,i->names,Plist<C_UserMemb>) {
      if ( m->hasValue() ) {
        current_cause = *m ;
        BTcause *ecause = mkExprCause(m->value()->pos);
        gen_expr_constraints(env,m->value(),NULL,ecause);
      }
    }
  }

  foreach(ed,pgm.enumtypes,Plist<C_EnumDef>) {
    foreach(m,ed->members(),Plist<C_UserMemb>) {
      if ( m->hasValue() ) {
        current_cause = *m ;
        BTcause *ecause = mkExprCause(m->value()->pos);
        gen_expr_constraints(env,m->value(),NULL,ecause);
        BTcause *cause
          = new StandardCause(m->value()->pos,
                              "the entire enum must be residual");
        env.map[m->value()->type]->influence(env.map[*ed],cause);
      }
    }
  }
    
  // Global variables
  foreach(i2,pgm.globals,Plist<C_Decl>) {
    if (btadebug^3) btadebug << '[' << (*i2)->get_name() << ']';
    gen_decl_constraints(env,*i2);
  }

  // Functions.
  foreach(i3,pgm.functions,Plist<C_Decl>) {
    if (btadebug^3) btadebug << '[' << (*i3)->get_name() << ']';
    gen_decl_constraints(env,*i3);
  }

  // Generator pseudofunctions
  foreach(i4,pgm.generators,Plist<C_Decl>) {
    if (btadebug^3) btadebug << '[' << (*i4)->get_name() << ']';
    gen_decl_constraints(env,*i4);
  }

  // External functions
  foreach(i5,pgm.exfuns,Plist<C_Decl>)
    gen_decl_constraints(env,*i5);
}

// ----------------------------------------------------------------------------
// Generate constraints to ensure the wellformedness of a btt (LOA: defn 5.3).

// Note: the binding-time variable in function pointer types is only used
// for detecting whether dynamic pointers to a specializable function
// exist - if that is the case, btdebug emits error messages.

static void
gen_type_constraints(BTenv &env, C_Type* t, BTcause *cause)
{
  switch (t->tag) {
  case Primitive:
  case Abstract:
    // No constraints for base types.
    break;
  case EnumType:
    env.map[t->enum_def()]->influence(env.map[t],cause);
    break ;
  case StructUnion: {
    if ( env.has_constraints.insert(t) )
      break ;
    Plist<C_UserMemb>::iterator m = t->user_def()->names.begin() ;
    foreach(i,t->user_types(),Plist<C_Type>) {
      // 1. the member must be well-formed itself
      gen_type_constraints(env, *i, cause);
      // 2. bitfields must be dynamic if their widths are
      if ( m->hasValue() )
        env.map[m->value()->type]->influence(env.map[*i],cause);
      // 3. if any field is dynamic, mark the struct as having a
      //    dynamic part
      env.add_dep_constraint(i->array_contents_type(),t,cause);
#ifdef NOPARTIALSTRUCTS
      // 4. if there is any dynamic part in the struct, or dynamic
      //    pointers to it, no members may have static values.
      env.add_dep_constraint(t,i->array_contents_type(),cause);
#endif
      m++ ;
    }
    break ; }
  case FunPtr:
  case Pointer:
    // Recursively traverse the type.
    gen_type_constraints(env,t->ptr_next(),cause);
    // If the pointer type is dynamic, then so is the type pointed to.
    env.add_dep_constraint(t, t->ptr_next(), cause);
    break;
  case Array:
    if( t->hasSize() ) {
      gen_expr_constraints(env,t->array_size(),NULL,
                           mkExprCause(t->array_size()->pos));
      // If the size is dynamic, then so is the array type.
      env.add_dep_constraint(t->array_size()->type, t, cause);
    }
    // Recursively traverse the type.
    gen_type_constraints(env,t->array_next(), cause);
    // If this array type is dynamic, then so is the type pointed to.
    env.add_dep_constraint(t, t->array_next(), cause);
    break;
  case Function:
    gen_type_constraints(env,t->fun_ret(),cause);
    // Generate constraints for the parameters.
    foreach(i, t->fun_params(), Plist<C_Type>) {
      gen_type_constraints(env,*i,cause);
    }
    break;
  }
}

static void
no_followable_ptrs(BTenv &env, C_Decl *d, BTcause *cause)
{
  if ( d->type->tag == Pointer )
    env.make_dynamic(d->type->ptr_next(), cause);
  else
    foreach(sub,d->members(),Plist<C_Decl>)
      no_followable_ptrs(env,*sub,cause);
}

static void
the_union_rule(BTenv &env, C_Decl *d)
{
  if ( d->type->tag != StructUnion || !d->type->user_def()->isUnion ) {
    foreach(sub,d->members(),Plist<C_Decl>)
      the_union_rule(env,*sub);
    return ;
  }

  // This is a union object: enforce the initial sequence rule
  BTcause *cause = new UnionRule(d);
  Plist<C_Decl> &umembers = d->members() ;
  // enforce the initial sequence rule
  foreach(m,umembers,Plist<C_Decl>)
    if ( m->type->tag != StructUnion ||
         m->type->user_def()->isUnion ) {
      no_followable_ptrs(env,d,cause);
      return ;
    }
  // Now we know all the union members are structs. Set up
  // arrays to simultaneously traverse their member lists.
  unsigned umember_count = umembers.size() ;
  Plist<C_Decl>::iterator *it = new Plist<C_Decl>::iterator[umember_count] ;
  unsigned i = 0 ;
  foreach(mm,umembers,Plist<C_Decl>)
    it[i++] = mm->members().begin() ;
  // as long as the members of all agree, just tie their
  // types together and perform the union rule on them
  while(1) {
    // "goto found_difference" exits this loop and functions
    // as a multi-level break.
    for ( i = 0 ; i < umember_count ; i++ )
      if ( ! it[i] )
        goto found_difference ;
    for ( i = 1 ; i < umember_count ; i++ )
      if ( !it[0]->type->equal(*it[i]->type) )
        goto found_difference ;
    // This member is in a common initial subseq
    for ( i = 1 ; i < umember_count ; i++ )
      env.force_equal( it[0]->type, it[i]->type, cause );
    the_union_rule( env, *it[0] );
    for ( i = 0 ; i < umember_count ; i++ )
      it[i]++ ;
  }
 found_difference:
  // the rest of the members are not in a c.i.s.
  for ( i = 0 ; i < umember_count ; i++ )
    while( it[i] )
      no_followable_ptrs( env, *it[i]++, cause );
  delete it ;
  return ;
}

static void
sloppy_init_constraints(BTenv &env, C_Init *i, C_Type *t, C_Decl *fun,
                        BTcause *cause)
{
  switch(i->tag) {
  case Simple:
    env.lift_faithful(fun,i->simple_init(),t,cause);
    return ;
  case StringInit:
    return ;
  case FullyBraced:
  case SloppyBraced:
    foreach(j,i->braced_init(),Plist<C_Init>)
      sloppy_init_constraints(env,*j,t,fun,cause);
    return ;
  }
}

static void
gen_init_constraints(BTenv &env, C_Init *i, C_Type *t, C_Decl *fun,
                     BTcause *cause)
{
  switch(i->tag) {
  case Simple:
    gen_expr_constraints(env, i->simple_init(),fun,cause);
    env.lift_into(fun,i->simple_init(),t,cause);
    return ;
  case StringInit:
    // statically indexed array of dynamic chars cannot
    // be initialized with string literals.
    env.add_dep_constraint(t->array_next(),t,cause);
    return ;
  case FullyBraced:
    switch(t->tag) {
    case Array: {
      foreach(j,i->braced_init(),Plist<C_Init>)
        gen_init_constraints(env,*j,t->array_next(),fun,cause);
      return ; }
    default:
      assert(0);
    case StructUnion: {
      Plist<C_Type>::iterator ti = t->user_types().begin() ;
      BTcause *cause2 = new StandardCause(Position(),
                                          "cannot split initializer");
      foreach(j,i->braced_init(),Plist<C_Init>) {
        env.add_dep_constraint(ti->array_contents_type(),*ti,cause2);
        gen_init_constraints(env,*j,*ti,fun,cause);
        ti++ ;
      }
      return ; }
    }
  case SloppyBraced:
    cause = new StandardCause(Position(),
                              "incompletely braced initializer");
    env.faithful(t,cause);
    sloppy_init_constraints(env,i,t,fun,cause);
    return ;
  }
}

static void
gen_decl_constraints(BTenv &env, C_Decl *d)
{
  current_cause = d ;

  switch (d->tag) {
  case VarDcl:
    // Do the type first.
    env.well_form_type(d->type,new StandardCause(d->pos,"well-formed type"));
      
    // Then the union rule
    the_union_rule(env,d);

    // External variables must be faithful
    switch(d->varmode()) {
    case VarExtDefault:
    case VarExtResidual:
    case VarExtSpectime:
    case VarVisResidual:
    case VarVisSpectime:
      env.faithful
        (d->type,new StandardCause(d->varmodeWhy(),
                                   "well-formed external type"));
      break ;
    default:
      break ;
    }

    // Obey residual annotations
    switch(d->varmode()) {
    case VarExtResidual:
    case VarExtDefault:
    case VarVisResidual:
    case VarIntResidual:
      if (btadebug ^ 2) {
        btadebug << " " << d->get_name() << " annotated D ";
      }
      env.make_dynamic(d->type,new UserAnnoCause(d->varmodeWhy()));
      break;
    default:
      break;
    }

    if ( d->hasInit() )
      gen_init_constraints(env, d->init(), d->type, d,
                           new StandardCause(d->pos,"initializer"));
      
    break ;
  case FunDf: {
    env.well_form_type(d->type->fun_ret(),
                       new StandardCause(d->pos,"well-formed return type"));
    foreach(i, d->fun_params(), Plist<C_Decl>)
      gen_decl_constraints(env, *i);
    foreach(ii, d->fun_locals(), Plist<C_Decl>)
      gen_decl_constraints(env, *ii);
    foreach(bb, d->blocks(), Plist<C_BasicBlock>)
      gen_bb_constraints(env,*bb,d);
    break ; }
  case ExtFun: {
    Position p(d->pos) ;
    CallTime ct = CTNoAnno ;
    if ( d->calltime() ) {
      ct = d->calltime()->time ;
      p = d->calltime()->pos ;
    }
    switch(ct) {
    case CTSpectime:
      return ;
    case CTNoAnno:
      if ( d->effects() == NULL ) {
        env.make_dynamic(d->type,new StandardCause(p,
                     "unannotated call to external function"));
        return ;
      }
      if ( d->effects()->state != SERWState &&
           d->effects()->state != SEROState )
        return ;
      p = d->effects()->pos ;
      // fall through
    case CTResidual:
      env.make_dynamic(d->type,new StandardCause(p,
                      "function is annotated to be called at spectime"));
      return ;
    }
    break; }
  case ExtState:
    Diagnostic(INTERNAL,RIGHT_HERE) << "this cannot happen";
    break;
  }
}

static void
gen_bb_constraints(BTenv &env, C_BasicBlock *bb, C_Decl *function)
{
  // Put the label number on the btype.
  foreach(it, bb->getBlock(), Plist<C_Stmt>)
    gen_stmt_constraints(env,*it,function,bb);
  gen_jump_constraints(env, bb->exit(),function, bb);
}

// This function gets the surrounding basic block and function.
static void
gen_stmt_constraints(BTenv &env, C_Stmt *s, C_Decl *fun, C_BasicBlock *bb)
{
  current_cause = s ;
  BTcause *cause = mkExprCause(s->pos);

  C_Type *target = NULL ;
    
  if ( s->hasTarget() ) {
    gen_expr_constraints(env,s->target(),fun,cause);
    target = s->target()->type->ptr_next() ;

    // assignment through S pointer to D must only
    // be allowed when all the pointed-to objects are
    // in scope
    if ( !env.all_in_scope(fun, env.pa.PT[s->target()], true) )
      env.add_dep_constraint(target,s->target()->type,cause);
        
    // if the target can be non-local and is under dynamic
    // control, residualize the assigned expression
    foreach(i,*env.pa.PT[s->target()],ALocSet)
      if ( !env.pa.TrulyLocals[fun]->find(*i) ) {
        BTcause *cause2 = new NonLocalSE(s,*i);
        C_Type *assigntype = s->target()->type->ptr_next() ;
        env.map[fun]->influence(env.map[assigntype],cause2);
        env.map[bb]->influence(env.map[assigntype],cause2);
        break ; // one is enough
      }
  }
    
  switch (s->tag) {
  case C_Assign: {
    C_Expr *e = s->assign_expr() ;
    gen_expr_constraints(env, e, fun, cause);
    assert(target);
    env.lift_into(fun, e, target,
                  new StandardCause(s->pos,"assignment"));
    break ; }
  case C_Call: {
    C_Expr *e0 = s->call_expr() ;
    C_Type *tpf = e0->type ;
    C_Type *tf = tpf->ptr_next() ;

    // find out if the function may be external, and
    // connect the "under dynamic control" variables
    bool may_call_external = false ;
    C_Decl *sensitive_call = NULL ;
    BTcause *dynctl_cause = new StandardCause(s->pos,"function call");
    foreach(i,*env.pa.PT[e0],ALocSet) switch(i->tag) {
    case FunDf:
      env.map[fun]->influence(env.map[*i],dynctl_cause);
      env.map[bb] ->influence(env.map[*i],dynctl_cause);
      break ;
    case ExtFun:
      if ( i->effects() == NULL || i->effects()->state == SERWState )
        sensitive_call = *i ;
      // fall through
    case ExtState:
      may_call_external = true ;
      break ;
    case VarDcl:
      Diagnostic d(WARNING,s->pos);
      d << "possible call of non-function" ;
      d.addline(i->pos) << '(' << **i << " possibly called)" ;
      break ;
    }
        
    gen_expr_constraints(env, e0, fun, cause);
        
    // lift the actuals into the formals ;
    Plist<C_Expr>::iterator actual = s->call_args().begin() ;
    Plist<C_Type>::iterator formal = tf->fun_params().begin() ;
    for ( ; actual ; actual++ ) {
      gen_expr_constraints(env, *actual, fun, cause);
      if ( formal ) {
        env.lift_into(fun,
                      actual->type, env.pa.PT[*actual],
                      *formal,
                      cause);
        formal ++ ;
      } else {
        BTcause *cause2 = new StandardCause(actual->pos,
                                            "variadic parameter");
        env.lift_faithful(fun,*actual,tpf,cause2);
      }
    }
    assert( !formal );

    // if there is a target, connect it to the return value
    if ( target ) {
      BTcause *cause2 = new StandardCause(s->target()->pos,
                                          "use of return value");
      if ( may_call_external ||
           e0->type->ptr_next()->fun_ret()->tag == StructUnion )
        env.force_equal(tf->fun_ret(),target,cause2);
      else {
        ALocSet retptset ;
        foreach(fn,*env.pa.PT[e0],ALocSet)
          retptset += *env.pa.PTvar[*fn] ;
        env.lift_into(fun,tf->fun_ret(),&retptset,target,cause2);
      }
    }
    
    // take care of call mode annotations
    if ( sensitive_call ) {
      BTcause *cause3 = new CallOrderCause(s->pos,sensitive_call);
      env.map[fun]->influence(env.map[tpf],cause3);
      env.map[bb] ->influence(env.map[tpf],cause3);
    }
    break; }
  case C_Alloc: {
    C_Decl *pseudoarray = s->alloc_objects() ;
    env.well_form_type(pseudoarray->type,
                       new StandardCause(s->pos,"well-formed type"));
    the_union_rule(env,pseudoarray);
    assert(target);

    cause = new StandardCause(s->pos,"heap allocation");
        
    env.force_equal(target->ptr_next(),
                    pseudoarray->type->array_next(),cause);
    env.add_dep_constraint(pseudoarray->type,target,cause);
    env.add_dep_constraint(target,pseudoarray->type,cause);

    // The allocation becomes dynamic if it is under dynamic control
    cause = new StandardCause(s->pos,"speculative allocation");
    env.map[fun]->influence(env.map[pseudoarray->type],cause);
    env.map[bb ]->influence(env.map[pseudoarray->type],cause);

    break; }
  case C_Free: {
    gen_expr_constraints(env,s->free_expr(),fun,cause);
    assert(!target);
    // the binding time of the split must match that of the
    // original allocation
    cause = new StandardCause(s->pos,"residual deallocation");
    foreach(pt,*env.pa.PT[s->free_expr()],ALocSet) {
      if ( !pt->isContained() ||
           pt->containedIn()->type->tag != Array )
        continue ; // e.g. if other pointers got mixed in along the way
      env.add_dep_constraint(s->free_expr()->type,
                             pt->containedIn()->type,cause);
    }
    break ; }
  case C_Sequence:
    break;
  }
}

static void
gen_jump_constraints(BTenv &env, C_Jump *s, C_Decl *fun, C_BasicBlock *bb)
{
  current_cause = s ;
  switch (s->tag) {
  case C_If: {
    gen_expr_constraints(env,s->cond_expr(),fun,mkExprCause(s->pos));

    BTvariable *btA = env.map[s->cond_expr()->type];
    BTvariable *btB = env.map[bb] ;

    BTvariable *btX = env.map[s->cond_then()];
    BTvariable *btY = env.map[s->cond_else()];

    BTcause *cause = new ControlFlowCause ;
    btA->influence(btX,cause);
    btA->influence(btY,cause);
    btB->influence(btX,cause);
    btB->influence(btY,cause);
    break;}
  case C_Goto:
    env.map[bb]->influence(env.map[s->goto_target()],new ControlFlowCause);
    break;
  case C_Return: {
    if ( s->hasExpr() ) {
      gen_expr_constraints(env,s->return_expr(),fun,mkExprCause(s->pos));
      env.lift_into(fun,s->return_expr(),fun->type->fun_ret(),
                    new StandardCause(s->pos,"return statement"));
      // if under dynamic control, residualize the return value
      // in this single case the dynamic control of the point we're
      // called from does not matter.
      env.map[bb]->influence(env.map[fun->type->fun_ret()],
                             new ControlFlowCause);
    }
    break;}
  }
}

static void
gen_expr_constraints(BTenv &env, C_Expr *e, C_Decl* fun, BTcause *cause)
{
  // step I: ensure the well-formedness of types owned by the expression
  switch (e->tag) {
  case C_Cnst: case C_EnumCnst: case C_Null: case C_Unary:
  case C_Binary: case C_PtrCmp: case C_SizeofT: case C_SizeofE:
  case C_Cast:
    env.well_form_type(e->type,cause);
    break ;
  case C_Member: case C_Var: case C_FunAdr: case C_Array: case C_PtrArith:
    env.add_dep_constraint(e->type,e->type->ptr_next(),cause);
    break ;
  case C_ExtFun: case C_DeRef:
    break ;
  }
    
  // step II: generate constraints for the operation itself
  switch (e->tag) {
  case C_ExtFun:
  case C_FunAdr:
  case C_Var:
    // shares type with the object, all OK
    break ;
  case C_EnumCnst:
    // dynamic if the enumeration itself is
    env.map[e->enum_cnst()]->influence(env.map[e->type],cause);
    break ;
  case C_Null:
    // nothing more to do for this
    break ;
  case C_Cnst:
    // btvars.cc has made sure that the entire variable
    // shares a single BTvariable, so nothing more is needed
    break ;
  case C_Unary:
    // unary operands are always primitive
    // hence can always be lifted
    env.add_dep_constraint(e->subexpr()->type,e->type,cause);
    // generate constraints for the operand
    gen_expr_constraints(env,e->subexpr(),fun,cause);
    break ;
  case C_PtrArith:
    // for now, tie the result type and the first operand's type together
    env.add_dep_constraint(e->binary_expr1()->type,e->type,cause);
    env.add_dep_constraint(e->type,e->binary_expr1()->type,cause);
    // if the index is dynamic the pointer must also be
    env.add_dep_constraint(e->binary_expr2()->type,e->type,cause);
    // generate constraints for the operands
    gen_expr_constraints(env,e->binary_expr1(),fun,cause);
    gen_expr_constraints(env,e->binary_expr2(),fun,cause);
    break ;
  case C_PtrCmp:
    // the two pointers must point to types with the same BT pattern
    env.force_equal(e->binary_expr1()->type->ptr_next(),
                    e->binary_expr2()->type->ptr_next(),cause);
    // We'll be going to possibly lift the two operand pointers
    // into a common binding time which is the same as that of
    // the result of the operator.
    // For == and != it is permissible to lift pointers-into-arrays.
    // The other pointer comparision operators essentially do
    // pointer arithmetic, so we have to use the full intermediate
    // lifting criterion for them.
    if ( true ) {
      bool finishing = e->binary_op() == Eq || e->binary_op() == NEq ;
      if ( !env.all_in_scope(fun,env.pa.PT[e->binary_expr1()],finishing) )
        env.add_dep_constraint(e->type,e->binary_expr1()->type,cause);
      if ( !env.all_in_scope(fun,env.pa.PT[e->binary_expr2()],finishing) )
        env.add_dep_constraint(e->type,e->binary_expr2()->type,cause);
    }
    // fall through to generate the standard direction constraints
  case C_Binary:
    env.add_dep_constraint(e->binary_expr1()->type, e->type, cause);
    env.add_dep_constraint(e->binary_expr2()->type, e->type, cause);
    // generate constraints for the operands
    gen_expr_constraints(env,e->binary_expr1(),fun,cause);
    gen_expr_constraints(env,e->binary_expr2(),fun,cause);
    break ;
  case C_Member: // pointer to struct => pointer to member
    env.add_dep_constraint(e->subexpr()->type, e->type, cause);
    // It is sometimes convenient to lift the operand to a Member
    // expression if the member may be aliased outside the function
    // but the entire struct is not.
    // The lift is a finishing one, because pointer arithmetic
    // on pointers to the struct cannot be done on pointers to
    // the member.
    if ( !env.all_in_scope(fun,env.pa.PT[e->subexpr()],true) )
      env.add_dep_constraint(e->type, e->subexpr()->type, cause);
    // generate constraints for the operand
    gen_expr_constraints(env,e->subexpr(),fun,cause);
    break ;
  case C_Array: {  // pointer to array => pointer to first element
    C_Type *alpha = e->type ;
    C_Type *beta = e->subexpr()->type ;
    C_Type *gamma = beta->ptr_next();
    // 0. We convert a pointer<beta> to array[gamma] of something
    //    to pointer<alpha> to the same something
    assert( alpha->ptr_next() == gamma->array_next() );
    // 1. The pointer must have the same binding time as the
    //    array indexing, so we must set alpha=gamma.
    env.add_dep_constraint(alpha,gamma,cause);
    env.add_dep_constraint(gamma,alpha,cause);
    // 2. beta=D and alpha=gamma=D is fine.
    // 3. beta=D and alpha=gamma=S is illegal by well-formedness on beta
    // 4. beta=S and alpha=gamma=D is is the same as the fully dynamic
    //    case with a (finishing) lift on the subexpression. So, if
    //    that lift cannot be allowed, put a constraint from gamma
    //    to beta.
    if ( !env.all_in_scope(fun,env.pa.PT[e->subexpr()],true) )
      env.add_dep_constraint(gamma,beta,cause);
    // generate constraints for the operand
    gen_expr_constraints(env,e->subexpr(),fun,cause);
    break ; }
  case C_DeRef:
    // If the pointer is not certain to point to objects in scope,
    // it must have the same binding time as its contents
    if ( !env.all_in_scope(fun,env.pa.PT[e->subexpr()],true) )
      env.add_dep_constraint(e->type,e->subexpr()->type,cause);
    // generate constraints for the operand
    gen_expr_constraints(env,e->subexpr(),fun,cause);
    break ;
  case C_Cast:
    if ( e->isBenign() )
      // lifting may be needed here when coercing an int to an enum.
      env.lift_into(fun,e->subexpr(),e->type,cause);
    else {
      BTcause *cause2 = new StandardCause(e->pos,"suspicious typecast");
      env.faithful(e->type,cause2);
      env.make_dynamic(e->type,cause2);
      env.faithful(e->subexpr()->type,cause2);
      env.make_dynamic(e->subexpr()->type,cause2);
    }
    // generate constraints for the operand
    gen_expr_constraints(env,e->subexpr(),fun,cause);
    break ;
  case C_SizeofT:
    env.well_form_type(e->sizeof_type(),cause);
    cause = new StandardCause(e->pos,"sizeof operator");
    env.make_dynamic(e->sizeof_type(),cause);
    env.make_dynamic(e->type,cause);
    break ;
  case C_SizeofE:
    cause = new StandardCause(e->pos,"sizeof operator");
    gen_expr_constraints(env,e->subexpr(),fun,cause);
    env.make_dynamic(e->sizeof_type(),cause);
    env.make_dynamic(e->type,cause);
    break ;
  }
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  C O N S T R A I N T   G E N E R A T I O N   M A C R O S
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

bool
BTenv::all_in_scope(C_Decl *f, ALocSet *pt, bool finishing)
{
  foreach(i,*pt,ALocSet) {
    C_Decl *d = *i ;

    // Pointers to functions and pointers to
    // external things cannot be lifted
    if ( d->tag != VarDcl )
      return false ;
        
    // When used in an intermediate cast, the pointer must not
    // be able to point directly at array elements
    if ( !finishing &&
         d->isContained() &&
         d->containedIn()->type->tag == Array &&
         d->containedIn()->type->hasSize() )
      return false ;

    // OK. then check if it is in scope
    if ( f == NULL || !pa.TrulyLocals[f]->find(d) ) {
      // It is not a truly local object.
      // It had better be global, then. Find its ultimate parent.
      while ( d->isContained() )
        d = d->containedIn() ;
      if ( d->tag != VarDcl )
        return false ;
    }
  }
  return true ;
}

void
BTenv::well_form_type(C_Type *t, BTcause *cause)
{
  gen_type_constraints(*this,t,cause);
}

void
BTenv::make_dynamic(C_Type *t, BTcause *cause)
{
  dynamic->influence(map[t],cause);
}

void
BTenv::add_dep_constraint(C_Type *t1, C_Type *t2, BTcause *cause)
{
  map[t1]->influence(map[t2],cause);
}

void
BTenv::force_equal(C_Type *t1, C_Type *t2, BTcause *cause)
{
  eql.unify(t1,t2,cause);
}

void
BTenv::lift_into(C_Decl *f, C_Type *t1, ALocSet *pt1, C_Type *t2,
                 BTcause *cause)
{
  if ( t1->tag == Primitive && t2->tag == Primitive ) {
    add_dep_constraint(t1,t2,cause);
    return ;
  }
  if ( t1->tag == Pointer && all_in_scope(f,pt1,false) ) {
    // it is OK to lift
    add_dep_constraint(t1,t2,cause);
    force_equal(t1->ptr_next(),t2->ptr_next(),cause);
    return ;
  }
  force_equal(t1,t2,cause);
}

void
BTenv::lift_into(C_Decl *f, C_Expr *e1, C_Type *t2, BTcause *cause)
{
  lift_into(f,e1->type,pa.PT[e1],t2,cause);
}

void
BTenv::lift_faithful(C_Decl *f, C_Expr *e1, C_Type *t2, BTcause *cause)
{
  // this emits constraints to ensure that e1 can be lifted into
  // a faithful pattern of t2's binding time
  switch (e1->type->tag) {
  case Primitive:
    add_dep_constraint(e1->type,t2, cause);
    break ;
  case Pointer:
    if ( all_in_scope(f,pa.PT[e1],false) ) {
      add_dep_constraint(e1->type,t2, cause);
      faithful(e1->type->ptr_next(),map[t2], cause);
      break ;
    }
    // else fall through
  default:
    faithful(e1->type,map[t2], cause);
    break ;
  }
}

void
BTenv::faithful(C_Type *t, BTcause* cause)
{
  faithful(t,new BTanonymousVar, cause);
}

void
BTenv::faithful(C_Type *t, BTvariable *nexus, BTcause *cause)
{
  nexus->influence(map[t],cause);

  // we assume that the type is well-formed in itself
  while(1) switch(t->tag) {
  case FunPtr:
  case Pointer:
    t = t->ptr_next() ;
    continue ;
  case Array:
    t = t->array_next() ;
    continue ;
  case Primitive:
  case Abstract:
  case EnumType:
    map[t]->influence(nexus,cause);
    return ;
  case Function: {
    map[t]->influence(nexus,cause);
    foreach(i,t->fun_params(),Plist<C_Type>)
      faithful(*i,nexus,cause);
    faithful(t->fun_ret(),nexus,cause);
    return ; }
  case StructUnion:
    if ( !made_faithful.insert(t) ) {
      map[t]->influence(nexus,cause);
      foreach(i,t->user_types(),Plist<C_Type>)
        faithful(*i,nexus,cause);
    }
    return ;
  }
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  T R A N S L A T I N G    E Q U A L I T Y    C O N S T R A I N T S
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

void
BTequalizer::Union(C_Type *a,C_Type *b)
{
  a = Find(a);
  b = Find(b);
  if ( a != b )
    unionfind[a] = b ;
}

C_Type *
BTequalizer::Find(C_Type *a) {
  C_Type *a0 = a ;
  C_Type *tmp ;

  // find the top
  while ( (tmp=unionfind[a]) != NULL )
    a = tmp ;

  // do path compression
  while ( a0 != a ) {
    tmp = unionfind[a0] ;
    unionfind[a0] = a ;
    a0 = tmp ;
  }
    
  return a ;
}

BTequalizer::BTequalizer(BTmap const& m)
  : unionfind(NULL), map(m)
{
}

void BTequalizer::unify(C_Type *a,C_Type *b,BTcause *cause)
{
  // if there already is a (planned) chain of equivalences
  // between a and b, take it easy and let the others do
  // the hard work for you.
  if ( Find(a) == Find(b) )
    return ;

    // register our decision to unify a and b
  Union(a,b);

  // do one unification step
  map[a]->influence(map[b],cause);
  map[b]->influence(map[a],cause);

    // continue recursively
  if ( a->isBase() ) {
    assert( b->isBase() );
    return ;
  } else
    assert(a->tag == b->tag);
  switch(a->tag) {
  case FunPtr:
  case Pointer:
    unify(a->ptr_next(),b->ptr_next(),cause);
    return ;
  case Array:
    unify(a->array_next(),b->array_next(),cause);
    return ;
  case Function: {
    double_foreach(i,j,a->fun_params(),b->fun_params(),Plist<C_Type>)
      unify(*i,*j,cause);
    unify(a->fun_ret(),b->fun_ret(),cause);
    return ; }
  case StructUnion: {
    double_foreach(i,j,a->user_types(),b->user_types(),Plist<C_Type>)
      unify(*i,*j,cause);
    return ; }
  case Primitive:
  case Abstract:
  case EnumType:
    return ;
  }
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  F I T T I N G   I T   A L L   T O G E T H E R
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

BTenv::BTenv(BTmap &m,PAresult const &p)
  : map(m), pa(p), eql(m)
{
  dynamic = new BTanonymousVar ;
}

void
BTenv::solve()
{
  dynamic->flood(BTvariable::Dynamic);
}

BTresult::BTresult()
  : analysed(false), map(NULL)
{
  map.accept(Numbered::C_TYPE);
  map.accept(Numbered::C_BASICBLOCK);
  map.accept(Numbered::C_DECL);
  map.accept(Numbered::C_ENUMDEF);
  map.accept(Numbered::C_USERMEMB);
}

bool
BTresult::Dynamic(Numbered const*n) const
{
  assert(n != NULL);
  if ( !analysed ) return true ;
  assert(map[n] != NULL);
  return map[n]->isDynamic() ;
}

bool
BTresult::hasBindingTime(Numbered *n) const
{
  assert(n != NULL);
  return map[n] != NULL ;
}

/////////////////////////////////////////////////////////////////////////////

void
binding_time_analysis(C_Pgm const &pgm, PAresult const &pa, BTresult &output)
{
  output.analysed = true ;
  BTenv env(output.map,pa);

  if (btadebug^1) btadebug << "Generating binding-time variables ";
  create_btvars(pgm,output.map);
  if (btadebug^1) btadebug << endl;
    
  if (btadebug^1) btadebug << "Generating constraints ";
  gen_program_constraints(env,pgm);
  if (btadebug^1) btadebug << endl;

  if (btadebug^1) btadebug << "Solving constraints ";
  env.solve();
  if (btadebug^1) btadebug << endl; 
}
