/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix gegen subsystem: Expression and type output
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "gegen.h"
#include "options.h"

static bool
MayBeStaticNull(C_Expr *e,BTresult const &bt)
{
  while(1) {
    switch(e->tag) {
    case C_FunAdr:
    case C_Var:
    case C_Null: /* ok, this _can_ be NULL, but never makes a hole */
      return false ;
    case C_Array:
    case C_Member:
      e = e->subexpr() ;
      break ;
    case C_PtrArith:
      e = e->binary_expr1();
      break ;
    default:
      return bt.Static(e->type);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
//  L I F T S
//

static void
LvalLift(C_Expr *expr,ostream &ost,Plist<C_Expr> &ipl)
{
  // either: dereferencing a spectime pointer to residual data
  //     or: mentioning the name of a residual variable in lval position
  ipl.push_back(expr);
  ost << '?' ;
}

static bool
ShouldLiftRval(C_Expr *expr)
{
  switch(expr->tag) {
  case C_Null:
  case C_EnumCnst:
  case C_Cnst:
  case C_ExtFun:
  case C_FunAdr:
    return false ;
  case C_Unary:
    return ShouldLiftRval(expr->subexpr());
  case C_Binary:
    return ShouldLiftRval(expr->binary_expr1())
      || ShouldLiftRval(expr->binary_expr2()) ;
  case C_Cast:
    // I: when lifting anything else than primitive types, do
    // it at residual time. In practise, "anything else" is a
    // pointer lift+cast. The pointer lift does not know about
    // the type, so we should leave the cast in the residual
    // program as a type specification, lest the original type
    // is invalid in the context without an explicit cast.
    if ( expr->type->tag != Primitive ||
         expr->subexpr()->type->tag != Primitive )
      return false ;
    // II: if chars are involved, lift the char, not the non-char
    if ( expr->type->basetype() == Char ) return true ;
    if ( expr->subexpr()->type->basetype() == Char ) return false ;
    if ( expr->type->basetype() == UChar ) return true ;
    if ( expr->subexpr()->type->basetype() == UChar ) return false ;
    //  (b) else prefer casting at spectime, unless the subexpression
    //      does not want to be lifted itself
    return ShouldLiftRval(expr->subexpr());
  case C_PtrCmp:
  case C_DeRef:
  case C_SizeofT:
  case C_SizeofE:
    return true ;
  case C_Var:
  case C_PtrArith:
  case C_Array:
  case C_Member:
    // return false here, not because we do not lift these
    // expressions, but because they get handled in EmitDeref
    return false ;
  }
  return true ;
}

void
GegenStream::std_hole(C_Type *t,int prec)
{
  switch(t->tag) {
  case Primitive:
    switch(t->basetype()) {
    case UChar: case SChar:
    case AbsT: case UAbsT: case FloatingAbsT:
      ost << '(' << t->primitive() << ")?" ;
      return ;
    case Char:
    case Int: case ShortInt:
    case Float: case Double: case LongDouble:
      ost << '?' ;
      return ;
    case UInt: case UShortInt:
      ost << "?u" ;
      return ;
    case LongInt:
      ost << "?L" ;
      return ;
    case ULongInt:
      ost << "?ul" ;
      return ;
    case Void:
      break ;
    }
    break ;
  case Pointer:
    if ( bt.Static(t->ptr_next()) )
      ost << '?' ; // a string lift
    else {
      // a *Code lift
      if ( prec > 27 )
        ost << "(&?)" ;
      else
        ost << "&?" ;
    }
    return ;
  default:
    break ;
  }
  Diagnostic(INTERNAL,RIGHT_HERE) << "unliftable lift" ;
}

static char const *
fill_string(C_Type *t,BTresult const &bt)
{
  switch( t->tag ) {
  case Primitive:
    switch(t->basetype()) {
    case AbsT: case SChar: case ShortInt: case Int: case LongInt:
      return "cmixLiftSigned(" ;
    case UChar:
      if ( lift_uchar_as_char )
        return "cmixLiftChar(" ;
      else
        return "cmixLiftUnsigned(" ;
    case UAbsT: case UShortInt: case UInt: case ULongInt:
      return "cmixLiftUnsigned(" ;
    case Char:
      return "cmixLiftChar(" ;
    case Float:
      return "cmixLiftFloat(0," ;
    case Double:
      return "cmixLiftFloat(1," ;
    case LongDouble:
      if ( lift_long_double )
        return "cmixLiftLongDouble(" ;
      else {
        static bool warned = false ;
        if ( !warned ) {
          Diagnostic d(WARNING,Position());
          d << "Lifting a long double truncates it to double" ;
          d.addline()
            << "(use the `lift long double' directive if you don't want this)";
        }
        warned = true ;
        return "cmixLiftFloat(2," ;
      }
    case FloatingAbsT:
      if ( lift_long_double )
        return "cmixLiftLongDouble(" ;
      else
        return "cmixLiftFloat(1," ;
    default:
      break ;
    }
    break ;
  case Pointer:
    if ( bt.Static(t->ptr_next()) )
      return "cmixLiftString(" ;
    else
      return "cmixLiftCode(" ; // only used for function returns
  default:
    break ;
  }
  assert(0);
  return "! ERROR HERE !" ;
}

void
GegenStream::fill_hole(C_Expr *e)
{
  if ( e->tag == C_Member && bt.Dynamic(e->type) ) {
    // we're asked to interpolate the name of a field in a
    // dynamic struct
    ost << "cmixMember."
        << names[e->subexpr()->type->ptr_next()->user_def()] << '.'
        << names[e->struct_name()] ;
    return ;
  }
  
  if ( e->type->tag != Pointer || bt.Static(e->type->ptr_next()) ) {
    // a vanilla lift, though possibly a string lift
    *this << fill_string(e->type,bt) << EmitExpr(e,0) << ")" ;
    return ;
  }
  
  // If nothing of the above matched, we're looking at an expression of
  // type 'pointer to Code', or 'pointer to struct with .cmix == Code'.
  // First find out which is the case
  if ( *dotcmix(e->type->ptr_next()) ) {
    if ( MayBeStaticNull(e,bt) )
      *this << "cmixLiftStructPtr(" << EmitExpr(e,0) << ")" ;
    else
      *this << EmitDeref(e,29) << ".cmix" ;
  } else {
    // it was straight pointer to Code
    if ( MayBeStaticNull(e,bt) )
      *this << "cmixLiftPtr(" << EmitExpr(e,0) << ")" ;
    else
      *this << EmitDeref(e,2);
  }
}

char const *
GegenStream::EmitLiftingHole(C_Type *t)
{
  userhole() ;
  std_hole(t,5);
  return fill_string(t,bt);
}

/////////////////////////////////////////////////////////////////////////////
//
//  D E R E F E R E N C E D   E X P R E S S I O N S
//

GegenStream &
GegenStream::operator<<(EmitDeref ed)
{
  if ( !pgenMode && bt.Static(ed.expr->type) ) {
    LvalLift(ed.expr,ost,interpolate);
    nullprotect = false ;
  }
  else switch (ed.expr->tag) {
  case C_Var:
    if ( pgenMode ) {
      assert(names[ed.expr->var()] != NULL);
      ost << names[ed.expr->var()] ;
    } else {
      assert(!nullprotect);
      LvalLift(ed.expr,ost,interpolate);
    }
    break ;
  case C_Member: {
    C_Expr *e = ed.expr->subexpr() ;
    if ( ed.prec > 29 ) ost << '(' ;
    if ( e->tag == C_DeRef ) {
       *this << EmitExpr(e,29);
       ost << "->" ;
     } else {
       *this << EmitDeref(e,29);
       ost << '.' ;
     }
     if ( pgenMode ) {
       C_UserMemb *m = ed.expr->struct_name() ;
       assert(names[m] != NULL);
       ost << names[m] ;
     } else {
       assert(!nullprotect);
       ost << '?' ;
       interpolate.push_back(ed.expr);
     }
     if ( ed.prec > 29 ) ost << ')' ;
     break ; }
   case C_PtrArith:
     if ( ed.expr->binary_op() == Add ) {
       if ( ed.prec > 29 ) ost << '(' ;
       *this << EmitExpr(ed.expr->binary_expr1(),29);
       ost << '[' ;
       *this << EmitExpr(ed.expr->binary_expr2(),0);
       ost << ']' ;
       if ( ed.prec > 29 ) ost << ')' ;
       break ;
     } // else fall through
   default:
     if ( ed.prec > 27 ) ost << '(' ;
     ost << '*' ;
     *this << EmitExpr(ed.expr,27);
     if ( ed.prec > 27 ) ost << ')' ;
     break ;
   }
   return *this ;
 }

 /////////////////////////////////////////////////////////////////////////////
 //
 //  N O M I N A L   E X P R E S S I O N S
 //

 static void
 escaping_print(const char *s, ostream &ost) {
   for ( ; *s ; s++ )
     switch(*s) {
     case '"':
     case '\\':
       ost << '\\' << *s ;
       break ;
     case '?':
       ost << "#!" ;
       break ;
     case ':':
       ost << "#;" ;
       break ;
     case '\'':
       ost << "#," ;
       break ;
     case '#':
       ost << "#=" ;
       break ;
     default:
       ost << *s ;
       break ;
     }
 }

 GegenStream &
 GegenStream::operator<<(EmitExpr ee)
 {
   assert( ee.expr->type->tag != Function );

   if ( !pgenMode && !nullprotect &&
        ee.expr->type->tag == Pointer && MayBeStaticNull(ee.expr,bt) ) {
     nullprotect = true ;
     *this << ":" << ee << "'" ;
     assert(!nullprotect);
     if ( ee.prec > 27 ) ost << '(' ;
     *this << "(" << AbstractDecl(ee.expr->type) << ")0" ;
     if ( ee.prec > 27 ) ost << ')' ;
     ost << '\'' ;
   }   
   else if ( !pgenMode && bt.Static(ee.expr->type) &&
             ShouldLiftRval(ee.expr) ) {
     std_hole(ee.expr->type,ee.prec);
     interpolate.push_back(ee.expr);
     nullprotect = false ;
   }
   else switch (ee.expr->tag) {
   case C_Null:
     if ( ee.prec > 27 ) ost << '(' ;
     *this << "(" << AbstractDecl(ee.expr->type) << ")0" ;
     if ( ee.prec > 27 ) ost << ')' ;
     break ;
   case C_Cnst:
     if ( pgenMode )
       ost << ee.expr->cnst() ;
     else
       escaping_print(ee.expr->cnst(),ost);
     break ;
   case C_EnumCnst:
     ost << names[ee.expr->enum_cnst()] ;
     break ;
   case C_ExtFun:
     ost << ee.expr->var()->get_name() ;
     break ;
   case C_FunAdr:
     assert(pgenMode);
     ost << names[ee.expr->var()] ;
     break ;
   case C_Var:
   case C_Member:
     if ( ee.prec > 27 ) ost << '(' ;
     *this << "&" << EmitDeref(ee.expr,27);
     if ( ee.prec > 27 ) ost << ')' ;
     break ;
   case C_Array:
   case C_DeRef:
     *this << EmitDeref(ee.expr->subexpr(),ee.prec);
     break ;
   case C_Unary:
     if ( ee.prec > 27 ) ost << '(' ;
     *this << ' ' << unary2str(ee.expr->unary_op())
           << EmitExpr(ee.expr->subexpr(),27) ;
     if ( ee.prec > 27 ) ost << ')' ;
     break ;
   case C_PtrArith:
   case C_PtrCmp:
   case C_Binary: {
     int myprec = binary2prec(ee.expr->binary_op());
     if ( ee.prec > myprec ) ost << '(' ;
     *this << EmitExpr(ee.expr->binary_expr1(),myprec)
           << " " << binary2str(ee.expr->binary_op()) << " "
           << EmitExpr(ee.expr->binary_expr2(),myprec+1);
     if ( ee.prec > myprec ) ost << ')' ;
     break ; }
   case C_SizeofE:
   case C_SizeofT:
     if ( ee.prec > 27 ) ost << '(' ;
     *this << "sizeof(" << AbstractDecl(ee.expr->sizeof_type()) << ")";
     if ( ee.prec > 27 ) ost << ')' ;
     break ;
   case C_Cast:
     if ( ee.prec > 27 ) ost << '(' ;
     *this << "(" << AbstractDecl(ee.expr->type) << ")"
           << EmitExpr(ee.expr->subexpr(),27);
     if ( ee.prec > 27 ) ost << ')' ;
     break ;
   }

   return *this ;
 }

 /////////////////////////////////////////////////////////////////////////////
 //
 //  R E N D E R I N G   T Y P E S
 //

 void
 GegenStream::emitcv(constvol cv,C_Type *t)
 {
   if ( pgenMode && bt.Dynamic(t) )
     return ;
   if ( cv.cst ) ost << "const " ;
   if ( cv.vol ) ost << "volatile " ;
 }

 void
 GegenStream::typerec(C_Type *type, Direction dir, bool paren_at_post)
 {
   if( pgenMode && bt.Dynamic(type) && (type->tag!=StructUnion||alwayscode) ) {
     // This is a Code object
     if ( dir == Prefix )
       ost << "Code ";
     return ;
   }
   alwayscode = false ;
   switch(type->tag) {
   case Abstract:
   case Primitive:
     if ( dir == Prefix )
       ost << type->primitive() << ' ' ;
     break;
   case EnumType:
     if ( dir == Prefix )
       ost << "enum " << names[type->enum_def()] << ' ' ;
     break;
   case StructUnion:
     if ( dir == Prefix ) {
       if ( type->user_def()->isUnion &&
            !(pgenMode && bt.Dynamic(type)) )
         ost << "union " ;
       else
         ost << "struct " ;
       if ( !pgenMode && residual_structs.find(type->user_def()) )
         ost << type->user_def()->get_name() ;
       else
         ost << names[type->user_def()] ;
       ost << ' ' ;
     }
     break;
   case FunPtr:
   case Pointer:
     typerec(type->ptr_next(),dir,true);
     if ( dir == Prefix ) {
       emitcv(type->qualifiers(),type->ptr_next());
       ost << '*' ;
     }
     break;
   case Array:
     if ( dir == Postfix ) {
       if ( paren_at_post )
         ost << ')' ;
      ost << '[' ;
      if ( type->hasSize() )
        *this << EmitExpr(type->array_size(),0);
      ost << ']' ;
     }
     typerec(type->array_next(),dir,false);
     if ( dir == Prefix ) {
       emitcv(type->qualifiers(),type->array_next());
       if ( paren_at_post )
         ost << '(' ;
     }
     break;
   case Function:
     if ( dir == Postfix ) {
       if ( paren_at_post )
         ost << ')' ;
       Plist<C_Type> &paramlist = type->fun_params() ;
       bool first = true ;
       foreach(i,paramlist,Plist<C_Type>) {
         alwayscode = true ;
         *this << ( first ? "(" : "," ) << AbstractDecl(*i);
         first = false ;
       }
       ost << ( type->fun_varargs()
                ? ( first ? "()" : ",...)" )
                : ( first ? "(void)" : ")" ) );
     }
     alwayscode = true ;
     typerec(type->fun_ret(),dir,false);
     if ( dir == Prefix && paren_at_post )
       ost << '(' ;
     break;
   }
 }


void
alwayscode(GegenStream *gs)
{
  gs->alwayscode = true ;
}

GegenStream &
GegenStream::operator<<(EmitType et)
{
  typerec(et.type,Prefix,false);
  alwayscode = false ;
  emitcv(et.cv,et.type);
  waitingtypes.push_back(et.type);
  if ( et.middle != NULL ) {
    ost << et.middle ;
    poptype(this);
  }
  return *this ;
}

void
poptype(GegenStream *gs)
{
  assert(!gs->waitingtypes.empty());
  C_Type *t = gs->waitingtypes.back();
  gs->waitingtypes.pop_back();
  gs->typerec(t,GegenStream::Postfix,false);
}

GegenStream &
GegenStream::operator<<(C_Decl *d)
{
  *this << pushtype(d->type,d->qualifiers) ;
  if ( pgenMode )
    ost << names[d] ;
  else {
    ost << '?' ;
    interpolate.push_back(new C_Expr(d,d->pos));
  }
  return *this << poptype ;
}

/////////////////////////////////////////////////////////////////////////////
//
//  R E N D E R I N G   I N I T I A L I Z E R S
//

GegenStream &
GegenStream::operator<<(C_Init *i)
{
  if ( pgenMode && bt.Dynamic(i->owner()->type) )
    *this << "cmixMkExp(" << enter << i << exit << ")" ;
  else switch(i->tag) {
  case Simple:
    *this << EmitExpr(i->simple_init(),2);
    break ;
  case StringInit:
    if ( pgenMode )
      ost << i->string_init() ;
    else
      escaping_print(i->string_init(),ost);
    break ;
  case FullyBraced:
  case SloppyBraced:
    ost << '{' ;
    bool do_newlines = false ;
    foreach(j,i->braced_init(),Plist<C_Init>)
      if ( j->tag == FullyBraced || j->tag == SloppyBraced )
        do_newlines = true ;
    foreach(jj,i->braced_init(),Plist<C_Init>) {
      *this << *jj << "," ;
      if ( do_newlines )
        ost << ( pgenMode ? "\n\t" : "\\n\\t\"\n\t\"" );
    }
    ost << '}' ;
  }
  return *this ;
}

/////////////////////////////////////////////////////////////////////////////
//
//  R E S I D U A L   T E M P L A T E   B U I L D I N G
//

void
enter(GegenStream *gs)
{
  assert(gs->pgenMode);
  assert(!gs->exit2pending);
  assert(gs->waitingtypes.empty());
  gs->interpolate1 = -1 ;
  gs->pgenMode = false ;
  gs->nullprotect = false ;
  gs->ost << '"' ;
}

void
GegenStream::userhole()
{
  assert(!pgenMode);
  assert(interpolate1 == -1);
  interpolate1 = interpolate.size() ;
}

void
userhole(GegenStream *gs)
{
  gs->userhole() ;
  gs->ost << '?' ;
}

void
exit(GegenStream *gs)
{
  unsigned max = gs->interpolate.size() ;
  assert(gs->waitingtypes.empty());
  assert(gs->exit2pending == gs->pgenMode);
  if ( gs->exit2pending ) {
    gs->exit2pending = false ;
  } else {
    gs->pgenMode = true ;
    gs->ost << '"' ;
    if ( gs->interpolate1 >= 0 ) {
      max = gs->interpolate1 ;
      gs->exit2pending = true ;
    }
  }
  for(Plist<C_Expr>::mutable_iterator i=gs->interpolate.begin();max;max-- ) {
    gs->ost << ',' ;
    gs->fill_hole(*i) ;
    gs->interpolate.erase(i);
  }
  assert(gs->exit2pending || gs->interpolate.empty());
}

/////////////////////////////////////////////////////////////////////////////
//
//  B A S I C   G E G E N S T R E A M   M E M B E R   F U N C T I O N S
//

GegenStream::GegenStream(ostream &o, BTresult const &b,
                         NameMap const &n,Pset<C_UserDef> &r)
  : ost(o), bt(b), names(n), residual_structs(r)
{
  pgenMode = true ;
  exit2pending = false ;
  nullprotect = false ;
  alwayscode = false ;
}

GegenStream::~GegenStream()
{
  assert(pgenMode);
  assert(interpolate.empty());
  assert(waitingtypes.empty());
}
