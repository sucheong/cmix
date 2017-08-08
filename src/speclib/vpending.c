/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix speclib: driver file for pending list routines that use
 *           void* instead of unsigned to identify p_gen labels.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#define PGENLABEL void *
#define PENDFUNC(x) x ## _v
#include "pending.c"
