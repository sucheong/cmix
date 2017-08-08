/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: definition of class PAconstraint
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __PA__
#define __PA__

#include "fixiter.h"

class PA_Set ;
class C_Decl ;

class PAconstraint : public FixpointPureVertex {
public:
  ~PAconstraint();
protected:
  PAconstraint();

  void depend_on(PA_Set const*set);
  virtual void solve() =0 ;

private:
  static Pset<PA_Set> changedsets ;
  virtual void Solve(FixpointSolver &);
  
protected:
  void insertOne(PA_Set *to, C_Decl *d);
  void insertMany(PA_Set *to, PA_Set const* from);
  void assignmentTransfer( C_Decl *target, C_Decl *source );
} ;

#endif
