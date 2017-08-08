/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Displaying binding-time reasons
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "outcore.h"
#include "outanno.h"
#include "bta.h"
#include "commonout.h"
#include "diagnostic.h"

OutBt::OutBt(BTresult const&d, CoreAnnotator *nxt)
  : CoreAnnotator(*nxt), data(d), IsSpectime(NULL),
    anc_cache(NULL), name_cache(NULL)
{
}

void
OutBt::operator()(C_Type &t,		Plist<Anchor>&as,Outcore*oc)
{
  decorate(data.map[t],as,oc);
  next(t,as,oc);
}

void
OutBt::operator()(C_Decl &d,		Plist<Anchor>&as,Outcore*oc)
{
  decorate(data.map[d.type],as,oc);
  next(d,as,oc);
}

void
OutBt::operator()(C_UserMemb &m, C_Type &t,Plist<Anchor>&as,Outcore*oc)
{
  decorate(data.map[t],as,oc);
  next(m,t,as,oc);
}

void
OutBt::operator()(C_Stmt &s,		Plist<Anchor>&as,Outcore*oc)
{
  switch(s.tag) {
  case C_Assign:
    break ;
  case C_Call:
    break ;
  case C_Alloc:
    decorate(data.map[s.target()->type->ptr_next()],as,oc);
    break ;
  case C_Free:
    decorate(data.map[s.free_expr()->type],as,oc);
    break ;
  case C_Sequence:
    break ;
  }
  next(s,as,oc);
}

void
OutBt::operator()(C_Jump &j,		Plist<Anchor>&as,Outcore*oc)
{
  switch(j.tag) {
  case C_Goto:
    break ;
  case C_Return:
    break ;
  case C_If:
    decorate(data.map[j.cond_expr()->type],as,oc);
  }
  next(j,as,oc);
}

void
OutBt::operator()(C_BasicBlock &b,	Plist<Anchor>&as,Outcore*oc)
{
  decorate(data.map[b],as,oc);
  next(b,as,oc);
}

void
OutBt::operator()(C_Expr &e,bool lval,	Plist<Anchor>&as,Outcore*oc)
{
  if ( lval )
    decorate(data.map[e.type->ptr_next()],as,oc);
  else
    decorate(data.map[e.type],as,oc);
  next(e,lval,as,oc);
}

/////////////////////////////////////////////////////////////////////////////

static BTvariable *
collapse_cause_chain(BTcause *cause,BTvariable *var) {
 again:
  for(BTpairlist::iterator i = var->get_causes() ; i ; i++ ) {
    if ( i.cause() == cause ) {
      var = i.variable() ;
      goto again ;
    }
  }
  return var ;
}

void
OutBt::explain(BTvariable *bt,Plist<Output> &causestack,
               Pset<BTvariable> &already_met, Plist<Output> *dest,
               Outcore *oc)
{
  for(BTpairlist::iterator i = bt->get_causes() ; i ; i++ ) {
    BTcause *this_cause = i.cause() ;
    BTvariable *this_var = collapse_cause_chain(this_cause,i.variable()) ;

    // do not display the same dependency twice
    if ( already_met.insert(this_var) )
      continue ;
        
    causestack.push_front(findname(this_cause,oc));
    if ( !this_var->hasName() && this_var->get_causes() )
      explain(this_var,causestack,already_met,dest,oc);
    else {
      static Output *o_and = new Output("and",OAnnoText);
      static Output *because = new Output("because",OAnnoText);
      static Output *why = new Output("[why?]",OAnnoText);
      dest->push_back(newline);
      dest->push_back(o_and);
      dest->push_back(blank);
      dest->push_back(because);
      dest->push_back(blank);
      OutputList causelist(0,Output::Inconsistent) ;
      causelist.sep.push_back(semi);
      causelist.sep.push_back(BREAK);
      foreach(j,causestack,Plist<Output>)
        causelist += *j ;
      if ( this_var->hasName() ) {
        dest->push_back(findname(this_var,oc));
        dest->push_back(blank);
        dest->push_back(new Output(why,
                                   new Plist<Anchor>(varanc(this_var))));
        dest->push_back(INDENT);
        dest->push_back(lparen);
        dest->push_back(causelist.inter());
        dest->push_back(rparen);
      } else
        dest->push_back(causelist.inter());
    }
    causestack.pop_front() ;
  }
}

void
OutBt::produce(BTvariable *bt,Outcore *oc)
{
  Plist<Output> *os = new Plist<Output>;
  Plist<Output> causestack;
  Pset<BTvariable> metbtvs;
  explain(bt,causestack,metbtvs,os,oc);
  if ( os->empty() )
    os->push_back(new Output("(no further explanation)",OAnnoText));
  else {
    os->pop_front(); // newline
    os->pop_front(); // "and"
    os->pop_front(); // space
    os->push_front(newline);
  }
  os->push_front(findname(bt,oc));
  Output *o = new Output(Output::Consistent,os);
  oc->container.add(new Output(anc_cache[bt],o),ODynamic);
}

Anchor *
OutBt::varanc(BTvariable *bt)
{
  if ( anc_cache[bt] == NULL ) {
    anc_cache[bt] = new Anchor(ODynamic);
    pending.push_back(bt);
  }
  return anc_cache[bt] ;
}

Output *
OutBt::findname(BTobject *o, Outcore *oc)
{
  if ( name_cache[o] == NULL )
    name_cache[o] = o->show(oc) ;
  return name_cache[o] ;
}

void
OutBt::decorate(BTvariable *bt,Plist<Anchor>&as,Outcore*oc)
{
  assert(bt!=NULL);
  if (bt == NULL)
    return ;
  if (!bt->isDynamic()) {
    if ( IsSpectime == NULL ) {
      IsSpectime = new Anchor(OStatic);
      oc->container.add(new Output(IsSpectime,o_Null),OStatic);
    }
    as.push_back(IsSpectime);
    return ;
  }
  // if we reach down here, we know that the binding-time variable
  // exists and is dynamic.

  as.push_back(varanc(bt));

  while ( !pending.empty() ) {
    BTvariable *bt2 = pending.back() ;
    pending.pop_back() ;
    produce(bt2,oc);
  }
}

/////////////////////////////////////////////////////////////////////////////

void
BTresult::Trace(Diagnostic &d,Numbered *n) const
{
  BTvariable *btv = map[n];
  assert(btv->isDynamic());
  d.addline() << "---------- BINDING-TIME PROBLEM TRACE BEGINS: ----------";
  while(1) {
    btv->DoShow(d);
    if ( !btv->get_causes() )
      break ;
    BTcause *btc = btv->get_causes().cause() ;
    btc->DoShow(d);
    btv = btv->get_causes().variable() ;
    btv = collapse_cause_chain(btc,btv) ;
  }
  d.addline() << "---------- BINDING-TIME PROBLEM TRACE ENDS ----------";
}
