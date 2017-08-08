/* Time-stamp: <98/04/15 15:33:21 panic>
 * Authors:  The C-mix Team
 * Content:  C-Mix system: ALoc (Abstract locations, aka objects)
 *                         utilities
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __ALOC_C__
#define __ALOC_C__

#include <cmixconf.h>
#include "ALoc.h"
#include <string.h>
#include <stdlib.h>
#include "outcore.h"
#include "commonout.h"
#include "renamer.h"

ALocSet::ALocSet(): Numbered(Numbered::ALOCSET) {}
//ALocSet::ALocSet(C_Decl& d): Numbered(Numbered::ALOCSET) { insert(&d); }
ALocSet::ALocSet(C_Decl* d): Numbered(Numbered::ALOCSET) { insert(d); }

ostream& operator<<(ostream& s, Pset<C_Decl> &a) {
  if ( a.empty() )
    return s << "(none)" ;
  bool first = true ;
  foreach(i,a,ALocSet) {
    if ( !first )
      s << ", ";
    s << **i ;
  }
  return s ;
}

ostream& operator<<(ostream& s, C_Decl &d) {
  switch(d.tag) {
  case ExtFun:
    return s << "extern " << d.get_name() ;
  case ExtState:
    return s << "{extern state}" ;
  case VarDcl:
    if ( d.isContained() ) {
      C_Decl* parent = d.containedIn() ;
      switch(parent->type->tag) {
      case Array:
        return s << parent << "[]" ;
      case StructUnion: {
        Plist<C_UserMemb>::iterator i
          = parent->type->user_def()->names.begin();
        Plist<C_Decl>::iterator j = parent->members().begin();
        while( *j != &d )
          i++, j++ ;
        return s << parent << '.' << i->get_name() ; }
      case Function:
        break ;
      default:
        return s << parent << "{strange decl owner}" ;
      }
    }
    // fall through
  case FunDf:
    return s << d.get_name() ;
  }
  return s << "thiscanthappen!" ;
}

// Create an Output object of a variable with a link to its declaration
Output*
outputDeclLink(C_Decl* d,Outcore *oc) {
  if (d->tag==ExtState)
    return new Output("{extern state}",OAnnoText);
  Plist<Output>* os = new Plist<Output>();
  while ( d->isContained() ) {
    C_Decl* parent = d->containedIn() ;
    switch(parent->type->tag) {
    case Array:
      os->push_front(rbracket);
      os->push_front(lbracket);
      break ;
    case StructUnion: {
      Plist<C_UserMemb>::iterator i = parent->type->user_def()->names.begin();
      Plist<C_Decl>::iterator j = parent->members().begin();
      while( *j != d )
        i++, j++ ;
      os->push_front(new Output((*oc->the_names)[*i],OVarname));
      os->push_front(dotsign);
      break ; }
    case Function:
      goto break2 ;
    default:
      Diagnostic(INTERNAL,parent->pos) << "strange type for decl owner";
    }
    d = parent ;
  }
 break2:
  os->push_front(oc->crosslink(new Output((*oc->the_names)[d], OVarname),d));
  if ( d->tag==ExtFun ) {
    os->push_front(blank);
    os->push_front(o_extern);
  }
  return new Output(Output::Consistent,os);
}

// Create an Output object of a set of objects (ALoc's)
Output*
outALocSet(Pset<C_Decl>* s,Outcore *oc)
{
  if (s == NULL)
    return nullptr ;
  else if ( s->empty() )
    return new Output("(none)", OAnnoText);
  else {
    OutputList setlist(0,Output::Inconsistent) ;
    setlist.sep.push_back(comma);
    setlist.sep.push_back(BREAK);
    foreach(i,*s,ALocSet) {
      if ((*i) == NULL)
        setlist += nullptr ;
      else
        setlist += outputDeclLink(*i,oc);
    }
    return setlist.inter() ;
  }
}

// Output set of outputType to container, adding an anchor for it to
// anclist and preceding it with pretext
Anchor*
outALocSet(const char* pretext, ALocSet* set,
           OType* outputType, Outcore *oc)
{
  Plist<Output>* infolist = new Plist<Output>();
  infolist->push_back(new Output(pretext, OAnnoText));
  infolist->push_back(INDENT);
  infolist->push_back(outALocSet(set,oc));
  Anchor* anchor = new Anchor(outputType);
  oc->container.add(new Output(anchor,
                               new Output(Output::Consistent,infolist)),
                    outputType);
  return anchor ;
}

#endif
