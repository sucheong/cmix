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

#ifndef __CMIX_TIME__
#define __CMIX_TIME__

/* ISO C 7.12 Date and time */
#pragma cmix header: <time.h>

/* ISO C 7.12.1 Components of time */
#ifndef NULL
# pragma cmix taboo: NULL
# define NULL ((void*)0)
#endif

extern const int CLOCKS_PER_SEC;
#pragma cmix well-known constant: CLOCKS_PER_SEC
typedef __CMIX(iu) size_t;
typedef __CMIX(a) clock_t;
typedef __CMIX(a) time_t;

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
};
#pragma cmix well-known: tm

/* ISO C 7.12.2 Time manipulation functions */
clock_t clock(void);
double difftime(time_t, time_t);
time_t mktime(struct tm*);
time_t time(time_t);
#pragma cmix well-known: clock difftime mktime time
#pragma cmix pure: difftime() mktime()

/* ISO C 7.12.3 Time conversion functions */
char *asctime(const struct tm*);
char *ctime(const time_t*);
struct tm *gmtime(const time_t*);
struct tm *localtime(const time_t*);
size_t strftime(char*, size_t, const char*, const struct tm*);
#pragma cmix well-known: asctime ctime gmtime localtime strftime

#endif
