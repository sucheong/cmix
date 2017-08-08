/* Content, cmixconf.h:	 C-Mix system: Parameters from `configure script`
 * Content, config.in:   Template for generating cmixconf.h
 * Content, acconfig.h:	 Template for generating config.in
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __CMIX_CONFIGURE__
#define __CMIX_CONFIGURE__

@TOP@

/* define if system headers assume it is defined (sigh, hp-sux) */
#undef _HPUX_SOURCE

/* define if you want to use the GNU source for getopt */
#undef USE_GNU_GETOPT

/* define if gethostname() is not available but <sys/systeminfo.h> is */
#undef GETHOSTNAME_FROM_SYSTEMINFO

/* define if the linker defines the symbols etext, edata, etc. */
#undef ETEXT_LINKED_IN

/* define if your <math.h> declares isnan() */
#define HAVE_ISNAN

/* define if your printf does not work for `long double's */
#undef PRINTF_BROKEN_LG

/* define to enable our own debugging-helper overloads for
   operator new and operator delete in the analyzer */
#undef OVERLOAD_NEW_DELETE

@BOTTOM@

#endif
