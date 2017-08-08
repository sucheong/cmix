/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Translating generator directives to Core C
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "directives.h"
#include "corec.h"
#include "cpgm.h"
#include "strings.h"
#include <string.h>
#ifdef HAVE_REGEX_H
#include <stddef.h> // for redhat machines which otherwise choke on regex.h
#include <regex.h>  // for OSF alphas with buggy GCC installations
#endif
#include <ctype.h>

// This module translates each generator directive into a Core C
// pseudo-function that can be binding-time analysed directly.
// This replaces earlier adhockery in gegen.
//
// The pseudo-functions follow a common scheme:
//
// NAME
//   - "main" means a 'goal:' entry point; anything else is
//     the name of a 'generator:' entry point.
// PARAMETERS
//   - first, a block corresponding directly to entry point
//     parameters. These all have VarVisSpectime varmode.
//   - second, a block corresponding directly to residual
//     function parameters. These all have VarVisResidual
//     varmode.
// RETURN TYPE
//   - the return type of the residual function
// LOCAL VARIABLES
//   - a single local variable may be present, for transfering
//     the residual return value. If present, its varmode is
//     VarVisResidual.
// BASIC BLOCKS
//   - always exactly one, containing two statements and a Return.
// FIRST STATEMENT
//   - a call (return value ignored) to an external function (call
//     expression ignored) with a printf-like parameter list. This
//     is performed at spectime to produce the name of the residual
//     function. The callmode is CallOnceSpectime.
// SECOND STATEMENT
//   - a call of the specializable function. The target can be
//     empty or point to the local variable.
// RETURN STATEMENT
//   - either without expression or returning the value of the local
//     variable

static void
parse_printf_specification(char const *pc, Plist<C_Type> &rest,
                           Position pos)
{
  for( ; pc[0] ; pc++)
    if ( pc[0] == '%' ) {
      if ( pc[1] == '%' )
        pc++ ; // skip %% conversion
      else {
        bool isLong = false ;
        bool last = false ;
        do switch( *++pc ) {
        case '-': case '+': case ' ': case '#': // flags
        case '.': // width/precision specifier
          break ;
        case '*':
          rest.push_back(new C_Type(Int));
          break ;
        case 'l': case 'L':
          isLong = true ;
          break ;
        default:
          if ( !isdigit(pc[0]) )
            last = true ;
          break ;
        } while ( !last );
        BaseTypeTag thetypename = Void ;
        switch( pc[0] ) {
        case 'd': case 'i': case 'c':
          thetypename = isLong ? LongInt : Int ;
          break ;
        case 'o': case 'u': case 'x': case 'X':
          thetypename = isLong ? ULongInt : UInt ;
          break ;
        case 'f': case 'e': case 'E': case 'g': case 'G':
          thetypename = isLong ? LongDouble : Double ;
          break ;
        case 's':
          break ;
        default:
          Diagnostic(WARNING,pos)
            << "cannot parse conversion specifier \"%"
            << pc[0] ;
        }
        if ( thetypename != Void )
          rest.push_back(new C_Type(thetypename));
        else {
          C_Type *achar = new C_Type(Char);
          rest.push_back(new C_Type(Pointer,achar));
        }
      }
    }
}

// This function implements very ugly heuristics for choosing
// static parameter type for the entry point.

static C_Type *
negotiate_type(Plist<C_Type> const &thelist,Position pos,int dollar)
{
  if ( thelist.empty() ) {
    Diagnostic(WARNING,pos) << '$' << dollar
                            << " is never used; defaulting to int" ;
    return new C_Type(Int) ;
  }
  bool all_equal_front = true ;
  bool all_primitive = true ;
  bool all_equal_front_ptr = true ;
  bool any_float = false ;
  bool any_longdouble = false ;
  bool any_long = false ;
  bool any_unsigned = false ;
  C_Type *front_ptr = thelist.front()->tag == Pointer ?
    thelist.front()->ptr_next() : NULL ;
  foreach(t,thelist,Plist<C_Type>) {
    if ( !t->equal(*thelist.front()) )
      all_equal_front = false ;
    if ( t->tag != Primitive )
      all_primitive = false ;
    else switch(t->basetype()) {
    case UAbsT: case UShortInt: case UInt:
      any_unsigned = true ;
      break ;
    case AbsT: case ShortInt: case Int:
    case Void: case Char: case SChar: case UChar:
      break ;
    case LongDouble:
      any_longdouble = true ;
      // fall through
    case Float: case Double: case FloatingAbsT:
      any_float = true ;
      break ;
    case ULongInt:
      any_unsigned = true ;
      // fall through
    case LongInt:
      any_long = true ;
      break ;
    }
    if ( !front_ptr ||
         t->tag != Pointer ||
         !t->ptr_next()->equal(*front_ptr) )
      all_equal_front_ptr = false ;
  }
  if ( all_equal_front ) {
    return thelist.front()->copy() ;
  } else if ( all_primitive ) {
    if ( any_longdouble )
      return new C_Type(LongDouble);
    else if ( any_float )
      return new C_Type(Double);
    else if ( any_unsigned ) {
      if ( any_long )
        return new C_Type(ULongInt);
      else
        return new C_Type(UInt);
    } else {
      if ( any_long )
        return new C_Type(LongInt);
      else
        return new C_Type(Int);
    }
  } else if ( all_equal_front_ptr ) {
    C_Type *res = thelist.front()->copy() ;
    constvol cv ;
    foreach(t,thelist,Plist<C_Type>) {
      cv += t->qualifiers() ;
    }
    res->qualifiers(cv);
    return res ;
  } else {
    Diagnostic(WARNING,pos) << "unrecoverable type problem for $"
                            << dollar ;
    return thelist.front()->copy();
  }
}

