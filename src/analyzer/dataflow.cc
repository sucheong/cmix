/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: In-Use and May/Must Write analyses
 * Docs:     ../doc/dataflow.tex
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "analyses.h"
#include "outanno.h"
#include "options.h"
#include "InvALocSet.h"
#include "commonout.h"
#include "fixiter.h"
#include "strings.h"
#include "Nset.h"
#include "gegen.h"

#define dfdebug debugstream_dataflow

///////////////////////////////
// Generic set constraints. 
// Constraint forms:
//  x := Union(y_i)     (x is the union of y_1, y_2, ...)
//  x := Intersect(y_i) (x is the intersection of y_1, y_2, ...)
//  x := y \ z          (Filter out the elements z from the set y)
//  x >= y              (Put everything in y into x)
//  x =< y              (Remove anything that is not in y from x)
///////////////////////////////

class SetConstraint : public FixpointVertex
{
 protected:
  InvALocSet *const x;        // Target set.
 public:
  bool noinverse ;
  SetConstraint(InvALocSet* t) : x(t) { assert(t); noinverse=false; }
  // Associate the target with this constraint.
  void hookup(array<InvALocSet,SetConstraint*>& setMap) {
    assert(!setMap[x]);
    setMap[x] = this;
  }
  // Set up constraint dependencies.
  virtual void init(array<InvALocSet,SetConstraint*>&) = 0;
};

#define DFDEBUG_START(str) \
if (dfdebug^2) dfdebug << str_lbracket << str << ' ' << this \
                       << str_colon << (void*)x << *x << str_rarrow
#define DFDEBUG_END        if (dfdebug^2) dfdebug << *x << str_rbracket ; \
    assert(!(noinverse && x->isInversed() ))

//  x := Union(y_i)     (x is the union of y_1, y_2, ...)
class MultiUnion : public SetConstraint
{
  Plist<InvALocSet const> ys; // Source sets
 public:
  MultiUnion(InvALocSet* t) : SetConstraint(t) {}
  void add(InvALocSet const* y) { assert(y); ys.push_back(y); }
  virtual bool solve(FixpointSolver& /*fps*/) {
    DFDEBUG_START("MultiUnion");
    // Recalculate the target.
    x->clear();
    foreach( y, ys, Plist<InvALocSet const> ) {
      x->Union(**y);
      if (dfdebug^2) dfdebug << *x << str_rarrow;
    }
    DFDEBUG_END;
    return true;
  }    
  virtual void init(array<InvALocSet,SetConstraint*>& setMap) {
    // Make this constraint a successor of all constraints that
    // produce the sets that this constraint depend on.
    foreach( y, ys, Plist<InvALocSet const> ) {
      SetConstraint* guard = setMap[*y];
      if (guard) *guard >> *this;
    }
  }
};


//  x := Intersect(y_i)     (x is the intersection of y_1, y_2, ...)
class MultiIntersect : public SetConstraint
{
  Plist<InvALocSet const> ys; // Source sets
 public:
  MultiIntersect(InvALocSet* t) : SetConstraint(t) {}
  void add(InvALocSet const* y) { assert(y); ys.push_back(y); }
  virtual bool solve(FixpointSolver& /*fps*/) {
    DFDEBUG_START("MultiIntersect");
    // Recalculate the target.
    x->fill();
    foreach( y, ys, Plist<InvALocSet const> ) {
      x->Intersect(**y);
      if (dfdebug^2) dfdebug << *x << str_rarrow;
    }
    DFDEBUG_END;
    return true;
  }    
  virtual void init(array<InvALocSet,SetConstraint*>& setMap) {
    // Make this constraint a successor of all constraints that
    // produce the sets that this constraint depend on.
    foreach( y, ys, Plist<InvALocSet const> ) {
      SetConstraint* guard = setMap[*y];
      if (guard) *guard >> *this;
    }
  }
};

