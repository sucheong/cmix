/* $Id: signal.h,v 1.1.1.1 1999/02/22 13:50:38 makholm Exp $
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

#ifndef __CMIX_SIGNAL__
#define __CMIX_SIGNAL__

/* ISO C 7.7 Signal handling */
#pragma cmix header: <signal.h>

typedef __CMIX(i) sig_atomic_t;

extern const void (*SIG_DFL)(int);
extern const void (*SIG_ERR)(int);
extern const void (*SIG_IGN)(int);
#pragma cmix well-known: SIG_DFL SIG_ERR SIG_IGN

extern const int SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM;
#pragma cmix well-known constant: SIGABRT SIGFPE SIGILL SIGINT SIGSEGV SIGTERM

/* ISO C 7.7.1 Specify signal handling */
void (*signal(int, void (*)(int)))(int);
#pragma cmix well-known: signal

/* ISO C 7.7.2 Send signal */
int raise(int);
#pragma cmix well-known: raise

#endif
