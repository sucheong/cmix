/* $Id: setjmp.h,v 1.2 1999/05/01 22:32:27 makholm Exp $
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

#ifndef __CMIX_SETJMP__
#define __CMIX_SETJMP__

/* ISO C 7.6 Nonlocal jumps */
#pragma cmix header: <setjmp.h>

typedef __CMIX() jmp_buf;
#pragma cmix taboo: setjmp longjmp
#define setjmp(X) CMix_cannot_handle_setjmp_or_longjmp
#define longjmp(X,Y) CMix_cannot_handle_setjmp_or_longjmp

#endif
