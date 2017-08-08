/* $Id: stdarg.h,v 1.1.1.1 1999/02/22 13:50:38 makholm Exp $
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

#ifndef __CMIX_STDARG__
#define __CMIX_STDARG__

/* ISO C 7.8 Variable arguments */
#pragma cmix header: <stdarg.h>

typedef __CMIX() va_list;

/* ISO C 7.8.1 Variable argument list access macros */
#define va_start(X,Y) CMix_does_not_handle_variable_length_argument_lists
#define va_arg(X,Y) CMix_does_not_handle_variable_length_argument_lists
#define va_end(X) CMix_does_not_handle_variable_length_argument_lists

#endif
