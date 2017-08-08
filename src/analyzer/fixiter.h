/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Fixpoint solver, version 2
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __FIX2__
#define __FIX2__

#include "auxilary.h"
#include "Pset.h"

// This module implements fixpoint iteration on dynamic graphs.
//
// The abstract algorithm is quite general. Given a directed
// graph (V,E), and some notion that a change in one node
// may need to be followed by changes in its successors, we
// maintain a set of "dirty" vertices D and iterate thus:
//
//   while ( D is not empty ) {
//      select v from D ;
//      D = D \ {v} ;
//      update v ;
//      if ( v changed because of the update )
//          D = D + { v' in V | v>>v' in E }
//   }
//
// The tricky thing is that new nodes and edges can be added
// to the graph on the fly - that is, the 'update' operation
// may have the side effect of adding elements to D or E.
//
// The scheduling strategy is in each step to select the v
// that has been least recently updated (*not* least recently
// put into D). Weise and Kanamori [1] report that this
// strategy generally reaches a fixpoint in less than a third
// iterations than "information-free" strategies such as LIFO
// and FIFO need.
//
// Usage
// -----
//
// Vertices are represented by deriviates of the FixpointVertex
// class. They need to implement a solve() member function
// that returns true if the vertex "changed" by the solution
// step.
//
// New edges are added by
//    v1 >> v2 ;
//
// New vertices can are added to D by the += operator
// of the solver object.
//
// Alternatively vertices can be derived from the FixpointPureVertex
// class. FixpointPureVertex must implement Solve() rather than
// solve(). Solve() methods must take care of inserting successors
// in the solving engine via the += operator themselves.
// A FixpointPureVertex cannot be the *right* operand of >>.
//
// [1] Kanamori, Weise:
// Worklist Management Strategies for Dataflow Analysis
// Technical Report MSR-TR-94-12, Microsoft Research

class FixpointSolver ;

class FixpointPureVertex : public Numbered {
  friend class FixpointSolver ;
  friend bool Fixpoint_lesseq(yg_datum,yg_datum) ;
  FixpointSolver *owner ;
  unsigned long stamp ;
  bool is_ajour ;
public:
  FixpointPureVertex() ;
  virtual void Solve(FixpointSolver&) =0 ;
  virtual ~FixpointPureVertex() ;
} ;

class FixpointVertex : public FixpointPureVertex {
  Pset<FixpointPureVertex> successors ;
  virtual void Solve(FixpointSolver&);
public:
  FixpointVertex() {}
  virtual bool solve(FixpointSolver&) =0 ;
  // returns true if the element is already in the set.
  bool operator >> (FixpointPureVertex&) ;
  virtual ~FixpointVertex() ;
} ;

class FixpointSolver {
  yg_tree worklist ;
  unsigned long clock ;
public:
  FixpointSolver();
  // returns true if the element is already in the worklist.
  bool operator += (FixpointPureVertex *);
  void solve();
  bool empty() { return worklist.size() == 0; }
} ;

#endif
