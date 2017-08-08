/* $Id: types.h,v 1.1.1.1 1999/02/22 13:50:40 makholm Exp $
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

#ifndef __CMIX_TYPES__
#define __CMIX_TYPES__

#pragma cmix header: <sys/types.h>

typedef __CMIX()  physadr;
typedef __CMIX()  daddr_t;
typedef __CMIX()  caddr_t;
typedef __CMIX(i) uint;
typedef __CMIX(i) ushort;
typedef __CMIX()  ino_t;
typedef __CMIX()  cnt_t;
typedef __CMIX(i) time_t;
typedef __CMIX()  dev_t;
typedef __CMIX(i) off_t;
typedef __CMIX(i) paddr_t;
typedef __CMIX()  key_t;
typedef __CMIX()  pid_t;
typedef __CMIX()  uid_t;
typedef __CMIX()  gid_t;

#endif
