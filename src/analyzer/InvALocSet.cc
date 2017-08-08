/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: 
 * Docs:     ../doc/ *.tex
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "InvALocSet.h"
#include "commonout.h"
#include "outcore.h"
#include "strings.h"

void
convertALocSet(ALocSet const* a, InvALocSet& res) {
  foreach( mem, *a, ALocSet) res.insert(*mem);
}

// Output set of outputType to container, adding an anchor for it to
// anclist and preceding it with pretext
Anchor*
outInvALocSet(const char* pretext, InvALocSet* set,
              OType* outputType, Outcore *oc)
{
  Plist<Output>* infolist = new Plist<Output>();
  infolist->push_back(new Output(pretext, OAnnoText));
  infolist->push_back(INDENT);
  if ( set->full() ) {
    infolist->push_back(new Output("all", OAnnoText));    
  }
  else if ( set->empty() ) {
    infolist->push_back(new Output("none", OAnnoText));    
  }
  else {
    if ( set->isInversed() ) {
      infolist->push_back(o_anything_but);
      infolist->push_back(blank);
    }    
    infolist->push_back(outALocSet(&set->set,oc));
  }
  Anchor* anchor = new Anchor(outputType);
  oc->container.add(new Output(anchor,
                               new Output(Output::Consistent,infolist)),
                    outputType);
  return anchor ;
}

ostream&
operator<<(ostream& s, InvALocSet &a)
{
  if ( a.full() )
    return s << "(all)" ;
  if ( a.empty() )
    return s << "(none)" ;
  if ( a.isInversed() )
    s << str_tilde;
  bool first = true ;
  foreach( i, a.set, Pset<C_Decl> ) {
    if ( !first )
      s << ", ";
    s << **i ;
  }
  return s ;
}

