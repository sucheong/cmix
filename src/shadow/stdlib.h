/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix shadow header: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 */

#ifndef __CMIX
#  error This file is only intended to be included from C-Mix/II
#endif

#ifndef __CMIX_STDLIB__
#define __CMIX_STDLIB__

#pragma cmix header: <stdlib.h>

/* ISO C 7.10 General Utilities */
typedef __CMIX(iu) size_t ;
typedef __CMIX(i) wchar_t;
typedef struct div_t { int quot; int rem; } div_t;
typedef struct ldiv_t { long int quot; long int rem; } ldiv_t;

#pragma cmix well-known: div_t ldiv_t

#ifndef NULL
#  pragma cmix taboo: NULL
#  define NULL ((void*)0)
#endif

extern const int EXIT_FAILURE, EXIT_SUCCESS;
extern const int RAND_MAX;
extern const int MB_CUR_MAX;
#pragma cmix well-known constant: EXIT_FAILURE EXIT_SUCCESS MB_CUR_MAX
#pragma cmix well-known constant: RAND_MAX

/* ISO C 7.10.1 String conversion functions */
double   atof(const char *);
int      atoi(const char *);
long int atol(const char *);
double   strtod(const char *, char **);
long int strtol(const char *, char **, int);
unsigned
long int strtoul(const char *, char **, int);
#pragma cmix well-known: atof atoi atol strtod strtol strtoul
#pragma cmix pure: atof() atoi() atol()
#pragma cmix stateless: strtod() strtol() strtoul()

/* ISO C 7.10.2 Pseudo-random sequence generation functions */
int rand(void);
void srand(unsigned int);
#pragma cmix well-known: rand srand

/* ISO C 7.10.3 Memory management functions */
/* These are handled specially by C-Mix, but only after the type check,
 * so we need to provide prototypes for them (which also conveniently
 * taboos their names).
 * Actually, realloc() is not handled specially; but that means that
 * calls, *and the pointer to the heap allocated data; hence the
 * allocation itself* will be residualized and all is fine. A user
 * who explicitly annotates a realloc call as spectime is asking
 * for trouble.
 */
void* calloc(size_t, size_t);
void free(void*);
void* malloc(size_t);
void* realloc(void*, size_t);
#pragma cmix well-known: calloc free malloc realloc

/* ISO C 7.10.4 Communication with the environment */
void abort(void);
void atexit(void (*)(void));
void exit(int);
char* getenv(const char*);
int system(const char*);
#pragma cmix well-known: abort atexit exit getenv system
#pragma cmix rwstate: abort() exit()
/* rwstate because the user should specify explicitly when to call them */
#pragma cmix stateless: getenv()

/* ISO C 7.10.5 Searching and sorting utilities */
void* bsearch(const void*, const void*, size_t, size_t,
	      int (*)(const void*, const void*));
void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
#pragma cmix well-known: bsearch qsort
#pragma cmix pure: bsearch(); stateless: qsort()

/* ISO C 7.10.6 Integer arithmetic functions */
int abs(int j);
div_t div(int numer, int denom);
long int labs(long int j);
ldiv_t ldiv(long int numer, long int denom);
#pragma cmix well-known: abs div labs ldiv
#pragma cmix pure: abs() div() labs() ldiv()

/* ISO C 7.10.7 Multibyte character functions */
int mblen(const char*, size_t);
int mbtowc(wchar_t*, const char*, size_t);
int wctomb(char*, wchar_t);
#pragma cmix well-known: mblen mbtowc wctomb
#pragma cmix pure: mblen()
#pragma cmix stateless: mbtowc() wctomb()

/* ISO C 7.10.8 Multibyte string functions */
size_t mbstowcs(wchar_t*, const char*, size_t);
size_t wcstombs(char*, const wchar_t*, size_t);
#pragma cmix well-known: mbstowcs wcstombs
#pragma cmix pure: mbstowcs() wcstombs()

#endif
