/* $Id: assert.h,v 1.2 1999/03/30 14:23:13 jpsecher Exp $
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

#ifndef __CMIX_ASSERT__
#define __CMIX_ASSERT__

/* ISO C 7.2 Diagnostics */
#pragma cmix header: <assert.h>

void assert(int);
#pragma cmix well-known: assert
#pragma cmix pure: assert()

#endif
