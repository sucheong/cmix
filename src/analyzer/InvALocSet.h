/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: 
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __InvALocSet__
#define __InvALocSet__

#include "InvPset.h"
#include "ALoc.h"
#include "output.h"
#include "corec.h"
#include <iostream.h>

struct InvALocSet : public InvPset<C_Decl>, public Numbered
{
  // The default constructor makes an empty set.
  InvALocSet() : InvPset<C_Decl>(), Numbered(Numbered::INVALOCSET) {}
  friend Anchor* outInvALocSet(const char*, InvALocSet*,
                               OType*, Outcore *);
  friend ostream& operator<<(ostream&, InvALocSet&);

};

void convertALocSet(ALocSet const*, InvALocSet&);

// Output set of outputType to container, adding an anchor for it to
// anclist and preceding it with pretext
Anchor* outInvALocSet(const char* pretext, InvALocSet* set,
                      OType* outputType, Outcore *);

ostream& operator<<(ostream&, InvALocSet&);

#endif
