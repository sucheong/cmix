/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix gegen subsystem: emit loops over statically indexed arrays
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "gegen.h"

ForCascade::ForCascade(C_Type *t,GegenEnv &env,unsigned i)
  : t1(t), ost(env.ost)
{
  Plist<C_Expr> limits ;
  while ( t->tag == Array && env.bt.Static(t) ) {
    limits.push_back(t->array_size());
    cv += t->qualifiers() ;
    t = t->array_next() ;
  }
  t2 = t ;
  depth = 0 ;
  indent = i ;
  nbraces = 0 ;

  if ( limits.empty() ) {
    if ( indent > 2 ) {
      // if there is no loop but the original indent is greater than 2
      // create a level of braces nevertheless
      ost << GegenEnv::multichar(' ',indent) << "{\n" ;
      indent+=2,nbraces++ ;
    }
  } else {
    // there are loops
    foreach(lim,limits,Plist<C_Expr>) {
      ost << GegenEnv::multichar(' ',indent);
      if ( depth == 0 )
        ost << "{ " , indent+=2,nbraces++ ;
      ost << "unsigned cmixI" << depth << ";\n"
          << GegenEnv::multichar(' ',indent)
          << "for(cmixI" << depth << "=0; "
          << "cmixI" << depth << '<' << EmitExpr(*lim,20)
          << "; cmixI" << depth << "++ ) {\n" ;
      indent+=2,nbraces++ ;
      depth++ ;
    }
  }

  assert( depth == limits.size() );
  ost << GegenEnv::multichar(' ',indent);
}

GegenStream &
operator<<(GegenStream &gs,ForCascade const &fc) {
  assert(&gs==&fc.ost);
  for ( unsigned indices = 0 ; indices < fc.depth ; indices++ )
    gs << "[cmixI" << indices << ']' ;
  return gs ;
}

GegenStream &
ForCascade::addline()
{
  ost << ";\n" << GegenEnv::multichar(' ',indent);
  return ost ;
}

void
ForCascade::close()
{
  if ( nbraces > 0 ) {
    ost << GegenEnv::multichar(' ',indent-2*nbraces -1) ;
    do
      ost << " }" ;
    while(--nbraces);
    ost << '\n' ;
  }
}

ForCascade::~ForCascade()
{
  assert(nbraces==0);
}
