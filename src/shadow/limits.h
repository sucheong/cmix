/* $Id: limits.h,v 1.1.1.1 1999/02/22 13:50:37 makholm Exp $
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix shadow header: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 */

#ifndef __CMIX
# error This file is only intended to be included from C-Mix/II
#endif

#ifndef __CMIX_LIMITS__
#define __CMIX_LIMITS__

/* ISO C 7.1.5 Limits */
#pragma cmix header: <limits.h>

/* ISO C 5.2.4.2.1 Sizes of integral types */
extern const int                CHAR_BIT;
extern const signed char        SCHAR_MIN, SCHAR_MAX;
extern const unsigned char      UCHAR_MAX;
extern const char               CHAR_MIN, CHAR_MAX;
extern const int                MB_LEN_MAX;
extern const short int          SHRT_MIN, SHRT_MAX;
extern const short unsigned int USHRT_MAX;
extern const int                INT_MIN, INT_MAX;
extern const unsigned int       UINT_MAX;
extern const long               LONG_MIN, LONG_MAX;
extern const unsigned long      ULONG_MAX;
#pragma cmix well-known constant: CHAR_BIT
#pragma cmix well-known constant: SCHAR_MIN SCHAR_MAX
#pragma cmix well-known constant: UCHAR_MAX
#pragma cmix well-known constant: CHAR_MIN CHAR_MAX
#pragma cmix well-known constant: MB_LEN_MAX
#pragma cmix well-known constant: SHRT_MIN SHRT_MAX
#pragma cmix well-known constant: USHRT_MAX
#pragma cmix well-known constant: INT_MIN INT_MAX
#pragma cmix well-known constant: UINT_MAX
#pragma cmix well-known constant: LONG_MIN LONG_MAX
#pragma cmix well-known constant: ULONG_MAX

#endif
