/* $Id: locale.h,v 1.2 1999/05/01 22:32:26 makholm Exp $
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

#ifndef __CMIX_LOCALE__
#define __CMIX_LOCALE__

/* ISO C 7.4 Localization */
#pragma cmix header: <locale.h>

struct lconv {
  char *decimal_point;
  char *thousands_sep;
  char *grouping;
  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char *int_frac_digits;
  char *frac_digits;
  char *p_cs_precedes;
  char *p_sep_by_space;
  char *n_cs_precedes;
  char *n_sep_by_space;
  char *p_sign_posn;
  char *n_sign_posn;
};
#pragma cmix well-known: lconv

#ifndef NULL
# pragma cmix taboo: NULL
# define NULL ((void*)0)
#endif

extern int LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC, LC_TIME;
#pragma cmix well-known: LC_ALL LC_COLLATE LC_CTYPE LC_MONETARY
#pragma cmix well-known: LC_NUMERIC LC_TIME

/* ISO C 7.4.1 Locale control */
char *setlocale(int, const char*);
#pragma cmix well-known: setlocale

/* ISO C 7.4.2 Numeric formatting convention inquiry */
struct lconv *localeconv(void);
#pragma cmix well-known: localeconv

#endif
