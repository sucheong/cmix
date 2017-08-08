/* $Id: float.h,v 1.1.1.1 1999/02/22 13:50:37 makholm Exp $
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix shadow header: stdio.h
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 */

#ifndef __CMIX
# error This file is only intended to be included from C-Mix/II
#endif

#ifndef __CMIX_FLOAT__
#define __CMIX_FLOAT__

/* ISO C 7.1.5 Limits */
#pragma cmix header: <float.h>

/* ISO C 5.2.4.2.2 Characteristics of floating types */
extern const int   FLT_RADIX;
extern const int   FLT_ROUNDS;
extern const int   FLT_MANT_DIG,   DBL_MANT_DIG,   LDBL_MANT_DIG;
extern const int   FLT_DIG,        DBL_DIG,        LDBD_DIG;
extern const int   FLT_MIN_EXP,    DBL_MIN_EXP,    LDBL_MIN_EXP;
extern const int   FLT_MIN_10_EXP, DBL_MIN_10_EXP, LDBL_MIN_10_EXP;
extern const int   FLT_MAX_EXP,    DBL_MAX_EXP,    LDBL_MAX_EXP;
extern const int   FLT_MAX_10_EXP, DBL_MAX_10_EXP, LDBL_MAX_10_EXP;
extern const float FLT_MAX,        DBL_MAX,        LDBL_MAX;
extern const float FLT_EPSILON,    DBL_EPSILON,    LDBL_EPSILON;
#pragma cmix well-known constant: FLT_RADIX
#pragma cmix well-known constant: FLT_ROUNDS
#pragma cmix well-known constant: FLT_MANT_DIG   DBL_MANT_DIG   LDBL_MANT_DIG
#pragma cmix well-known constant: FLT_DIG        DBL_DIG        LDBD_DIG
#pragma cmix well-known constant: FLT_MIN_EXP    DBL_MIN_EXP    LDBL_MIN_EXP
#pragma cmix well-known constant: FLT_MIN_10_EXP DBL_MIN_10_EXP LDBL_MIN_10_EXP
#pragma cmix well-known constant: FLT_MAX_EXP    DBL_MAX_EXP    LDBL_MAX_EXP
#pragma cmix well-known constant: FLT_MAX_10_EXP DBL_MAX_10_EXP LDBL_MAX_10_EXP
#pragma cmix well-known constant: FLT_MAX        DBL_MAX        LDBL_MAX
#pragma cmix well-known constant: FLT_EPSILON    DBL_EPSILON    LDBL_EPSILON


#endif