static void
convert_params(Plist<GeneratorDirective::Param> &plist,
               Plist<C_Type> const &fit_types,
               Plist<C_Expr> *result,
               Plist<C_Decl> *pseudo_params, Position pos)
{
  Plist<C_Type>::iterator ti = fit_types.begin() ;
  foreach(p,plist,Plist<GeneratorDirective::Param>) {
    C_Expr *e = NULL;
    switch(p->tag) {
    case GeneratorDirective::Residual: {
      C_Decl *resparm = new C_Decl(VarDcl,ti->copy(),"residual_arg");
      resparm->pos = pos ;
      resparm->varmode(VarVisResidual,pos);
      pseudo_params->push_back(resparm);
      e = new C_Expr(C_DeRef,new C_Expr(resparm,pos),pos);
      break; }
    case GeneratorDirective::SpectimeArg:
      e = new C_Expr(C_DeRef,
                     new C_Expr((*pseudo_params)[p->argpos-1],pos),
                     pos);
      if ( !e->type->equal(**ti) )
        e = new C_Expr(ti->copy(),e,pos);
      break ;
    case GeneratorDirective::ConstInteger:
      if ( ti->tag != Primitive )
        Diagnostic(WARNING,pos) << '`' << p->constval
                                << "' given where non-primitive "
                                   "type expected" ;
      e = new C_Expr(ti->copy(),p->constval,pos);
      break ;
    case GeneratorDirective::ConstString:
      if ( ti->tag != Pointer ||
           ti->ptr_next()->tag != Primitive ||
           ti->ptr_next()->basetype() != Char &&
           ti->ptr_next()->basetype() != UChar &&
           ti->ptr_next()->basetype() != SChar )
        Diagnostic(WARNING,pos) << p->constval
                                << "' given where nonstring expected" ;
      e = new C_Expr(ti->copy(),p->constval,pos);
      break ;
    default:
      assert(0);
    }
    result->push_back(e);
    ti++ ;
  }
}

static C_Expr *
pseudoprintf()
{
  Plist<C_Type> *ts = new Plist<C_Type>(new C_Type(".string.",Void));
  C_Type *funtype = new C_Type(ts,new C_Type(Void),true);
  C_Type *funptype = new C_Type(FunPtr,funtype);
  return new C_Expr(funptype,"create-residual-name",Position());
}

/////////////////////////////////////////////////////////////////////////////

static void
main_violated(Position p1,Position p2)
{
  Diagnostic d(ERROR,p1);
  d << "goal directive cannot coexist with other goal" ;
  d.addline() << "or generator directives" ;
  d.addline(p2) << "but here is another one" ;
}

