/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Prototypes for analysis phases
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __ANALYSES__
#define __ANALYSES__

#include "corec.h"
#include "array.h"
#include "ALoc.h"
#include "InvALocSet.h"
#include <iostream.h>

class GegenStream;

//-------------------------------------------------------
//  POINTER ANALYSIS                                pa.cc
//
// This module calculates, for each expression and
// pointer-typed declaration in the program, a
// conservative (i.e., possibly too wide) approximation
// of the set of ALocs it may point to.

struct PAresult {
  array<C_Expr,ALocSet*> PT;
  array<C_Decl,ALocSet*> PTvar; // for functions: PT set for return values
  array<C_Decl,ALocSet*> ExtfunReading;
  array<C_Decl,ALocSet*> ExtfunWriting;
  array<C_Decl,ALocSet*> TrulyLocals;
  ALocSet freedHeapObjects;
  ALocSet sideEffectedObjects;
  PAresult();
};

void pointsToAnalysis(C_Pgm const&, PAresult&);

//-------------------------------------------------------
//  INITIALIZER FINISHING                         init.cc
//
// This module substitutes assignments for initializers
// for those local variables that are initialized and
// also suffer normal side effects.

void SubstInitializers(C_Pgm &, PAresult &);

//-------------------------------------------------------
//  GALL GRAPH                                      cg.cc
//
// This module calculates, for each function, the set of
// functions that (perhaps transitively) calls it.

struct CGresult {
  array<C_Decl,ALocSet*> preds;
  inline CGresult() : preds(NULL) {}
};

void mkCallGraph(C_Pgm const&, PAresult&, CGresult&);

//-------------------------------------------------------
//  TRULY LOCAL                               closures.cc
//
// This module UPDATES the points-to set so that each
// function in the program maps to the set of local
// variables and parameters that cannot be aliased to
// earlier recursive instances of the function
// (elements in eglible local arrays are included in the
// set, though).
// The ALocSet (last parameter) will contain all declarations that represent a
// single object.
void TrulyLocal(C_Pgm const&,CGresult const&,
                PAresult&, ALocSet&);

//-------------------------------------------------------
//  BINDING-TIME ANALYSIS                          bta.cc
//
// This module creates binding times according to the
// pointer information passed to it.
//
// There are "binding times" for
// - C_Type: ordinary binding times
//   (summary binding time for StructUnion types)
// - C_Decl/function: ever called speculatively?
// - C_BasicBlock: block under dynamic control?
// - C_EnumDef: any constant cannot be spectime?

class Diagnostic ;
class BTvariable ;
struct BTresult {
    bool analysed ;
    multiArray<BTvariable*> map ;

    BTresult();
    bool Dynamic(Numbered const*) const;
    inline bool Dynamic(Numbered const&t) const { return Dynamic(&t); }
    inline bool Static(Numbered const*t) const { return !Dynamic(t); }
    inline bool Static(Numbered const&t) const { return !Dynamic(&t); }

    bool hasBindingTime(Numbered *) const;
    
    void Trace(Diagnostic &,Numbered *) const;
    
    void markDynamic(C_Type *);
    void markStatic(C_Type *);
};
void binding_time_analysis(C_Pgm const&,PAresult const&,
                           BTresult &);

//-------------------------------------------------------
//  STRUCTURE SEPARATION                      separate.cc
//
// This module duplicates C_UserDefs in the program so
// all instances of each C_UserDef have the exact same
// binding-time properties.

void Separate(C_Pgm &, BTresult const&);

//-------------------------------------------------------
//  DATA FLOW ANALYSES                        dataflow.cc
//
// This module currently computes nothing. In time it is
// going to perfom in-use and may/must-write analyses
// that are used to support memoisation decisions in
// gegen.

typedef array<C_Decl,unsigned> IdNumberMap ;
class DFresult {
public:
  multiArray<InvALocSet*> Wdef;
  multiArray<InvALocSet*> Wmay;
  multiArray<InvALocSet*> Use;
  DFresult();
  // Performs the dataflow analysis:
  void Analyse(C_Pgm const& ,PAresult const&, ALocSet const&);
  // return the set of alocs that are live at the entry to
  // the basic block
  Pset<C_Decl> const &GetMemoSet(C_BasicBlock*) const;
  // return plain aloc sets for gegen
  Pset<C_Decl> const &GetReadSet(C_Decl*) const ;
  Pset<C_Decl> const &GetWriteSet(C_Decl*) const ;

  InvALocSet* getWdefSet(Numbered const*);
  InvALocSet* getWmaySet(Numbered const*);
  InvALocSet* getUseSet(Numbered const*);
};

//-------------------------------------------------------
//  CODE-SHARING ANALYSIS                        share.cc
//
// This module computes a set of functions that should be
// inlined by the generating extention. This can have
// various reasons, e.g. they contain impure spectime
// actions or it is judged that their residuals would be
// too trivial to warrant a separate residual function.

struct SAresult : ALocSet {};
void findUnsharable(C_Pgm const&,PAresult const&,
                    CGresult const&, BTresult const&,
                    SAresult &);

//-------------------------------------------------------
//  SANITY                                     btdebug.cc
//
// This module checks that 'spectime' user annotations
// hold, and that external calls are generally
// well-formed.

void checkUserAnnoSanity(C_Pgm const&, BTresult const&);

//-------------------------------------------------------
//  STRUCTSORT                              structsort.cc
//
// This module reorders usertype definitions to make sure
// that no structure definition in P_gen comes before the
// definition of its member types.

void sortStructDecls(C_Pgm &);

//-------------------------------------------------------
//  GEGEN                                        gegen.cc
//
// This module emits the generating extension as raw C++
// code.

void emitGeneratingExtension(C_Pgm const&, BTresult const&, PAresult const&,
                             SAresult const&, DFresult const&,
                             ostream &);

#endif
