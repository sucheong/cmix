/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix gegen subsystem: struct/union handling
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "gegen.h"
#include "directives.h"

char const *
dotcmix(C_Type *t)
{
  if ( t->tag == StructUnion )
    return ".cmix" ;
  return "" ;
}


void
GegenEnv::pgen_usertype_fwds()
{
  foreach(i,pgm.usertypes,Plist<C_UserDef>) {
    oksection("Forward declarations for usertypes") ;
    if ( i->isUnion && bt.Static(i->instances.front()) )
      ost << "union " ;
    else
      ost << "struct " ;
    ost << names[*i] << ";\n" ;
  }
}

void
GegenEnv::pgen_usertype_decls()
{
  foreach(i,pgm.enumtypes,Plist<C_EnumDef>) {
    if ( bt.Dynamic(*i) )
      continue ;
    oksection("Definitions for usertypes") ;
    ost << "enum " << names[*i] << " {\n" ;
    foreach(j,i->members(),Plist<C_UserMemb>) {
      ost << "  " << names[*j] ;
      if ( j->hasValue() ) {
        ost << " = " << EmitExpr(j->value()) ;
      }
      ost << ",\n" ;
    }
    ost << "};\n" ;
  }
  foreach(ii,pgm.usertypes,Plist<C_UserDef>) {
    bool dyn = bt.Dynamic(ii->instances.front()) ;
    if ( !dyn && ii->names.empty() )
      continue ;
    oksection("Definitions for usertypes") ;
    ost << ( !dyn && ii->isUnion ? "union " : "struct " )
        << names[*ii] << " {\n" ;
    if ( dyn )
      ost << "  Code cmix;\n" ;
    foreach(j,**ii,C_UserDef) {
      ost << "  " << pushtype(j.type(),j.name()->qualifiers)
          << names[j.name()] << poptype ;
      if ( j.name()->hasValue() && !dyn ) {
        ost << " : " << EmitExpr(j.name()->value()) ;
      }
      ost << ";\n" ;
    }
    ost << "};\n" ;
  }
}

void
GegenEnv::define_cmixMember()
{
  bool begun = false ;
  foreach(i,pgm.usertypes,Plist<C_UserDef>)
    if ( bt.Dynamic(i->instances.front()) ) {
      if ( !begun ) {
        oksection("Names for dynamic struct members")
          << "static struct {\n" ;
        begun = true ;
      }
      ost << "  struct {\n" ;
      foreach(j,**i,C_UserDef) {
        ost << "    Code " << names[j.name()] ;
        for ( C_Type *t = j.type() ;
              t->tag == Array && bt.Static(t) ;
              t = t->array_next() ) {
          assert(t->hasSize());
          ost << "[" << EmitExpr(t->array_size()) << "]" ;
        }
        ost << ";\n" ;
      }
      ost << "  } " << names[*i] << ";\n" ;
    }
  if ( begun )
    ost << "} cmixMember;\n" ;
}

unsigned
GegenEnv::putnameseq(C_Type *t)
{
  if ( t->tag != StructUnion )
    return 0 ;
  return PutName[t->user_def()] ;
}

void
GegenEnv::define_cmixPutName()
{
  static unsigned putnamecounter = 0 ;
  foreach(i,pgm.usertypes,Plist<C_UserDef>)
    if ( bt.Dynamic(i->instances.front()) ) {
      oksection("Naming functions for dynamic usertypes");
      PutName[*i] = ++putnamecounter ;
      ost << "static void\ncmixPutName" << putnamecounter
          << "(struct " << names[*i] << " *cmixThis,Code cmixIt)\n{\n"
        "  cmixThis->cmix = cmixIt;\n" ;
      foreach(j,**i,C_UserDef) {
        ForCascade fc(j.type(),*this);
        ost << "cmixPutName" << putnameseq(fc.t2) << "(&cmixThis->"
            << names[j.name()] << fc
            << ",cmixMkExp(\"?.?\",cmixIt,cmixMember." << names[*i]
            << '.' << names[j.name()] << fc << "));\n" ;
        fc.close() ;
      }
      ost << "}\n" ;
    }
}

void
GegenEnv::init_struct()
{
  // declare enum types
  foreach(et,pgm.enumtypes,Plist<C_EnumDef>) {
    res_taboos.push_back(names[*et]);
    ost << "  cmixDeclare(cmixStruct,0," << enter
        << "enum " << names[*et] << " {" ;
    foreach(m,et->members(),Plist<C_UserMemb>) {
      res_taboos.push_back(names[*m]);
      ost << "\\n\"\n\t\"" << names[*m] ;
      if ( m->hasValue() )
        ost << " = " << EmitExpr(m->value());
      ost << ',' ;
    }
    ost << "\"\n\t\" }" << exit << ");\n";
  }
  // make field names; declare usertypes
  foreach(ud,pgm.usertypes,Plist<C_UserDef>)
    if ( !directives.is_wellknown(ud->get_name()) && 
         bt.Dynamic(ud->instances.front()) &&
         !ud->names.empty() ) {
      bool may_shrink = true ;
      foreach(j,**ud,C_UserDef) {
        ForCascade fc(j.type(),*this);
        ost << "cmixMember." << names[*ud] << '.'
            << names[j.name()] << fc << '='
            << resnames.request(j.name()->get_name()) << ";\n" ;
        fc.close() ;
      }
      ost << "  cmixDeclare(cmixStruct,0,\""
          << ( ud->isUnion ? "union " : "struct " );
      if ( residual_structs.find(*ud) )
        ost << ud->get_name(), may_shrink = false ;
      else
        ost << names[*ud] ;
      ost << "\");\n" ;
            
      // struct with bitfields may not lose any fields
      if ( may_shrink )
        foreach(m,ud->names,Plist<C_UserMemb>)
          if ( m->hasValue() )
            may_shrink = false ;
            
      foreach(m,**ud,C_UserDef) {
        ForCascade fc(m.type(),*this);
        fc.cv += m.name()->qualifiers ;
        ost << "cmixDeclare(cmixMemberDecl," ;
        if ( may_shrink )
          ost << "cmixMember." << names[*ud] << '.'
              << names[m.name()] << fc ;
        else
          ost << '0' ;
        ost << ',' << enter << "\\n  "
            << pushtype(fc.t2,fc.cv) << userhole << poptype ;
        if ( m.name()->hasValue() )
          ost << " #; " << EmitExpr(m.name()->value());
        ost << ';' << exit << ",cmixMember." << names[*ud] << '.'
            << names[m.name()] << fc << exit << ");\n";
        fc.close() ;
      }
    }
}

void
GegenEnv::exit_struct()
{
  bool first = true ;
  foreach(ud,pgm.usertypes,Plist<C_UserDef>)
    if ( !directives.is_wellknown(ud->get_name()) && 
         bt.Dynamic(ud->instances.front()) ) {
      if ( first ) ost << "  fprintf(fp," ;
      first = false ;
      ost << "\n\t\"" << ( ud->isUnion ? "union " : "struct " ) ;
      if ( residual_structs.find(*ud) )
        ost << ud->get_name() ;
      else
        ost << names[*ud] ;
      ost << ";\\n\"\n" ;
    }
  if ( !first )
    ost << ");\n" ;
}

