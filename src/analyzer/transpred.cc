/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Calculation of predecessor calls, transitively.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "fixiter.h"
#include "corec.h"
#include "ALoc.h"
#include "options.h"
#include "analyses.h"

/* This module traverses the program and creates the set of caller/callee
   relations for functions. For each function f, the transitive closurse of
   functions that call f is calculated by fixpoint iteration.
*/

#define debug (debugstream_cg)

// A PredConstraint represents a function. It contains a reference to the
// function's predecessor set. The successors of a constraint for function f
// are the constraints for the functions that f calls.
class PredConstraint : public FixpointVertex
{
  ALocSet& preds; // The predecessor set for the function.
  Plist<ALocSet> succs; // All functions called from the function.
public:
  PredConstraint(ALocSet& c) : preds(c) {}
  virtual bool solve(FixpointSolver& fps) {
    // Add all predecessors of the function to the predecessor set of
    // all successors.
    bool changed = false;
    foreach(callee, succs, Plist<ALocSet>) {
      if (debug^2) debug << '[' << **callee;
      count_t szOfCalleeSet = callee->size();
      **callee += preds;
      count_t insertedIntoCalleeSet = callee->size() - szOfCalleeSet;
      assert(insertedIntoCalleeSet >= 0);
      if (debug^2) debug << "->" << **callee << ']';
      if (insertedIntoCalleeSet > 0) changed = true;
    }
    return changed;
  }
  // Add a successor.
  void operator >> (PredConstraint& pd) {
    succs.push_front(&pd.preds);
    FixpointVertex::operator>>(pd);
  }
};

static ALocSet& getSet(C_Decl*, array<C_Decl,ALocSet*>&);
static void DepthFirstTraverseCalls(FixpointSolver&,
			     array<C_Decl,PredConstraint*>&, 
			     array<C_Decl,ALocSet*>&,
			     array<C_Expr,ALocSet*>&, C_Decl*,
			     C_Decl*); 


void mkCallGraph(C_Pgm const& pgm, PAresult& paresult,
		 CGresult& cgresult)
{
  FixpointSolver fps;   // The fixpoint solver.
  array<C_Decl,PredConstraint*> visited(NULL);
  // Go depth first through function calls and make caller/callee
  // constraints.
  foreach(fun, pgm.functions, Plist<C_Decl>) {
    DepthFirstTraverseCalls(fps, visited, cgresult.preds, paresult.PT,
			    *fun, NULL); 
  }
  fps.solve();
  
  foreach(fun2, pgm.generators, Plist<C_Decl>)
      cgresult.preds[*fun2] = new ALocSet ;
}

void DepthFirstTraverseCalls(FixpointSolver& fps,
			     array<C_Decl,PredConstraint*>& visited,
			     array<C_Decl,ALocSet*>& predSets,
			     array<C_Expr,ALocSet*>& PTSet,
			     C_Decl* fun,
			     C_Decl* predfun)
{
  if (debug^2) debug << "[" << fun->get_name() << " calls ";
  // Get the constraint that represents the predecessor set for the
  // function. 
  PredConstraint* pred = visited[fun];
  ALocSet& callerSet = getSet( fun, predSets );
  if (!pred) {
    // The function has no been visited. Get the predecessor set from
    // the final result package and put this in a constraint.
    pred = new PredConstraint(callerSet);
    visited[fun] = pred;
    fps += pred;
    // Go through the basic blocks of the body.
    foreach(block, fun->blocks(), Plist<C_BasicBlock>) {
      // Go through the statements and find calls to other
      // functions.
      foreach(stmt, block->getBlock(), Plist<C_Stmt>) {
	if (stmt->tag == C_Call) {
	  // Make a constraint for each possible callee and insert
	  // the caller in the predecessor set for the callee.
	  foreach(callee, *PTSet[stmt->call_expr()], ALocSet){
	    // Recurse through the calls; exclude external calls
            if ( callee->tag == FunDf ) {
              if (debug^2) debug << callee->get_name() << " ";
              DepthFirstTraverseCalls(fps, visited, predSets, PTSet,
                                      *callee, fun);
            }
            else
              if (debug^2) debug << "<ext> ";
	  }
	}
      }
    }
  }
  // Make the functions predecessor set a successor of the caller.
  if (predfun) { // There might not be a caller.
    // Get the constraint for the caller.
    PredConstraint* constr = visited[predfun];
    assert(constr);
    *constr >> *pred;
    callerSet.insert(predfun);
  }
  if (debug^2) debug << "]";
}

// Make sure a function has a predecessor set.
ALocSet& getSet( C_Decl* fun, array<C_Decl,ALocSet*>& predSets )
{
  assert(fun);
  ALocSet* preds = predSets[fun];
  if (preds)
    return *preds;
  else {
    ALocSet* emptySet = new ALocSet;
    predSets[fun] = emptySet;
    return *emptySet;
  }
}
