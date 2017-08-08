/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix gegen subsystem: Declaring objects and support data
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "directives.h"
#include "gegen.h"

char const GegenEnv::cmixGlobal[] = "cmixGlobal" ;
char const GegenEnv::cmixGinit[] = "cmixGinit" ;

static bool
MustSplitInit(C_Init *init,BTresult const &bt)
{
  if ( init == NULL )
    return false ;
  C_Type *t = init->owner()->type ;
  return t->tag == Array &&
    bt.Static(t) &&
    bt.Dynamic(t->array_contents_type());
}

bool
GegenEnv::NameDynamicThing(ForCascade &fc,
                           char const *resname, char const *genname,
                           bool managed,
                           const char *registry, char const *linkage,
                           C_Init *init)
{
  if ( bt.Static(fc.t2) )
    return false ;
  bool array_init = MustSplitInit(init,bt) ;
        
  ost << "cmixPutName" << putnameseq(fc.t2)
      << "(&" << genname << fc << ',' ;
  if ( managed )
    ost << resnames.request(resname);
  else
    ost << "cmixMkExp(\"" << resname << "\")" ;
  ost << ')' ;
  if ( registry != NULL ) {
    if ( registry == cmixGlobal && init )
      registry = cmixGinit ;
    do {
      fc.addline() << "cmixDeclare(" << registry << ',' ;
      if ( managed )
        ost << genname << fc << dotcmix(fc.t2) ;
      else
        ost << '0' ;
      ost << ',' << enter << linkage
          << pushtype(fc.t2,fc.cv) << userhole << poptype ;
      if ( array_init )
        ost << " = ?" ;
      else if ( init )
        ost << " = " << init ;
      ost << exit << ',' << genname << fc << dotcmix(fc.t2) << exit ;
      if ( array_init )
        ost << ",cmixRI" << fc ;
      ost << ')' ;
            
      // globals with initializers get an extra round
      if ( registry != cmixGinit )
        break ;
      registry = cmixGlobal ;
      init = NULL ;
      array_init = false ;
    } while(1) ;
  }
  ost << ";\n" ;
  return true ;
}

static void
emit_cmixRI_initializers(GegenStream &ost,C_Init *i,unsigned *arr,
                         unsigned idx,unsigned dimensions)
{
  if ( idx < dimensions ) {
    assert(i->tag == FullyBraced);
    arr[idx] = 0 ;
    foreach(sub,i->braced_init(),Plist<C_Init>) {
      emit_cmixRI_initializers(ost,*sub,arr,idx+1,dimensions);
      arr[idx]++ ;
    }
  } else {
    ost << "    cmixRI" ;
    for(unsigned j=0;j<idx;j++) {
      ost << '[' << arr[j] << ']' ;
    }
    ost << " = " << i << ";\n" ;
  }
}

void
GegenEnv::NameDynamicObject(C_Decl *d, const char *registry)
{
  if ( spectime(d) )
    return ;
  bool manage_the_name = true ;
  char const *declprefix = "" ;
  if ( !d->isContained() )
    switch(d->varmode()) {
    case VarExtResidual:
    case VarExtDefault:
      if ( directives.is_wellknown(d->get_name()) )
        registry = NULL ;
      declprefix = "extern ";
      manage_the_name = false ;
      break ;
    case VarVisResidual:
      manage_the_name = false ;
      break ;
    case VarIntAuto:
    case VarIntResidual:
      declprefix = "static " ;
    default:
      break ;
    }
  if ( !manage_the_name )
    res_taboos.push_back(d->get_name());

  C_Init *init = NULL ;
  bool split_array = false ;
  if ( d->hasInit() ) {
    init = d->init() ;
    if ( MustSplitInit(init,bt) ) {
      // initialization of a split array
      unsigned splitdimen = 0 ;
      split_array = true ;
      ost << "  { Code cmixRI" ;
      for ( C_Type* tt=d->type; bt.Static(tt); tt=tt->array_next() ) {
        ost << '[' << EmitExpr(tt->array_size(),0) << ']' ;
        splitdimen++ ;
      }
      ost << " = {0};\n" ;

      unsigned *indices = new unsigned[splitdimen] ;
      emit_cmixRI_initializers(ost,init,indices,0,splitdimen);
      delete[] indices ;
    }
  }
  ForCascade fc(d->type,*this, split_array ? 4 : 2);
  fc.cv += d->qualifiers ;
  NameDynamicThing(fc,d->get_name(),names[d],manage_the_name,
                   registry,declprefix,init);
  fc.close() ;
  if ( split_array )
    ost << "  }\n" ;
}

void
GegenEnv::define_pgen_globals()
{
  oksection("Gobal variables") ;

  foreach(i,pgm.globals,Plist<C_Decl>) {
    switch(i->varmode()) {
    case VarVisSpectime:
      // ost << "extern \"C\" { " ;
      break ;
    case VarExtSpectime:
      if ( directives.is_wellknown((*i)->get_name()) )
        continue ;
      // ost << "extern \"C\"" ;
      break ;
    default:
      ost << "static " ;
      break ;
    }
    ost << *i ;
    if ( spectime(*i) && i->hasInit() )
      ost << " = " << i->init() ;
    ost << ';' ;
    // if (i->varmode() == VarVisSpectime )
    //   ost << " }" ;
    ost << '\n' ;
  }
}
