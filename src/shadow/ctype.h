/* $Id: ctype.h,v 1.3 1999/11/18 14:23:31 makholm Exp $
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

#ifndef __CMIX_CTYPE__
#define __CMIX_CTYPE__

/* ISO C 7.3 Character handling */
#pragma cmix header: <ctype.h>

/* ISO C 7.3.1 Character testing functions */
int isalnum(int);
int isalpha(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
#pragma cmix well-known: isalnum isalpha iscntrl isdigit isgraph
#pragma cmix well-known: islower isprint ispunct isspace isupper isxdigit
#pragma cmix pure: isalnum() isalpha() iscntrl() isdigit() isgraph()
#pragma cmix pure: islower() isprint() ispunct() isspace() isupper() isxdigit()

/* ISO C 7.3.2 Character case mapping functions */
int tolower(int);
int toupper(int);
#pragma cmix well-known: toupper tolower
#pragma cmix pure: toupper() tolower()

#endif
