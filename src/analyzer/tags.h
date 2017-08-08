/* (-*-c++-*-)
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Tags used both by cpgm.h and corec.h
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __TAGS__
#define __TAGS__

#include <iostream.h>
#include "diagnostic.h"

enum SideEffects { SEPure, SEStateless, SEROState, SERWState } ;

/*  switch (effects()->theEffects()) {
    case SEPure:
      break;
    case SEStateless:
      break;
    case SEROState:
      break;
    case SERWState:
      break;
    case SENoAnno:
      break;
      }*/

enum CallTime { CTNoAnno, CTSpectime, CTResidual } ;

/* switch (calltime()->theTime()) {
 case CTSpectime:
   break;
 case CTNoAnno:
   break;
 case CTResidual:
   break;
   }*/

enum VariableMode { VarIntAuto,      // internal; BTA decides
                    VarIntResidual,  // internal; declared residual
                    VarIntSpectime,  // internal; declared spectime
                    VarVisResidual,  // declared visible residual
                    VarVisSpectime,  // declared visible spectime
                    VarExtResidual,  // external; declared residual
                    VarExtDefault,   // external; residual by default
                    VarExtSpectime,  // external; declared spectime
                    VarConstant,     // pseudo-variable: universal constant
                    VarEnum,         // pseudo-variable: enum member
                    VarMu            // not a variable: these do not apply
} ;

struct constvol {
  bool cst, vol;
  constvol() : cst(false), vol(false) {}
  constvol(bool c,bool v) : cst(c), vol(v) {}
  bool subsetof(const constvol&) const ;
  void operator+= (const constvol&) ;
};

// The order is important: used for conversions.
typedef enum { AbsT, UAbsT, FloatingAbsT, Void,
               Char, SChar, UChar,
               ShortInt, UShortInt, Int, UInt, LongInt, ULongInt,
               Float, Double, LongDouble } BaseTypeTag;

typedef enum { Struct, Union, Enum } UserTag;

typedef enum { Addr, DeRef, Pos, Neg, Not, Bang } UnOp;
/*
switch (op)
{
case Addr:
case DeRef:
case Pos:
case Neg:
case Not:
case Bang:
}
*/
typedef enum { Mul, Div, Mod, Add, Sub, LShift, RShift,
               LT, GT, LEq, GEq, Eq, NEq, BAnd, BEOr, BOr,
               And, Or} BinOp;
/*
switch (op) {
case Mul:
case Div:
case Mod:
case Add:
case Sub:
case LShift:
case RShift:
case LT:
case GT:
case LEq:
case GEq:
case Eq:
case NEq:
case BAnd:
case BEOr:
case BOr:
case And:
case Or:
}
*/

// precedence table used in outcore and gegen:
//
// 1  ,
// 3  = *= /= %= += -= >>= <<= &= ^= |=
// 5  ?:
// 7  ||
// 9  &&
// 11 |
// 13 ^
// 15 &
// 17 == !=
// 19 < > <= =>
// 21 << >>
// 23 + -
// 25 * / %
// 27 prefix
// 29 postfix
// 31 primary

int binary2prec(BinOp);

typedef enum { Character, Integer, UInteger, Long, ULong,
               Floating, DoubleF, StrConst } ConstantTag;

/*
        switch (t)
        {
        case  Integer:
        case  UInteger:
        case  Long:
        case  ULong:
        case  Floating:
        case  DoubleF:
        case  Character:
        case  StrConst:
        }

*/

const char* basetype2str(BaseTypeTag bt);
const char* usertype2str(UserTag tag);
const char* unary2str(UnOp op);
const char* binary2str(BinOp op);



#endif /* __TAGS__ */
