/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Miscellaneous annotation generators
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "outanno.h"
#include "commonout.h"
#include "ALoc.h"

//////////////////////////////////////////////////////////////////////

OutCg::OutCg(CGresult const&d, CoreAnnotator *nxt)
  : CoreAnnotator(*nxt), data(d), called_cached(NULL)
{
}

void
OutCg::operator()(C_Decl &d, Plist<Anchor> &alist ,Outcore *oc)
{
  if ( d.tag == FunDf ) {
    ALocSet *set = data.preds[d] ;
    assert(set!=NULL);
    if ( called_cached[set] == NULL )
      called_cached[set]
        = outALocSet("The function is called from",set,OCG,oc);
    alist.push_back(called_cached[set]);
  }
  next(d,alist,oc);
}

//////////////////////////////////////////////////////////////////////

OutTrulyLocal::OutTrulyLocal(PAresult const&d, CoreAnnotator *nxt)
  : CoreAnnotator(*nxt), data(d), NotLocal(NULL)
{
}

void
OutTrulyLocal::operator()(C_Decl &d, Plist<Anchor> &alist, Outcore *oc)
{
  if ( d.isContained() &&
       d.containedIn()->tag == FunDf &&
       !data.TrulyLocals[d.containedIn()]->find(&d) ) {
    if ( NotLocal == NULL ) {
      NotLocal = new Anchor(OAliased);
      oc->container.add(new Output(NotLocal,o_Null),OAliased);
    }
    alist.push_back(NotLocal);
  }
  next(d,alist,oc);
}

//////////////////////////////////////////////////////////////////////

OutSa::OutSa(SAresult const&d, CoreAnnotator *nxt)
  : CoreAnnotator(*nxt), data(d), Shareable(NULL), Unshareable(NULL)
{
}

void
OutSa::operator()(C_Decl &d, Plist<Anchor> &alist, Outcore *oc)
{
  if ( d.tag == FunDf ) {
    if ( data.find(&d) ) {
      if ( Unshareable == NULL ) {
        Unshareable = new Anchor(OUnshareable);
        oc->container.add(new Output(Unshareable,o_Null),OUnshareable);
      }
      alist.push_back(Unshareable);
    } else {
      if ( Shareable == NULL ) {
        Shareable = new Anchor(OShareable);
        oc->container.add(new Output(Shareable,o_Null),OShareable);
      }
      alist.push_back(Shareable);
    }
  }
  next(d,alist,oc);
}

