/* (-*-c++-*-)
 * Authors:  The C-mix Team
 * Content:  C-Mix system: ALoc (Abstract locations, aka objects)
 *                         utilities
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __ALOC_H__
#define __ALOC_H__

#include "auxilary.h"
#include "output.h"
#include "Pset.h"
#include <iostream.h>

class C_Decl;
struct ALocSet: public Pset<C_Decl>, public Numbered {
  ALocSet() ;
  ALocSet(C_Decl* d);
};

ostream& operator<<(ostream& s, Pset<C_Decl> &a);
ostream& operator<<(ostream& s, C_Decl &d);

class Outcore ;

// Create the pretty-printed name of an aloc
Output *outputDeclLink(C_Decl *,Outcore *) ;
                      
// Create an Output object of a set of objects (ALoc's)
Output*
outALocSet(Pset<C_Decl>*,Outcore *);

// Output set of outputType to container, adding an anchor for it to
// anclist and preceding it with pretext
Anchor*
outALocSet(const char* pretext, ALocSet* set,
	   OType* outputType, Outcore *);

#endif
