/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Outputting annotated core C
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __OUTCORE__
#define __OUTCORE__

#include "corec.h"
#include "output.h"
#include "Pset.h"
#include "array.h"

class NameMap ;
class Outcore ;

// struct CoreAnnotator
// --------------------
// Derive a class from this to add a new set of annotations to the
// Core C program. Any operator()s it defines must call trough to
// the next CoreAnnotator and add one or more anchors to the anchor
// list. An annotator can call the render() methods in Outcore to
// format e.g. expressions or types for the annotations, and can
// call its crosslink() methods to annotate subtrees of the annotations
// with back links into the original program

struct CoreAnnotator {
  CoreAnnotator& next ;
    
  virtual void operator()(C_Type&,		Plist<Anchor>&,Outcore*);
  virtual void operator()(C_Init&,		Plist<Anchor>&,Outcore*);
  virtual void operator()(C_Decl&,		Plist<Anchor>&,Outcore*);
  virtual void operator()(C_EnumDef&,		Plist<Anchor>&,Outcore*);
  virtual void operator()(C_UserMemb&,	Plist<Anchor>&,Outcore*);
  virtual void operator()(C_UserDef&,		Plist<Anchor>&,Outcore*);
  virtual void operator()(C_UserMemb&,C_Type&,Plist<Anchor>&,Outcore*);
  virtual void operator()(C_Stmt&,		Plist<Anchor>&,Outcore*);
  virtual void operator()(C_Jump&,		Plist<Anchor>&,Outcore*);
  virtual void operator()(C_BasicBlock&,	Plist<Anchor>&,Outcore*);
  virtual void operator()(C_Expr&,bool lval,	Plist<Anchor>&,Outcore*);
  // in the (C_Expr) function, lval is true when annotations
  // for the value pointed to by the expression's value is needed.

  CoreAnnotator(CoreAnnotator &) ;
  virtual ~CoreAnnotator() ;
};

// struct NoCoreAnno
// -----------------
// The only CoreAnnotator that does *not* call a successor; used to
// terminate the annotatior chain

struct NoCoreAnno : CoreAnnotator {
  virtual void operator()(C_Type&,		Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_Init&,		Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_Decl&,		Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_EnumDef&,		Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_UserMemb&,	Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_UserDef&,		Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_UserMemb&,C_Type&,Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_Stmt&,		Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_Jump&,		Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_BasicBlock&,	Plist<Anchor>&,Outcore*) {}
  virtual void operator()(C_Expr&,bool lval,	Plist<Anchor>&,Outcore*) {}
  // in the (C_Expr) function, lval is true when annotations
  // for the value pointed to by the expression's value is needed.

  NoCoreAnno();
  virtual ~NoCoreAnno();
};

// class Outcore
// -------------
// Includes the machinery necessary to format Core C programs as Output
// trees.

class Outcore {
  OType *const our_type ;
    
  multiArray<Anchor*> anchors ;
  Pset<Numbered> orphans ;
  CoreAnnotator &annotate ;
  Output *define(Numbered *,Output *);
  Output *decorate(Plist<Anchor>&,Output *);

  Output *deref(C_Expr&,bool,Plist<Anchor> &,int=0);

public:
  NameMap *the_names ;
  OutputContainer &container ;

  Outcore(OutputContainer &ct,CoreAnnotator &,OType *);
  Output *operator()(C_Pgm &,NameMap&);

  Output *render(C_Type&,Output* =NULL,bool=false);
  Output *render(C_Init&);
  Output *render(C_Decl&);
  Output *render(C_EnumDef&);
  Output *render(C_UserDef&);
  Output *render(C_Stmt&);
  Output *render(C_BasicBlock&);
  Output *render(C_Jump&);
  Output *render(C_Expr&,int=0);
  Output *xref(C_BasicBlock&);
    
  Output *crosslink(Output *,Numbered *);
  Output *crosslink(Output *o,Numbered &n) { return crosslink(o,&n); }
};

#endif
