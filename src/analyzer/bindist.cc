/* (-*-c++-*-)
 * Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Generic versions of configurable literal strings
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>

#ifdef HAVE_GETOPT_LONG
#error Configure with --without-gnu-getopt if you want to create bindists
.. GNU getopt is GLPL source so we must not distribute bindists containing
it without also releasing full sources. Which we do not want at the moment.
#endif

char CPP[] = "/usr/lib/cpp" ;
char CMIX_SHADOW_DIR[] = "/usr/local/share/cmix/shadow" ;

