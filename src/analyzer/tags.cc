/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: 
 * History:  Derived from code by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "tags.h"
#include "strings.h"
#include "diagnostic.h"

const char*
basetype2str(BaseTypeTag bt)
{
    switch (bt) {
      case Void:	return str_void;
      case Char:	return str_char;
      case SChar:       return str_schar;
      case UChar:	return str_uchar;
      case ShortInt:	return str_short;
      case UShortInt:	return str_ushort;
      case Int:		return str_int;
      case UInt:	return str_uint;
      case LongInt:	return str_long;
      case ULongInt:	return str_ulong;
      case Float:	return str_float;
      case Double:	return str_double;
      case LongDouble:	return str_ldouble;
    case AbsT: case FloatingAbsT: case UAbsT: break ;
    }
    Diagnostic(INTERNAL,RIGHT_HERE)
        << "abstract type hwith no canonical names";
    return NULL;
}

const char*
usertype2str(UserTag tag)
{
    switch (tag) {
    case Struct: return str_struct;
    case Union:  return str_union;
    case Enum:   return str_enum;
    }
    Diagnostic(INTERNAL,RIGHT_HERE) << "this should be unreachable";
    return NULL;
}

const char*
unary2str(UnOp op)
{
    switch (op) {
      case Addr:	return str_addr;
      case DeRef:	return str_mul;
      case Pos:		return str_plus;
      case Neg:		return str_minus;
      case Not:		return str_neg;
      case Bang:	return str_bang;
    }
    Diagnostic(INTERNAL,RIGHT_HERE) << "this should be unreachable";
    return NULL;
}

const char*
binary2str(BinOp op)
{
    switch (op) {
      case Mul:		return str_mul;
      case Div:		return str_div;
      case Mod:		return str_mod;
      case Add:		return str_plus;
      case Sub:		return str_minus;
      case LShift:	return str_lshift;
      case RShift:	return str_rshift;
      case LT:		return str_less;
      case GT:		return str_greater;
      case LEq:		return str_lesseq;
      case GEq:		return str_greatereq;
      case Eq:		return str_equal;
      case NEq:		return str_notequal;
      case BAnd:	return str_addr;
      case BEOr:	return str_bxor;
      case BOr:		return str_bor;
      case And:		return str_and;
      case Or:		return str_or;
    }
    Diagnostic(INTERNAL,RIGHT_HERE) << "this should be unreachable";
    return NULL;
}

int
binary2prec(BinOp bop)
{
    switch(bop) {
    case Mul:
    case Div:
    case Mod:    return 25 ;
    case Add:
    case Sub:    return 23 ;
    case LShift:
    case RShift: return 21 ;
    case LT:
    case GT:
    case LEq:
    case GEq:    return 19 ;
    case Eq:
    case NEq:    return 17 ;
    case BAnd:   return 15 ;
    case BEOr:   return 13 ;
    case BOr:    return 11 ;
    case And:    return 9 ;
    case Or:     return 7 ;
    }
    Diagnostic(INTERNAL,RIGHT_HERE) << "this should be unreachable";
    return 0 ;
}

bool constvol::subsetof(constvol const &o) const {
    return !( cst && !o.cst ||
              vol && !o.vol );
}

void constvol::operator+=(constvol const &o) {
    cst |= o.cst ;
    vol |= o.vol ;
}