//  x := y \ z          (Filter out the elements z from the set y)
class Include : public SetConstraint
{
  InvALocSet const* const y; // Base set
  InvALocSet const* const z; // Diff set
 public:
  Include(InvALocSet* t, InvALocSet const* b, InvALocSet const* d)
    : SetConstraint(t), y(b), z(d) {} 
  virtual bool solve(FixpointSolver& /*fps*/) {
    DFDEBUG_START("Include");
    // Recalculate the target.
    x->clear();
    x->Union(*y);
    x->Minus(*z);
    DFDEBUG_END;
    return true;
  }    
  virtual void init(array<InvALocSet,SetConstraint*>& setMap) {
    // Make this constraint a successor of all constraints that
    // produce the sets that this constraint depend on.
    SetConstraint* guardY = setMap[y];
    if (guardY) *guardY >> *this;
    SetConstraint* guardZ = setMap[y];
    if (guardZ) *guardZ >> *this;
  }
};


//  x >= y_i            (Put everything in y_i into x)
class Supersetof : public SetConstraint
{
  Plist<InvALocSet const> ys; // Source set
 public:
  Supersetof(InvALocSet* t) : SetConstraint(t) {}
  void add(InvALocSet const* y) { assert(y); ys.push_back(y); }
  virtual bool solve(FixpointSolver& /*fps*/) {
    DFDEBUG_START("Superset");
    bool same = true;
    foreach( y, ys, Plist<InvALocSet const> ) {
      same &= x->Union(**y);
    }
    DFDEBUG_END;
    return !same;
  }    
  virtual void init(array<InvALocSet,SetConstraint*>& setMap) {
    // Make this constraint a successor of all constraints that
    // produce the sets that this constraint depend on.
    foreach( y, ys, Plist<InvALocSet const> ) {
      SetConstraint* guard = setMap[*y];
      if (guard) *guard >> *this;
    }
  }
};

//  x =< y_i            (Remove anything that is not in y_i from x)
class SubsetOf : public SetConstraint
{
  Plist<InvALocSet const> ys; // Source set
 public:
  SubsetOf(InvALocSet* t) : SetConstraint(t) {}
  void add(InvALocSet const* y) { assert(y); ys.push_back(y); }
  virtual bool solve(FixpointSolver& /*fps*/) {
    DFDEBUG_START("MultiIntersect");
    bool same = true;
    foreach( y, ys, Plist<InvALocSet const> ) {
      same &= x->Intersect(**y);
    }
    DFDEBUG_END;
    return !same;
  }    
  virtual void init(array<InvALocSet,SetConstraint*>& setMap) {
    // Make this constraint a successor of all constraints that
    // produce the sets that this constraint depend on.
    foreach( y, ys, Plist<InvALocSet const> ) {
      SetConstraint* guard = setMap[*y]; // XXX: *y gives nonsense!
      if (guard) *guard >> *this;
    }
  }
};



DFresult::DFresult()
  : Wdef(NULL), Wmay(NULL), Use(NULL) {
  // Some of these are just temporary (while dfdebugging)
  Wdef.accept(Numbered::C_DECL);
  Wdef.accept(Numbered::C_STMT);
  Wdef.accept(Numbered::C_JUMP);
  Wdef.accept(Numbered::C_BASICBLOCK);
  Wmay.accept(Numbered::C_DECL);
  Wmay.accept(Numbered::C_STMT);
  Wmay.accept(Numbered::C_JUMP);
  Wmay.accept(Numbered::C_BASICBLOCK);
  Use.accept(Numbered::C_DECL);
  Use.accept(Numbered::C_STMT);
  Use.accept(Numbered::C_JUMP);
  Use.accept(Numbered::C_BASICBLOCK);
}