void
GeneratorDirective::g2core(C_Pgm &pgm)
{
  static GeneratorDirective *prior = NULL ;
  if ( prior == NULL )
    prior = this ;
  else {
    if ( strcmp(generator_name,"main") == 0 )
      main_violated(pos,prior->pos);
    else if ( strcmp(prior->generator_name,"main") == 0 )
      main_violated(prior->pos,pos);
  }
  if ( subject_def == NULL ) {
    Diagnostic(ERROR,pos) << "no " << subject_name << "() function" ;
    return ;
  }
  C_Decl *realfun = subject_def->translated ;
  assert(realfun != NULL);
  Plist<C_Type>& realpars = realfun->type->fun_params() ;
  C_Type *rettype = realfun->type->fun_ret()->copy() ;

  // First, make sure the number of parameters match that of the
  // original function
  if ( params->size() != realpars.size() ) {
    Diagnostic d(ERROR,pos);
    d << params->size() << " argument specifications for "
      << subject_name << " given, but" ;
    d.addline(realfun->pos) << subject_name << "() takes "
                            << realpars.size()
                            << " parameters" ;
    return ;
  }

  // Try to parse the residual-name template and make a list of
  // types for the ellipsis arguments
  Plist<C_Type> residual_name_ellipsis ;
  parse_printf_specification(residual_name_base,residual_name_ellipsis,pos);

  // Check if the number we reached match what the user specified
  if ( residual_name_ellipsis.size() != residual_name_parts->size() ) {
    Diagnostic(ERROR,pos)
      << residual_name_base << " takes "
      << residual_name_ellipsis.size() << " extra arguments, but "
      << residual_name_parts->size() << " are provided" ;
    return ;
  }

  // Find out how many parameters the entry point should have,
  // and allocate an array for the types
  unsigned entryparams = 0 ;
  foreach(p,*params,Plist<Param>)
    if ( p->tag == SpectimeArg && p->argpos > entryparams )
      entryparams = p->argpos ;
  foreach(pp,*residual_name_parts,Plist<Param>)
    if ( pp->tag == SpectimeArg && pp->argpos > entryparams )
      entryparams = pp->argpos ;
  Plist<C_Type> *entryparamtypes = new Plist<C_Type>[entryparams] ;
  if ( entryparamtypes == NULL ) {
    Diagnostic d(FATAL,pos);
    d << "Out of memory" ;
    d.addline() << "trying to allocate space for " << entryparams
                << "static arguments for " << generator_name ;
  }

  // traverse the argument specifications and collect type proposals
  // for each entry in the type array.
  if ( true ) {
    Plist<C_Type>::iterator rpi = realpars.begin() ;
    foreach(p,*params,Plist<Param>) {
      if ( p->tag == SpectimeArg )
        entryparamtypes[p->argpos-1].push_back(*rpi);
      rpi++ ;
    }
    rpi = residual_name_ellipsis.begin();
    foreach(ppp,*residual_name_parts,Plist<Param>) {
      if ( ppp->tag == SpectimeArg )
        entryparamtypes[ppp->argpos-1].push_back(*rpi);
      rpi++ ;
    }
  }

  // OK, now create the entry point param C_Decls
  Plist<C_Decl> *pseudoparamlist = new Plist<C_Decl>;
  for(unsigned i=0;i<entryparams;i++) {
    C_Decl *newparmdecl= new C_Decl(VarDcl,
                                    negotiate_type(entryparamtypes[i],pos,i+1),
                                    "goalparam") ;
    newparmdecl->pos = pos ;
    newparmdecl->varmode(VarVisSpectime,pos);
    pseudoparamlist->push_back(newparmdecl);
  }
  delete[] entryparamtypes ;

  // create the two call statements. Along the way we add the
  // residual pseudoparams ;
  Plist<C_Expr> *call1paramlist = new Plist<C_Expr>;
  Plist<C_Expr> *call2paramlist = new Plist<C_Expr>;

  call1paramlist->push_back(new C_Expr(new C_Type(".string.",Void),
                                       residual_name_base,pos));
  convert_params(*residual_name_parts,residual_name_ellipsis,
                 call1paramlist,pseudoparamlist,pos);
  convert_params(*params,realpars,call2paramlist,pseudoparamlist,pos);

  C_Stmt *stmt1 = new C_Stmt(pseudoprintf(),call1paramlist,pos);
  C_Stmt *stmt2 = new C_Stmt(new C_Expr(realfun,pos),
                             call2paramlist,pos);
  C_BasicBlock *bb = new C_BasicBlock ;
  bb->getBlock().push_back(stmt1);
  bb->getBlock().push_back(stmt2);

  // create the pseudo-function's type, and the function itself
  Plist<C_Type> *pseudoparamtypes = new Plist<C_Type> ;
  foreach(ppar,*pseudoparamlist,Plist<C_Decl>)
    pseudoparamtypes->push_back(ppar->type);
  C_Type *pseudofunctiontype = new C_Type(pseudoparamtypes,rettype);
  C_Decl *pseudofun = new C_Decl(FunDf,pseudofunctiontype,generator_name);
  pseudofun->pos = pos ;
  pseudofun->fun_params(pseudoparamlist);

  // if there is a function return value, communicate that
  Plist<C_Decl> *localvar = new Plist<C_Decl> ;
  if ( rettype->isVoid() ) {
    bb->exit(new C_Jump(NULL,pos));
  } else {
    C_Decl *retvar = new C_Decl(VarDcl,rettype->copy(),"r");
    retvar->pos = pos ;
    retvar->varmode(VarVisResidual,pos) ;
    stmt2->target(new C_Expr(retvar,pos));
    localvar->push_back(retvar);
    bb->exit(new C_Jump(new C_Expr(C_DeRef,new C_Expr(retvar,pos),
                                   pos),pos));
  }
  pseudofun->fun_locals(localvar);
  pseudofun->blocks(new Plist<C_BasicBlock>(bb));

  // insert the pseudo-function into the program
  pgm.generators.push_back(pseudofun);
}
