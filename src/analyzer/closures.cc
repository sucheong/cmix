/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Compute truly local variables.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "analyses.h"
#include "options.h"
#include "ALoc.h"

#define debug (debugstream_locals)

    // takeClosure computes the points-to closure of the alocs mentioned
    // in `source' and puts it into `target' which is assumed to be
    // empty initially.
    // Any alocs that are mentioned in `exclude' are ignored during
    // the closure calculations.
    // The `include' parameter states whether an ALoc must be put into
    // target simply by being mentioned in source without being pointed
    // to by anything.
static void
takeClosure(PAresult const &pa,C_Decl *current,bool include,
            ALocSet &target, ALocSet const &exclude)
{
    if ( exclude.find(current) || target.find(current) )
        return ;
    if ( include )
        target.insert(current);
    switch(current->tag) {
    case FunDf:
    case ExtFun:
        break ;
    case VarDcl:
    case ExtState:
        foreach(i,current->members(),Plist<C_Decl>)
            takeClosure(pa,*i,include,target,exclude);
        foreach(ii,*pa.PTvar[current],ALocSet)
            takeClosure(pa,*ii,true,target,exclude);
        break ;
    }
}

static void
takeClosure(PAresult const &pa,Plist<C_Decl> const &source,bool include,
            ALocSet &target,ALocSet const &exclude)
{
    foreach(current,source,Plist<C_Decl>)
        takeClosure(pa,*current,include,target,exclude);
}

static void
makeLocals(Plist<C_Decl> &list,ALocSet const &excl1,ALocSet const &excl2,
           ALocSet &target)
    // makeLocals adds to `target' all alocs in `list' and all their
    // subsidiaries, excluding along the way any alocs mentioned in
    // one of the `exclN' sets. As an optimisation we assume that
    // the union of the `exclN' sets is subsidiary-closed.
{
    foreach(i,list,Plist<C_Decl>) {
        if ( excl1.find(*i) || excl2.find(*i) )
            continue ;
        target.insert(*i);
        makeLocals(i->members(),excl1,excl2,target);
    }
}

static bool
containedInArray(C_Decl* d)
{
  assert(d);
  while ( d->isContained() ) {
    d = d->containedIn();
    if (d->type->tag == Array)
      return true;
  }
  return false;
}

static void
collectUniqueObjects(C_Decl* d, ALocSet& oneObjectOnlyDecls)
{
  oneObjectOnlyDecls.insert(d);
  if( d->type->tag == StructUnion )
    foreach(m,d->members(),Plist<C_Decl>)
      collectUniqueObjects(*m,oneObjectOnlyDecls);
}

void
TrulyLocal(C_Pgm const &pgm,CGresult const &cg,PAresult &pa,
           ALocSet& oneObjectOnlyDecls)
{
  // A handy set to have.
  ALocSet emptyset ;
  // create (once and for all) the set of ALocs that can be reached
  // from global variables.
  ALocSet global_closure ;
  takeClosure(pa,pgm.globals,true,global_closure,emptyset);
  foreach(ef,pgm.exfuns,Plist<C_Decl>)
    foreach(i,*pa.ExtfunReading[*ef],Pset<C_Decl>)
      takeClosure(pa,*i,false,global_closure,emptyset);

  // create sets of truly local alocs for each function
  foreach(fun,pgm.functions,Plist<C_Decl>) {
    ALocSet *newset = new ALocSet ;
    if ( cg.preds[*fun]->find(*fun) ) {
      // the function is recursive, so there is danger of aliasing.
      // exclude any aloc that is reachable from global variables
      // or from one of the parameters.
      ALocSet param_closure ;
      takeClosure(pa,fun->fun_params(),false,
                  param_closure,global_closure);
      makeLocals(fun->fun_params(),global_closure,param_closure,*newset);
      makeLocals(fun->fun_locals(),global_closure,param_closure,*newset);
    } else {
      // the function is not recursive; nothing can be aliased
      makeLocals(fun->fun_params(),emptyset,emptyset,*newset);
      makeLocals(fun->fun_locals(),emptyset,emptyset,*newset);
    }
    // All truly local declarations posibly refer to a single object.
    foreach( d, *newset, ALocSet )
      if ( !containedInArray(*d) )
        oneObjectOnlyDecls.insert(*d);
    assert(pa.TrulyLocals[*fun]==NULL);
    pa.TrulyLocals[*fun] = newset ;
  }

  foreach(fun2,pgm.generators,Plist<C_Decl>) {
    ALocSet *newset = new ALocSet ;
    makeLocals(fun2->fun_locals(),emptyset,emptyset,*newset);
    foreach( d, *newset, ALocSet )
      if ( !containedInArray(*d) )
        oneObjectOnlyDecls.insert(*d);
    assert(pa.TrulyLocals[*fun2]==NULL);
    pa.TrulyLocals[*fun2] = newset ;
  }

  // All global declarations posibly refer to a single object.
  foreach( d, pgm.globals, Plist<C_Decl>)
    collectUniqueObjects(*d, oneObjectOnlyDecls);
  
  if (debug) {
    debug << "[oneObjectOnlyDecls = {";
    foreach( d, oneObjectOnlyDecls, ALocSet ) debug << d->get_name() << ' ';
    debug << "}]" << endl;
  }
}
