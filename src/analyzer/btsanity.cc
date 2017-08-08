/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Binding-time debugger
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "analyses.h"
#include "directives.h"

static void badvarmode(C_Decl *decl,char const *whatsit) {
  Diagnostic d(INTERNAL,decl->pos);
  d << decl->get_name() << " has bad VarMode for a " << whatsit
    << decl->varmode();
  d.addline(decl->varmodeWhy()) << "(alleged source of varmode)" ;
}

static void
checkinternalvar(C_Decl *decl,char const *whatsit,BTresult const&bt)
{
  switch(decl->varmode()) {
  case VarIntAuto: // Everything is well with these
  case VarIntResidual: // Handled by the BTA
    break ;
  case VarIntSpectime:
    if ( bt.Dynamic(decl->type) ) {
      Diagnostic d(ERROR,decl->varmodeWhy()) ;
      d << decl->get_name() << " could not be kept as spectime." ;
      bt.Trace(d,decl->type);
    }
    break ;
  default:
    badvarmode(decl,whatsit);
  }
}

void checkUserAnnoSanity(C_Pgm const&corepgm,BTresult const&bt)
{
  foreach(v,corepgm.globals,Plist<C_Decl>)
    switch((*v)->varmode()) {
    case VarVisResidual: // Handled by the BTA
    case VarExtResidual: // ditto
    case VarExtDefault:  // ditto
      break ;
    case VarVisSpectime: // should be completely static
    case VarExtSpectime: // ditto
      if ( bt.Dynamic(v->type) ) {
        Diagnostic d(ERROR,(*v)->varmodeWhy());
        d << (*v)->get_name() << " could not be kept as spectime." ;
        bt.Trace(d,v->type);
      }
      break ;
    default:
      checkinternalvar(*v,"global variable",bt);
      break ;
    }

  foreach(f,corepgm.exfuns,Plist<C_Decl>)
    if ( f->calltime() && f->calltime()->time == CTSpectime &&
         bt.Dynamic(f->type) ) {
      Diagnostic d(ERROR,f->pos);
      d << f->get_name() << " is annotated to be called at spectime" ;
      d.addline() << "but that is not possible" ;
      d.addline(f->calltime()->pos) << "(source of annotation)" ;
      bt.Trace(d,f->type) ;
    }
  foreach(ff,corepgm.functions,Plist<C_Decl>) {
    foreach(d,ff->fun_params(),Plist<C_Decl>)
      checkinternalvar(*d,"function parameter",bt);
    foreach(dd,ff->fun_locals(),Plist<C_Decl>)
      checkinternalvar(*dd,"local variable",bt);
    if ( bt.Dynamic(ff->type) ) {
      Diagnostic d(ERROR,ff->pos);
      d << "Sorry, we do not yet support dynamic" ;
      d.addline() << "pointers to functions whose body is known" ;
      d.addline() << "but one to " << ff->get_name() << " needs to exist" ;
      bt.Trace(d,ff->type) ;
    }
  }
  foreach(fff,corepgm.generators,Plist<C_Decl>) {
    foreach(v,fff->fun_params(),Plist<C_Decl>)
      switch(v->varmode()) {
      case VarVisResidual: // Handled by the BTA
        break ;
      case VarVisSpectime:
        if ( bt.Dynamic(v->type) ) {
          Diagnostic d(ERROR,v->varmodeWhy());
          d << "a parameter specified as spectime needs "
            << "to be residual." ;
          bt.Trace(d,v->type);
        }
      default:
        break ;
      }
    foreach(vv,fff->fun_locals(),Plist<C_Decl>) {
      assert(vv->varmode()==VarVisResidual);
      // handled by the BTA
    }
  }
  
  if ( corepgm.generators.empty() )
    Diagnostic(ERROR,Position())
      << "no goal: or generator: directives found" ;
}
