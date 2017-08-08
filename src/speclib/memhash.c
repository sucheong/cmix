/*	File:	  mem.c
 *      Author:	  Peter Holst Andersen
 *                Arne John Glenstrup
 *                Henning Makholm
 *      Content:  C-Mix cogen: sharing memoization copies
 *
 *      Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 *      Redistribution and modification are allowed under certain
 *      terms; see the file COPYING.cmix for details.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <memory.h>
#include <string.h>	/* for memcmp */
#define cmixSPECLIB_SOURCE
#include "speclib.h"

/* Memoisation of objects less than this size do not use the
 * hash table.
 */
#define MIN_SIZE	1024

/* Note that the hash key function assumes a table size of 65536
 */
#define TABLE_SIZE	65536

/* Each table entry contains a bucket list of objects of increasing size,
 * and increasing lexicographical order (as defined by memcmp). The lists
 * are primarily sorted by size.
 */

struct Bucket {
  void *obj;
  size_t size;
  long ref_count;
  struct Bucket *next;
};

static struct Bucket *
newBucket(void *o, size_t s)
{
  struct Bucket *b = NEW(struct Bucket);
  if ( b == NULL )
    cmixFatal("out of memory");
  b->obj = o ;
  b->size = s ;
  b->ref_count = 1 ;
  b->next = NULL ;
  return b ;
}

static struct Bucket *table[TABLE_SIZE] = {0};

#if TABLE_SIZE != 65536
#error The hash function assumes the table size is 65536
#endif
/* XXX this hash functions has absolutely no nice theoretical
 * properties.
 */
static int
hash(void *obj, size_t size)
{
  unsigned short key = size + (size >> 16);
  unsigned short *a;
  long x = (long)obj;
  if (x % 2 != 0)
    a = (unsigned short *) (x+1);
  else
    a = (unsigned short *) x;
  for( ; (long)a < (long)obj - 1; a++)
    key ^= *a;
  return key % TABLE_SIZE;
}

static void *
lookup(void *obj, size_t size)
{
  int x;
  int n = hash(obj, size);
  
  struct Bucket *b = table[n];
  struct Bucket **prev = &(table[n]);
  cmixCollectStatistic(STAT_LOOKUP_SHARED,0);
  for( ; b; prev = &(b->next), b = b->next) {
    cmixCollectStatistic(STAT_LOOKUP_SHARED2,0);
    if (size == b->size) {
      x = memcmp(obj, b->obj, size);
      if (x < 0)
        continue;
      if (x == 0) {
        cmixCollectStatistic(STAT_SHARED_FOUND, size);
        b->ref_count++;		/* Already there */
        return b->obj;
      }
      break;
    }
    if (size < b->size)
      continue;
    /* Size is greater than b->size, so the object is not in the list */
    break;
  }
  /* Insert new bucket between prev and b */
  {
    void *to = cmixMalloc(size);
    memcpy(to, obj, size);
    *prev = newBucket(to, size);
    (*prev)->next = b;
    return to;
  }
}
    
static int
remove_hashed(void *obj, size_t size)
{
  int x;
  int n = hash(obj, size);
  struct Bucket *b = table[n];
  struct Bucket **prev = &(table[n]);
  for( ; b; prev = &(b->next), b = b->next) {
    if (size == b->size) {
      x = memcmp(obj, b->obj, size);
      if (x < 0)
        continue;
      if (x == 0) {
        b->ref_count--;
        /* No more references: remove it from the list, and free the
         * storage
         */
        if (b->ref_count == 0) {
          free(b->obj);
          *prev = b->next;
        }
        return b->ref_count;
      }
      break;
    }
    if (size < b->size)
      continue;
    break;
  }
  cmixFatal("SimpleHash::remove: object not found\n");
  return -1;
}

void
cmixFreeMemoCopy(void *copy,size_t size)
{
  if (size >= MIN_SIZE)
    remove_hashed(copy, size);
  else
    free(copy);
}

void *
cmixCreateMemoCopy(void *from, size_t size)
{
  if (size >= MIN_SIZE)
    return lookup(from, size);
  else {
    void *to = cmixMalloc(size);
    cmixCollectStatistic(STAT_ALLOCATED, size);
    memcpy(to, from, size);
    return to;
  }
}
