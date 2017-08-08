/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Sharability analysis
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "analyses.h"
#include "directives.h"
#include "options.h"

//-------------------------------------------------------
//  CODE-SHARING ANALYSIS                        share.cc
//
// This module computes a set of functions that should be
// inlined by the generating extention. This can have
// various reasons, e.g. they contain impure spectime
// actions or it is judged that their residuals would be
// too trivial to warrant a separate residual function.

//-------------------------------------------------------
// Important section: any function that contains a call
// with callmode CallOnceSpectime or a static allocation
// must be unsharable.

void
findImpureSpectime(C_Pgm const &pgm, PAresult const &pa,
                   CGresult const &cgraph, BTresult const &bt,
                   SAresult &result)
{
  // initially no functions are unsharable. The constructor sees to that
  foreach(fun,pgm.functions,Plist<C_Decl>) {
    // don't waste time analysing the function if it is already
    // marked as unsharable
    if ( result.find(*fun) )
      continue ;
    foreach(bb,fun->blocks(),Plist<C_BasicBlock>)
      foreach(s,bb->getBlock(),Plist<C_Stmt>)
      switch(s->tag) {
      case C_Call:
        // Spectime calls to RWstate functions are unsharable.
        // XXX what about calls to ROstate or Stateless functions
        // that may nevertheless side-effect the external state
        // because the explicitly get a pointer to it?
        if ( bt.Static(s->call_expr()->type) ) {
          foreach(fun,*pa.PT[s->call_expr()],ALocSet) {
            if( fun->tag == ExtFun &&
                (fun->effects()==NULL || fun->effects()->state==SERWState) )
              goto thisisunsharable ;
          }
        }
        break ;
      case C_Alloc:
        // Static allocations are unsharable
        if ( bt.Static(s->target()->type->ptr_next()) )
          goto thisisunsharable ;
        break ;
      default:
        break ;
      }
    continue;
  thisisunsharable:
    result.insert(*fun);
    foreach(pred,*cgraph.preds[*fun],Pset<C_Decl>)
      result.insert(*pred);
    continue;
  }
}

//-------------------------------------------------------
// Another important section: a function with a static,
// non-void, return value must be unsharable. This is
// because gegen doesn't yet know how to memoise function
// return values.

static void
findStaticReturns(C_Pgm const &pgm, BTresult const &bt, SAresult &result)
{
  foreach(fun,pgm.functions,Plist<C_Decl>)
    if ( !fun->fun_ret()->isVoid() && bt.Static(fun->fun_ret()) )
      result.insert(*fun);
}

//-------------------------------------------------------
// Now for heuristics. A function will be unsharable if
//  (1) it contains no dynamic conditionals, and
//  (2a) it contains no dynamic assignments, or
//  (2b) it it not recursive and contains no loops
// In theory it should be "safe" (in the sense that it
// only introduces infinite specialization if there are
// statically-controlled infinite recursions without
// dynamic exits) to inline every function that has no
// dynamic conditionals. Rules (2a) and (2b) try to
// restrict inlining to functions that will be "small"
// residually, in an attempt to reduce code blowup.
//   Functions that match (2a) will be *empty* in the
// residual programs, modulo other function calls, and
// are obvious candidates for inlining.
//   Functions that match (2b) can at least be trusted
// not to grow without bounds.

static bool anyCycles(C_BasicBlock *,
                      Pset<C_BasicBlock> &, Pset<C_BasicBlock> &);

static bool
isInliningCandidate(C_Decl *fun, CGresult const &cgraph, BTresult const &bt)
{
  foreach(bb,fun->blocks(),Plist<C_BasicBlock>)
    if ( bb->exit()->tag == C_If &&
         bt.Dynamic(bb->exit()->cond_expr()->type) )
      return false ;
  
  // there aren't any dynamic conditionals. Try (2a) first.
  foreach(bbb,fun->blocks(),Plist<C_BasicBlock>)
    foreach(s,bbb->getBlock(),Plist<C_Stmt>)
    if ( s->tag == C_Assign &&
         bt.Dynamic(s->target()->type->ptr_next()) )
      goto no_match_2a ;
  return true ;
  
 no_match_2a:
  // reject recursive functions
  if ( cgraph.preds[fun]->find(fun) )
    return false ;
  
  Pset<C_BasicBlock> tempa, tempb ;
  return !anyCycles(fun->blocks().front(),tempa,tempb) ;
}

// This function determines if a flow graph is cyclic.
// It is derived from the Dragon Book's description of
// depth-first ordering.
//   We do a depth-first traversal of the flow graph,
// maintaining along the way two sets of *pending* and
// *finished* blocks. Once a block gets marked as
// finished we know that no cycle can be reached from
// that block. The pending blocks are those that are
// ancestors to the one currently processed.
//   At the scanning time each edge can lead to either
// (1) an unprocessed node, which we descend to. If it
//     finished, then all is fine
// (2) a finished node - then we know that the edge
//     cannot be part of cycle.
// (3) a pending node, in which case we've spotted a
//     cycle and abort the search.

static bool
anyCycles(C_BasicBlock *bb,
          Pset<C_BasicBlock> &Pending, Pset<C_BasicBlock> &Finished)
{
  if ( Finished.find(bb) )
    return false ;
  if ( Pending.insert(bb) )
    return true ;
  switch(bb->exit()->tag) {
  case C_Goto:
    if ( anyCycles(bb->exit()->goto_target(),Pending,Finished) )
      return true ;
    break ;
  case C_If:
    if ( anyCycles(bb->exit()->cond_then(),Pending,Finished) ||
         anyCycles(bb->exit()->cond_else(),Pending,Finished) )
      return true ;
    break ;
  case C_Return:
    break ;
  }
  Pending.erase(bb);
  Finished.insert(bb);
  return false ;
}

static void
selectForInlining(C_Pgm const &pgm, CGresult const &cgraph, BTresult const &bt,
                  SAresult &result)
{
  foreach(fun,pgm.functions,Plist<C_Decl>) {
    if ( !result.find(*fun) && isInliningCandidate(*fun,cgraph,bt) )
      result.insert(*fun);
  }
}

void
findUnsharable(C_Pgm const &pgm, PAresult const &pa,
               CGresult const &cgraph, BTresult const &bt,
               SAresult &result)
{
  findImpureSpectime(pgm,pa,cgraph,bt,result);
  findStaticReturns(pgm,bt,result);
  // maybe turn off other cases of inlining for debugging or
  // testing purposes
  if ( !less_inlining )
    selectForInlining(pgm,cgraph,bt,result);
}
