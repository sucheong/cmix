/* $Id: errno.h,v 1.1.1.1 1999/02/22 13:50:36 makholm Exp $
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

#ifndef __CMIX_ERRNO__
#define __CMIX_ERRNO__

/* ISO C 7.1.4 Errors */
#pragma cmix header: <errno.h>

extern const int EDOM, ERANGE;
#pragma cmix well-known constant: EDOM ERANGE

extern const int errno;
#pragma cmix well-known constant: errno

#endif
