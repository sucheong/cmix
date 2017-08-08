/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Output producer for the PA annotations.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "options.h"
#include "commonout.h"
#include "outanno.h"
#include "directives.h"
#include "ALoc.h"

OutPa::OutPa(PAresult const&d, CoreAnnotator *nxt)
  : CoreAnnotator(*nxt), data(d), exp_cached(NULL), lval_cached(NULL)
{
}

static void
make_pa_line(C_Decl &d,ALocSet *data,Plist<Output> *os,
             Outcore *oc, Output *middle)
{
  Plist<Output> *os2 = new Plist<Output> ;
  os2->push_back(outputDeclLink(&d,oc));
  os2->push_back(blank);
  os2->push_back(middle);
  os2->push_back(INDENT);
  os2->push_back(outALocSet(data,oc));
  os->push_back(new Output(Output::Consistent,os2));
  os->push_back(newline);
}

void
OutPa::declrec(C_Decl &d,Plist<Output> *os, Outcore *oc)
{
  switch(d.type->tag) {
  default:
    break ;
  case Function: {
    static Output *middle = NULL ;
    if ( middle == NULL ) middle = new Output("returns one of",OAnnoText);
    make_pa_line(d,data.PTvar[d],os,oc,middle);
    return ; }
  case StructUnion: {
    foreach(i,d.members(),Plist<C_Decl>)
      declrec(**i,os,oc);
    break ; }
  case Array:
    declrec(*d.members().front(),os,oc);
    break ;
  case FunPtr:
  case Pointer: {
    static Output *middle = NULL ;
    if ( middle == NULL ) middle = new Output("may point to",OAnnoText);
    make_pa_line(d,data.PTvar[d],os,oc,middle);
    return ; }
  }
  assert(data.PTvar[d] == NULL || data.PTvar[d]->empty());
}

void
OutPa::operator()(C_Decl &d, Plist<Anchor> &alist, Outcore *oc)
{
  Plist<Output> *os = new Plist<Output> ;
  declrec(d,os,oc);
  if ( os->empty() )
    delete os;
  else {
    Output *o = new Output(Output::Consistent,os) ;
    Anchor *a = new Anchor(OPA) ;
    oc->container.add(new Output(a,o),OPA);
    alist.push_back(a);
  }
  next(d,alist,oc);       
}

void
OutPa::operator()(C_Expr &e, bool lval, Plist<Anchor> &alist, Outcore *oc)
{
  ALocSet *set = data.PT[e] ;
  assert(set);
  if ( e.tag==C_ExtFun ) {
    alist.push_back( outALocSet("May read",
                                data.ExtfunReading[e.var()],
                                OExternRead,oc ));
    if ( e.var()->effects() == NULL || e.var()->effects()->state != SEPure )
      alist.push_back( outALocSet("May write",
                                  data.ExtfunWriting[e.var()],
                                  OExternWrite,oc ));
  }
  if ( ( e.type->tag==Pointer || e.type->tag==FunPtr ) &&
       ( hardcore_mode ||
         e.tag != C_Var && e.tag != C_Cnst &&
         e.tag != C_FunAdr && e.tag != C_ExtFun ) ) {
    if ( lval ) {
      if ( lval_cached[set] == NULL )
        lval_cached[set]
          = outALocSet("The lval may refer to",set,OPA,oc);
      alist.push_back(lval_cached[set]);
    } else {
      if ( exp_cached[set] == NULL )
        exp_cached[set]
          = outALocSet("The expression may point to",set,OPA,oc);
      alist.push_back(exp_cached[set]);
    }
  }
  next(e,lval,alist,oc);
}


