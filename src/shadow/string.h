/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix shadow header: string.h
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 */

#ifndef __CMIX
#  error This file is only intended to be included from C-Mix/II
#endif

#ifndef __CMIX_STRING__
#define __CMIX_STRING__

#pragma cmix header: <string.h>

/* ISO C 7.11.1 String function conventions */
typedef __CMIX(iu) size_t ;

#ifndef NULL
# pragma cmix taboo: NULL
# define NULL ((void*)0)
#endif

/* ISO C 7.11.2 Copying functions */
void* memcpy  (void *, const void *, size_t);
void* memmove (void *, const void *, size_t);
char* strcpy  (char *, const char *);
char* strncpy (char *, const char *, size_t);
#pragma cmix well-known: memcpy memmove strcpy strncpy
#pragma cmix stateless: memcpy() memmove() strcpy() strncpy()
/* Something like the line below could be used to indicate that the four
   functions all return something that can be reached from argument one: */
/* pragma cmix returns(1): memcpy() memmove() strcpy() strncpy() */

/* ISO C 7.11.3 Concatenation functions */
char* strcat  (char *, const char *);
char* strncat (char *, const char *, size_t);
#pragma cmix well-known: strcat strncat
#pragma cmix stateless: strcat() strncat()
/* pragma cmix returns(1): strcat() strncat() */

/* ISO C 7.11.4 Comparison functions */
int    memcmp  (const void *, const void *, size_t);
int    strcmp  (const char *, const char *);
int    strcoll (const char *, const char *);
int    strncmp (const char *, const char *, size_t);
size_t strxfrm(char *, const char *, size_t);
#pragma cmix well-known: memcmp strcmp strcoll strncmp strxfrm
#pragma cmix pure: memcmp() strcmp() strcoll() strncmp()
#pragma cmix stateless: strxfrm()

/* ISO C 7.11.5 Search functions */
void*  memchr(const void*, int c, size_t);
char*  strchr  (const char *, int);
size_t strcspn(const char*, const char*);
char*  strpbrk (const char *, const char *);
char*  strrchr (const char *, int);
size_t strspn(const char*, const char*);
char*  strstr  (const char *, const char *);
char*  strtok  (char *, const char *);
#pragma cmix well-known: memchr strchr strcspn strpbrk strrchr
#pragma cmix well-known: strspn strstr strtok
#pragma cmix pure: memchr() strchr() strcspn() strpbrk()
#pragma cmix pure: strrchr() strspn() strstr()
/* pragma cmix returns(1): memchr() strchr() strpbrk() strrchr() strstr() */

/* ISO C 7.11.6 Miscellaneous functions */
void * memset(void*, int, size_t);
char*  strerror(int);
size_t strlen(const char*);
#pragma cmix well-known: memset strerror strlen
#pragma cmix stateless: memset()
#pragma cmix pure: strlen() strerror()
/* pragma cmix returns(1): memset() */

#endif

