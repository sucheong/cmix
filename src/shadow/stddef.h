/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix shadow header: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 */

#ifndef __CMIX
# error This file is only intended to be included from C-Mix/II
#endif

#ifndef __CMIX_STDDEF__
#define __CMIX_STDDEF__

/* ISO C 7.1.6 Common definitions */
#pragma cmix header: <stddef.h>
typedef __CMIX(is) ptrdiff_t;
typedef __CMIX(iu) size_t;
typedef __CMIX(i) wchar_t;

#ifndef NULL
# pragma cmix taboo: NULL
# define NULL ((void*)0)
#endif

#pragma cmix taboo: offsetof
#define offsetof(__s_name, __m_name) CMix_does_not_handle_offsetof

#endif
