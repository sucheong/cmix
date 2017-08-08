/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: latch for finished HTML data
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __latch__
#define __latch__

char* stringDup(const char* str);      /* Duplicate a string (dynamically). */
void* safe_malloc(size_t s);
#define alloc(t) ((t*)(safe_malloc(sizeof(t))))
void die(const char* format,...);

struct latch ;

void latchf(struct latch *,const char *fmt,...);
void latchs(struct latch *,const char*);
void latchc(struct latch *,char);
struct latch *newlatch() ;
#ifdef SEEK_SET /* stdio.h test */
  void outputlatch(struct latch *,FILE*);
#endif
void deletelatch(struct latch *);

#if !defined(NULL)
#define NULL ((void*)0)
#endif

#endif /* __annofront__ */