struct DFenv {
  DFresult& dfResult;
  // InvALocSets are mapped to their defining constraint through this map.
  array<InvALocSet,SetConstraint*> defMap;
  array<InvALocSet,SetConstraint*> mayMap;
  array<InvALocSet,SetConstraint*> useMap;
  Pset<SetConstraint> allWdefs;
  Pset<SetConstraint> allWmays;
  Pset<SetConstraint> allUses;
  // The result of the points-to analysis.
  PAresult const& paResult;
  // The result of the oneObject analysis.
  ALocSet const& oneObject;
  // The Wdef constraint solver.
  FixpointSolver WdefSolver;
  // The Wmay constraint solver.
  FixpointSolver WmaySolver;
  // The Use constraint solver.
  FixpointSolver UseSolver;

  // Construct the environment from the results passed to this analysis.
  DFenv(DFresult& df, PAresult const& pa, ALocSet const& ones)
    : dfResult(df), defMap(NULL), mayMap(NULL), useMap(NULL), 
      paResult(pa), oneObject(ones) {} 

  // Three functions for adding a new constraint (Wdef,Wmay,Use). They
  // first associate the target set with the constraint, and then put
  // the constraint in the right solver.
  void addWdefConstraint(SetConstraint* c) {
    if (dfdebug^2) dfdebug << "[addWdef " << c << ']';
    c->hookup(defMap);
    WdefSolver += c;
    allWdefs.insert(c);
  }
  void addWmayConstraint(SetConstraint* c) {
    if (dfdebug^2) dfdebug << "[addWmay " << c << ']' ;
    c->hookup(mayMap);
    WmaySolver += c;
    c->noinverse = true ;
    allWmays.insert(c);
  }
  void addUseConstraint(SetConstraint* c) {
    if (dfdebug^2) dfdebug << "[addUse " << c << ']';
    c->hookup(useMap);
    UseSolver += c;
    c->noinverse = true ;
    allUses.insert(c);
  }

  void solve() {
    if (dfdebug^2) dfdebug << "\n    Initializing Wdef";
    foreach( c, allWdefs, Pset<SetConstraint> ) {
      if (dfdebug^2) dfdebug << '.';
      c->init(defMap);
    }
    if (dfdebug^2) dfdebug << endl << "    solving";
    WdefSolver.solve();
    if (dfdebug^2) dfdebug << "\n    Initializing Wmay";
    foreach( cc, allWmays, Pset<SetConstraint> ) {
      if (dfdebug^2) dfdebug << '.';
      cc->init(mayMap);
    }
    if (dfdebug^2) dfdebug << endl << "    solving";
    WmaySolver.solve(); 
    if (dfdebug^2) dfdebug << "\n    Initializing Use";
    foreach( ccc, allUses, Pset<SetConstraint> ) {
      if (dfdebug^2) dfdebug << '.';
      ccc->init(useMap);
    }
    if (dfdebug^2) dfdebug << endl << "    solving";
    UseSolver.solve();
  }
};


// Lookup a T set. If non exists, create one.
#define GETSET(T) InvALocSet*             \
DFresult::get##T##Set(Numbered const* n)  \
{                                         \
  InvALocSet* set = T[n];                 \
  if (!set) {                             \
    set = new InvALocSet;                 \
    T[n] = set;                           \
  }                                       \
  return set;                             \
}

GETSET(Wdef) // defines DFresult::getWdefSet
GETSET(Wmay) // defines DFresult::getWMaySet
GETSET(Use)  // defines DFresult::getUseSet

#undef GETSET

Pset<C_Decl> const &
DFresult::GetMemoSet(C_BasicBlock *bb) const
{
  InvALocSet const *set = Use[bb] ;
  assert(!set->isInversed());
  return set->theSet();
}

Pset<C_Decl> const &
DFresult::GetReadSet(C_Decl *fun) const
{
  InvALocSet const *set = Use[fun] ;
  assert(!set->isInversed());
  return set->theSet();
}

Pset<C_Decl> const &
DFresult::GetWriteSet(C_Decl *fun) const
{
  InvALocSet const *set = Wmay[fun] ;
  assert(!set->isInversed());
  return set->theSet();
}


// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Treatment of points-to sets.
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

// Return the set if it contains _precise_ information, else Ø.
static ALocSet*
one(ALocSet const& aset, ALocSet const& oneObjectDecls)
{
  ALocSet* one_set = new ALocSet;
  // If the size of the set is 1, it posibly represents a unique object.
  if ( aset.size() == 1 ) {
    C_Decl* d = *aset.begin();
    if (oneObjectDecls.find(d)) {
      // OK, the declaration represents only one object. Return this object.
      one_set->insert(d);
    }
  }
  return one_set;
}

static void
collectSubObjects(C_Decl* d, InvALocSet& subObjects)
{
  assert(d->tag == VarDcl);
  switch(d->type->tag) {
  case Abstract:
  case Primitive:
  case EnumType:
  case Pointer:
  case FunPtr:
    // All the above have no subdeclarations.
    subObjects.insert(d);
    break;
  case Function:
    assert(0);
    break;
  case Array:
  case StructUnion:
    // Struct/Unions/Arrays constain subdeclarations.
    subObjects.insert(d);
    foreach( mem, d->members(), Plist<C_Decl> ) {
      collectSubObjects(*mem, subObjects);
    }
    break;
  }
}

static InvALocSet*
collectSubObjects(ALocSet const* init)
{
  InvALocSet* subObjects = new InvALocSet;
  foreach( d, *init, ALocSet ) {
    if ( d->tag == VarDcl )
      collectSubObjects( *d, *subObjects );
  }
  return subObjects;
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Generation of constraints.
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

static InvALocSet*
DFexpr(C_Expr* e, DFenv& env)
{
  InvALocSet* res = NULL;
  switch (e->tag) {
  case C_ExtFun:
  case C_FunAdr:
  case C_Var:
  case C_EnumCnst:
  case C_Cnst:
  case C_Null:
  case C_SizeofT:
  case C_SizeofE:
    res = new InvALocSet;
    break;
  case C_Unary:
  case C_Member:
  case C_Array:
  case C_Cast:
    res = DFexpr(e->subexpr(),env);
    break;
  case C_PtrArith:
  case C_PtrCmp:
  case C_Binary: {
    // Union the subresults, and throw away one of them.
    res = DFexpr(e->binary_expr1(),env);
    InvALocSet* tmp = DFexpr(e->binary_expr2(),env);
    res->Union(*tmp);
    delete tmp;
    break; }
  case C_DeRef:
    // Union the subresult with the (closure of) the points-to information.
    res = DFexpr(e->subexpr(),env);
    res->Union(*collectSubObjects(env.paResult.PT[e->subexpr()]));
    break;
  }
  assert(!res->isInversed());
  return res;
}


static InvALocSet*
DFstmt(C_Stmt *s, DFenv& env, InvALocSet const* postUse)
{
  if (dfdebug^2) dfdebug << "[stmt " << s->pos << "]";
  // Wdef set ///////////
  InvALocSet* Wdef_init = env.dfResult.getWdefSet(s);
  // Use set = Use(target) 
  InvALocSet* Use_init = env.dfResult.getUseSet(s);
  Use_init->clear();
  // Use_tmp = Ø
  InvALocSet* Use_tmp_init = new InvALocSet;
  // Use_tmp := PostUse(s) \ Wdef(s)
  Include* Use_tmp = new Include(Use_tmp_init, postUse,
                              env.dfResult.getWdefSet(s)); 
  env.addUseConstraint(Use_tmp);
  // Use(s) >= Use_tmp
  Supersetof* Use_s = new Supersetof(Use_init);
  env.addUseConstraint(Use_s);
  Use_s->add(Use_tmp_init);
  // Wmay set = subs(PT(e))
  InvALocSet* Wmay_init = env.dfResult.getWmaySet(s);
  Wmay_init->clear();
  if ( s->hasTarget() ) {
    // Use
    InvALocSet* Use_target = DFexpr(s->target(),env);
    Use_init->Union(*Use_target);
    delete Use_target;
    // Wmay    
    Wmay_init->Union(*collectSubObjects(env.paResult.PT[s->target()]));
  }
  switch (s->tag) {
  case C_Assign: {
    // Wdef constraints ///////////
    Wdef_init->clear();
    if ( s->hasTarget() ) {
      // Wdef = subs(one(PT(e)))
      //InvALocSet* subObjs = 
      Wdef_init->Union(*collectSubObjects(one(*env.paResult.PT[s->target()],env.oneObject)));
    }
    // Use constraints ///////////
    // Use(s) += Use(assign_expr).
    InvALocSet* Use_rhs = DFexpr(s->assign_expr(),env);
    Use_init->Union(*Use_rhs);
    delete Use_rhs;
    break; }
  case C_Call: {   // call of form [x =] (e_0)(e_1,...,e_n)
    // Wdef constraints ///////////
    // Wtmp = D
    InvALocSet* Wtmp = new InvALocSet;
    Wtmp->fill() ;
    // Wtmp =< Wdef(d_i), forall d_i in e_f
    SubsetOf* Wdef_X = new SubsetOf(Wtmp);
    env.addWdefConstraint(Wdef_X);
    foreach( d, *env.paResult.PT[s->call_expr()], ALocSet ) {
      // Get the Wdef set for the function d.
      switch (d->tag) {
      case ExtState:
      case ExtFun: 
        // An external function definitely-kills nothing.
        Wdef_X->add(new InvALocSet);
        break;
      case VarDcl:
        assert(0);
        break;
      case FunDf: 
        InvALocSet* Wdef_d = env.dfResult.getWdefSet(*d);
        Wdef_X->add(Wdef_d);
        break; } 
    }
    // Wdef = D
    Wdef_init->fill();
    // Wdef := Union{subs(one(PT(e))),Wtmp}
    MultiUnion* Wdef_s = new MultiUnion(Wdef_init);
    env.addWdefConstraint(Wdef_s);
    Wdef_s->add(Wtmp);
    if ( s->hasTarget() ) {
      // Wdef >= subs(one(PT(e)))
      Wdef_s->add(collectSubObjects(one(*env.paResult.PT[s->target()],env.oneObject)));
    }
    // Use constraints ///////////
    // Use(s) += Use(e_f).
    InvALocSet* Use_e_f = DFexpr(s->call_expr(),env);
    Use_init->Union(*Use_e_f);
    delete Use_e_f;
    // Use(s) += Use(e_i).
    foreach( e, s->call_args(), Plist<C_Expr> ) {
      InvALocSet* Use_e = DFexpr(*e,env);
      Use_init->Union(*Use_e);
      delete Use_e;
    }
    // Use(s) >= Use(f_i), forall f_i in PT(s.call_expr).
    // Wmay(s) >= Wmay(f_i), forall f_i in PT(s.call_expr).
    Supersetof* Wmay_s = new Supersetof(Wmay_init);
    env.addWmayConstraint(Wmay_s);
    foreach( f, *env.paResult.PT[s->call_expr()], ALocSet ) {
      switch (f->tag) {
      case ExtState:
        // XXX what happens when we call a function pointer got from
        //     another external function? For now, nothing.
        break ;
      case ExtFun: {
        convertALocSet(env.paResult.ExtfunReading[*f],*Use_init);
        convertALocSet(env.paResult.ExtfunWriting[*f],*Wmay_init);
        break; }
      case VarDcl:
        assert(0); // Cannot happen.
        break;
      case FunDf: {
	// Use
	InvALocSet* Use_f = env.dfResult.getUseSet(*f);
	Use_s->add(Use_f);
	// Wmay
	InvALocSet* Wmay_f = env.dfResult.getWmaySet(*f);
	Wmay_s->add(Wmay_f);
        break; }
      }
    }
    break; }
  case C_Alloc: {
    // Wdef constraints ///////////
    Wdef_init->clear();
    if ( s->hasTarget() ) {
      // Wdef = subs(one(PT(e)))
      Wdef_init->Union(*collectSubObjects(one(*env.paResult.PT[s->target()],env.oneObject)));
    }
    // Use constraints ///////////
    // Use(s) >= (size expression of d)
    C_Type* array_type = s->alloc_objects()->type;
    if ( array_type->hasSize() ) {
      InvALocSet* Use_size = DFexpr(array_type->array_size(),env);
      Use_init->Union(*Use_size);
      delete Use_size;
    }
    break; }
  case C_Free: {   // deallocation.
    // Wdef constraints ///////////
    Wdef_init->clear();
    break; }
  case C_Sequence: // sequence point
    // Wdef constraints ///////////
    Wdef_init->clear();
    break;
  }
  // Return Wdef(s)
  return Wdef_init;
}

static InvALocSet*
DFjump(C_Jump *j, DFenv& env)
{
  if (dfdebug^2) dfdebug << "[jump " << j->pos << "]";
  InvALocSet* Wdef_init = env.dfResult.getWdefSet(j);
  InvALocSet* Use_init = env.dfResult.getUseSet(j);
  InvALocSet* Wmay_init = env.dfResult.getWmaySet(j);
  Wmay_init->clear();
  switch (j->tag) {
  case C_If: {
    // Wdef constraints ///////////
    // Wdef = D
    Wdef_init->fill();
    // Wdef =< Wdef(b_then), Wdef =< Wdef(b_else)
    SubsetOf* Wdef_j = new SubsetOf(Wdef_init);
    env.addWdefConstraint(Wdef_j);
    Wdef_j->add(env.dfResult.getWdefSet(j->cond_then()));
    Wdef_j->add(env.dfResult.getWdefSet(j->cond_else()));
    // Use constraints ///////////
    // Use(j) = Use(e).
    Use_init->clear();
    Use_init->Union(*DFexpr(j->cond_expr(),env));
    // Use(j) >= Use(b_then), Use(j) >= Use(b_else).
    Supersetof* Use_j = new Supersetof(Use_init);
    env.addUseConstraint(Use_j);
    Use_j->add(env.dfResult.getUseSet(j->cond_then()));
    Use_j->add(env.dfResult.getUseSet(j->cond_else()));
    // Wmay >= Wmay(b_then), Wmay >= Wdef(b_else)
    Supersetof* Wmay_j = new Supersetof(Wmay_init);
    env.addWmayConstraint(Wmay_j);
    Wmay_j->add(env.dfResult.getWmaySet(j->cond_then()));
    Wmay_j->add(env.dfResult.getWmaySet(j->cond_else()));
    break; }
  case C_Goto: {
    // Wdef constraints ///////////
    // Wdef = D
    Wdef_init->fill();
    // Wdef =< Wdef(b)
    SubsetOf* Wdef_s = new SubsetOf(Wdef_init);
    env.addWdefConstraint(Wdef_s);
    Wdef_s->add(env.dfResult.getWdefSet(j->goto_target()));
    // Use constraints ///////////
    // Use(j) = Ø.
    Use_init->clear();
    // Use(j) >= Use(b).
    Supersetof* Use_j = new Supersetof(Use_init);
    env.addUseConstraint(Use_j);
    Use_j->add(env.dfResult.getUseSet(j->goto_target()));
    // Wmay >= Wmay(b)
    Supersetof* Wmay_j = new Supersetof(Wmay_init);
    env.addWmayConstraint(Wmay_j);
    Wmay_j->add(env.dfResult.getWmaySet(j->goto_target()));
    break; }
  case C_Return: {
    // Wdef constraints ///////////
    // Wdef = Ø
    Wdef_init->clear();
    // Use constraints ///////////
    // Use(j) = Use(e).
    Use_init->clear();
    if (j->hasExpr()) {
      Use_init->Union(*DFexpr(j->return_expr(),env));
    }
    break; }
  }
  // Return Wdef(j)
  return Wdef_init;
}
 
static InvALocSet*
nextUseSet(Plist<C_Stmt>::iterator si, C_Jump* j, DFenv& env)
{
  // Get the next use-set for *si.
  if (si) {
    si++;
    if (si) {
      return (env.dfResult.getUseSet(*si));
    }
  }
  return (env.dfResult.getUseSet(j));
}
 
static void
DFfun(C_Decl *fun, DFenv& env)
{
  assert( fun->tag == FunDf );
  // A function must have a first block.
  C_BasicBlock* first_bb = fun->blocks().front();
  // Wdef(fun) = D
  InvALocSet* Fset = env.dfResult.getWdefSet(fun);
  Fset->fill();

  // Wdef(fun) := Wdef(b_1) \ truelocals(fun)
  InvALocSet* TL_F = new InvALocSet;
  convertALocSet(env.paResult.TrulyLocals[fun], *TL_F);
  InvALocSet* Wdef_b_1 = env.dfResult.getWdefSet(first_bb);
  Include* Wdef_fun = new Include(Fset, Wdef_b_1, TL_F);
  env.addWdefConstraint(Wdef_fun);

  // Wmay(fun) = Ø
  InvALocSet* Wmay_init = env.dfResult.getWmaySet(fun);
  Wmay_init->clear();
  // Wmay(fun) := Wmay(b_1) \ truelocals(fun)
  InvALocSet * Wmay_b_1 = env.dfResult.getWmaySet(first_bb);
  Include* Wmay_fun = new Include(Wmay_init, Wmay_b_1, TL_F);
  env.addWmayConstraint(Wmay_fun);  

  // Use(fun) = Ø
  InvALocSet* Use_init = env.dfResult.getUseSet(fun);
  Use_init->clear();
  // Use(fun) >= Use(b_1)
  InvALocSet * Use_b_1 = env.dfResult.getUseSet(first_bb);
  Supersetof* Use_fun = new Supersetof(Use_init);
  env.addUseConstraint(Use_fun);
  Use_fun->add(Use_b_1);
  // Pass the post-use set to each statement, ie the use-set of the
  // subsequent statement/jump.
  foreach( bb, fun->blocks(), Plist<C_BasicBlock> ) {
    // Wdef(bb) = D
    InvALocSet* wdefset = env.dfResult.getWdefSet(*bb);
    wdefset->fill();
    // Wdef(bb) := Union{Wdef(s_1),...,Wdef(j)}
    MultiUnion* Wdef_bb = new MultiUnion(wdefset);
    env.addWdefConstraint(Wdef_bb);

    // Wmay(bb) = Ø
    InvALocSet* Wmay_bb_init = env.dfResult.getWmaySet(*bb);
    Wmay_bb_init->clear();
    // Wmay(bb) >= Wmay(s_1), ... , Wmay(bb) >= Wmay(j)
    Supersetof* Wmay_bb = new Supersetof(Wmay_bb_init);
    env.addWmayConstraint(Wmay_bb);

    // Use(bb) = Ø
    InvALocSet* Use_bb_init = env.dfResult.getUseSet(*bb);
    Use_bb_init->clear();
    // Use(bb) >= Use(s_1)
    Supersetof* Use_bb = new Supersetof(Use_bb_init);
    env.addUseConstraint(Use_bb);
    Plist<C_Stmt>::iterator first_stmt = bb->getBlock().begin();
    if (first_stmt) {
      Use_bb->add(env.dfResult.getUseSet(*first_stmt));
    }
    else {
      // No statements: Use(bb) >= Use(j)
      Use_bb->add(env.dfResult.getUseSet(bb->exit()));
    }      
    foreach(stmt,bb->getBlock(),Plist<C_Stmt>) {
      // Wdef constraints ///////////
      Wdef_bb->add(DFstmt(*stmt, env, nextUseSet(stmt,bb->exit(),env)));
      // Use constraints are also added by DFstmt
      // Wmay 
      Wmay_bb->add(env.dfResult.getWmaySet(*stmt));
    }
    Wdef_bb->add(DFjump(bb->exit(), env));
    Wmay_bb->add(env.dfResult.getWmaySet(bb->exit()));
  } 
}


void
DFresult::Analyse(C_Pgm const& pgm, PAresult const& pa, ALocSet const& ones)
{
  if (dfdebug^1) dfdebug << "  Generating dataflow constraints";
  // Make the environment that is passed around.
  DFenv env(*this, pa, ones);
  // Traverse the program.
  foreach( fun, pgm.functions, Plist<C_Decl> ) {
    DFfun(*fun,env);
  }
  foreach( fun2, pgm.generators, Plist<C_Decl> ) {
    DFfun(*fun2,env);
  }
  if (dfdebug^1) dfdebug << "\n  Solving dataflow constraints";
  env.solve();
  if (dfdebug^1) dfdebug << endl;  
}

/////////////////////////////////////////////////////////////////////////////

OutDf::OutDf(DFresult const&d, CoreAnnotator *nxt)
  : CoreAnnotator(*nxt), data(d)
{
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  Annotation generation.
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

void
OutDf::operator()(C_Decl &d, Plist<Anchor> &alist, Outcore *oc)
{
  if ( d.tag==FunDf ) {
    InvALocSet* Wdef = data.Wdef[d];
    assert(Wdef);
    alist.push_back(outInvALocSet("The function will definitely kill",Wdef,OKill,oc));
    InvALocSet* Use = data.Use[d];
    assert(Use);
    alist.push_back(outInvALocSet("The function will use",Use,ORead,oc));
    InvALocSet* Wmay = data.Wmay[d];
    assert(Wmay);
    alist.push_back(outInvALocSet("The function may kill",Wmay,OWrite,oc));
  }
  next(d,alist,oc);
}

void
OutDf::operator()(C_BasicBlock &bb, Plist<Anchor> &alist, Outcore *oc)
{
  InvALocSet* Wdef = data.Wdef[bb];
  assert(Wdef);
  alist.push_back(outInvALocSet("Definitely kill",Wdef,OKill,oc));
  InvALocSet* Use = data.Use[bb];
  assert(Use);
  alist.push_back(outInvALocSet("In use:",Use,ORead,oc));
  InvALocSet* Wmay = data.Wmay[bb];
  assert(Wmay);
  alist.push_back(outInvALocSet("May kill",Wmay,OWrite,oc));
  
  next(bb,alist,oc);
}

void
OutDf::operator()(C_Stmt &s, Plist<Anchor> &alist, Outcore *oc)
{
  if (dfdebug) {
    InvALocSet* Wdef = data.Wdef[s];
    assert(Wdef);
    alist.push_back(outInvALocSet("Definitely kill",Wdef,OKill,oc));
    InvALocSet* Use = data.Use[s];
    assert(Use);
    alist.push_back(outInvALocSet("In use:",Use,ORead,oc));
    InvALocSet* Wmay = data.Wmay[s];
    assert(Wmay);
    alist.push_back(outInvALocSet("May kill",Wmay,OWrite,oc));
  }
  next(s,alist,oc);
}

void
OutDf::operator()(C_Jump &s, Plist<Anchor> &alist, Outcore *oc)
{
  if (dfdebug) {
    InvALocSet* Wdef = data.Wdef[s];
    assert(Wdef);
    alist.push_back(outInvALocSet("Definitely kill",Wdef,OKill,oc));
    InvALocSet* Use = data.Use[s];
    assert(Use);
    alist.push_back(outInvALocSet("In use:",Use,ORead,oc));
    InvALocSet* Wmay = data.Wmay[s];
    assert(Wmay);
    alist.push_back(outInvALocSet("May kill",Wmay,OWrite,oc));
  }
  next(s,alist,oc);
}

