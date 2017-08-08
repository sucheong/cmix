/*	File:	  mem.c
 *      Author:	  Peter Holst Andersen
 *                Arne John Glenstrup
 *                Henning Makholm
 *      Content:  C-Mix speclib: memoization
 *
 *      Copyright © 1999-2000. The TOPPS group at DIKU, U of Copenhagen.
 *      Redistribution and modification are allowed under certain
 *      terms; see the file COPYING.cmix for details.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <memory.h>
#include <string.h>	/* for memcmp */
#define cmixSPECLIB_SOURCE
#include "speclib.h"
#include "code.h"  /* for struct cmixFun */

struct cmixDataObjectCopy {
  void *copy;				/* Copied data                */
  size_t size;		        	/* Size of the copied object  */
  struct cmixDataObject *obj;		/* The original object desc.  */
  long offset ;		        	/* offset always memoized     */
  struct cmixDataObjectCopy *next;	/* Next element in the list   */
};

typedef struct HeapObject {
  cmixDataObject obj ;
  unsigned length ;
  struct HeapObject *next ;
} HeapObject ;

#define DEBUG_MEM(args)
/* #define DEBUG_MEM(args)	printf args, fflush(stdout) */
#define DEBUG_HEAP(args)
/* #define DEBUG_HEAP(args)	printf args, fflush(stdout) */

/* dump_copied_state is NOT static; that prevents warnings that it is unused */
void dump_copied_state(cmixDataObjectCopy *copy);

/*==========================================================================*
 * Handling of data objects						    *
 *==========================================================================*/

static HeapObject *heap_objects = NULL;

static cmixDataObject **data_stack = NULL;
static int data_sp = -1;
static int data_stack_size = 0;

static cmixDataObject *globals ;

static void *globals_low = NULL;
static void *globals_high = NULL;
static void *stack_low = NULL;
static void *stack_high = NULL;
static void *heap_low = NULL;
static void *heap_high = NULL;

void
cmixPushGlobals(cmixDataObject the_globals[])
{
  unsigned i ;
  globals = the_globals ;

  for ( i = 0; the_globals[i].obj; i++ ) {
    if (!globals_low || the_globals[i].obj < globals_low)
	globals_low = the_globals[i].obj;
    if (!globals_high ||
        VOIDPTR_PLUS(the_globals[i].obj,the_globals[i].size) > globals_high)
	globals_high = VOIDPTR_PLUS(the_globals[i].obj,the_globals[i].size) ;
  }
}

void
cmixPushLocals(cmixDataObject locals[])
{
  unsigned i ;
  for(i=0; locals[i].obj; i++) {
    DEBUG_MEM(("cmixPushLocals: %p\n", (void*)&locals[i]));
    data_sp++;
    if (data_sp >= data_stack_size) {
      data_stack_size += 1000;
      if (!data_stack)
        data_stack = (cmixDataObject **)cmixMalloc(data_stack_size *
                                               sizeof(cmixDataObject *));
      else
        data_stack = (cmixDataObject **)realloc(data_stack,
                                                data_stack_size *
                                                sizeof(cmixDataObject *));
      if (!data_stack)
        cmixFatal("out of memory");
    }
    data_stack[data_sp] = &locals[i];
    
    if (!stack_low || locals[i].obj < stack_low)
      stack_low = locals[i].obj;
    if (!stack_high || VOIDPTR_PLUS(locals[i].obj,locals[i].size) > stack_high)
      stack_high = VOIDPTR_PLUS(locals[i].obj,locals[i].size) ;
  }
}

void cmixPopLocals(int n)
{
  DEBUG_MEM(("cmixPopLocals: %d\n", n));
  if (data_sp - n < -1)
    cmixFatal("cmixPopLocals: stack underflow\n");
  data_sp -= n ;
}

/*==========================================================================*
 * Helper functions for the memoisation administration                      *
 *==========================================================================*/

static cmixMemoWhat global_what ;

static void
clear_visit_flags()
{
  int i ;
  HeapObject *hop ;
  DEBUG_MEM(("clear_visit_flags\n"));
  for(i=0; globals[i].obj; i++)
    globals[i].visit = 0 ;
        
  for(i = 0; i <= data_sp; i++)
    data_stack[i]->visit = 0;
  
  for(hop = heap_objects; hop; hop = hop->next)
    hop->obj.visit = 0;
}

static int
setinwhat(cmixDataObject *d)
{
  unsigned id = d->ID ;
  return ( (unsigned char)(global_what[id/8]) & (1 << (id&7)) ) != 0 ;
}

static int
maycopy(cmixDataObject *d)
{
  if ( d->visit ) {
    DEBUG_MEM(("already visited - ID %ld\n",d->ID));
    return 0 ;
  }
  d->visit = 1 ;
  if ( !setinwhat(d) ) {
    DEBUG_MEM(("not in use - ID %ld\n",d->ID));
    return 0 ;
  }
  return 1 ;
}

static cmixDataObjectCopy *
append(cmixDataObjectCopy *xs, cmixDataObjectCopy *ys)
{
  cmixDataObjectCopy *tmp ;

  if (!xs)
    return ys;
  if (!ys)
    return xs;
  
  tmp = xs->next;
  xs->next = ys->next;
  ys->next = tmp;
  return ys;
}

int
cmix_follow0(cmixPointerCallback dummy1,cmixDataObjectCopy **dummy2,
             void *dummy3, unsigned dummy4)
{
  return 1 ;
}

/*==========================================================================*
 * Copying states							    *
 *==========================================================================*/

static cmixDataObjectCopy *copy_one(cmixDataObject *, unsigned, void const *);

/* this always returns 1, so the generic following functions will not abort */
static int
copy_referenced_object(cmixDataObjectCopy **copy, void const* ref)
{
  int i ;
  DEBUG_MEM(("copy referenced object: %p\n", (void*)ref));
  
  if (ref == NULL) {
    cmixCollectStatistic(STAT_COPY_NULL,0);
    return 1;
  }
  
  if (heap_low <= ref && ref <= heap_high) {
    HeapObject *foo ;
    cmixCollectStatistic(STAT_COPY_CHECK_HEAP,0);
    for(foo = heap_objects; foo; foo = foo->next ) {
      if ( foo->obj.obj <= ref && 
           ref < VOIDPTR_PLUS(foo->obj.obj,foo->obj.size) ) {
        cmixCollectStatistic(STAT_COPY_HEAP,0);
        *copy = append(*copy,copy_one(&foo->obj,foo->length,ref) );
        return 1 ;
      }
    }
  }
  
  if (globals_low <= ref && ref <= globals_high) {
    cmixCollectStatistic(STAT_COPY_CHECK_GLOBALS,0);
    for(i = 0; globals[i].obj; i++) {
      if (globals[i].obj <= ref &&
          ref < VOIDPTR_PLUS(globals[i].obj,globals[i].size) ) {
        cmixCollectStatistic(STAT_COPY_GLOBALS,0);
        *copy = append(*copy,copy_one(&globals[i],1,ref));
        return 1 ;
      }
    }
  }
  
  if (stack_low <= ref && ref <= stack_high) {
    cmixCollectStatistic(STAT_COPY_CHECK_STACK,0);
    for(i = 0; i <= data_sp; i++) {
      if (data_stack[i]->obj <= ref &&
          ref < VOIDPTR_PLUS(data_stack[i]->obj,data_stack[i]->size) ) {
        cmixCollectStatistic(STAT_COPY_STACK,0);
        *copy = append(*copy,copy_one(data_stack[i],1,ref));
        return 1 ;
      }
    }
  }
  
  DEBUG_MEM(("copy_ref: pointer not found: %p\n", ref));
  cmixCollectStatistic(STAT_COPY_NOT_FOUND,0);
  return 1 ;
}

static cmixDataObjectCopy *
do_copy_object(cmixDataObject *object,void const*ref)
{
  cmixDataObjectCopy *copy = NEW(cmixDataObjectCopy) ;
  if ( copy == NULL )
    cmixFatal("out of memory");
  copy->obj = object ;
  copy->size = object->size ;
  copy->copy = cmixCreateMemoCopy(object->obj, object->size);
  copy->offset = VOIDPTR_MINUS(ref,object->obj);
  copy->next = copy;
  DEBUG_MEM(("basic copy: %p (loc: %p) -> %p (loc: %p), size: %lu\n",
             (void*)object, (void*)object->obj, (void*)copy, (void*)copy->copy,
             (unsigned long)object->size));
  return copy ;
}

static cmixDataObjectCopy *
copy_one(cmixDataObject *d,unsigned length,void const*ref)
{
  if (!maycopy(d))
    return NULL ;
  else {
    cmixDataObjectCopy *copy = do_copy_object(d,ref);
    d->follow_pointers(copy_referenced_object,&copy,d->obj,length);
    return copy;
  }
}

static cmixSavedState
copy_objects(cmixDataObject objs[])
{
  unsigned i ;
  cmixDataObjectCopy *copies = NULL;

  DEBUG_MEM(("copy_objects: %p\n", (void*)objs));
  if (!objs)
    return NULL;
  
  for(i = 0; objs[i].obj; i++) {
    DEBUG_MEM(("copy_objects: objs[%d] = %lu\n",
               i, objs[i].ID));
    copies = append(copies, copy_one(&objs[i],1,objs[i].obj));
  }
  return copies;
}

static cmixSavedState
cmixCopyState(cmixDataObject locals[])
{
  cmixDataObjectCopy *temp ;

  clear_visit_flags();
  temp = copy_objects(globals);
  return append(temp,copy_objects(locals));
}

/* Make a state copy containing the new values of those objects
 * memoised as the initial state for the function that have changed.
 * Local objects in the function are not copied; it is irrelevant
 * to the caller whether they change (and it would be a disaster to
 * try to "restore" them later, anyway);
 * For performance reasons, objects that have not changed are not
 * copied, either.
 *   Previously, this function was called cmixCopyStateMinusLocals,
 *   and simply did normal breadth-first copy starting from the end
 *   configuration. This is wrong, because a function might change
 *   an object and then forget the pointer to the changed object, though
 *   it may still be visible to the caller.
 */
static cmixSavedState
cmixExtractEndState(cmixDataObjectCopy *beginstate, cmixDataObject locals[])
{
  cmixDataObjectCopy *restore = NULL ;
  cmixDataObjectCopy *copy = beginstate ;
  unsigned i ;
  
  if ( beginstate == NULL )
    return NULL ;
  
  /* use the visit flags to mark those objects that should not be
   * copied because they are locals ;
   */
  clear_visit_flags();
  for(i = 0; locals[i].obj; i++)
    locals[i].visit = 1;
  
  do {
    /* local objects are irrelevant; optimize away unchanged objects */
    if ( !copy->obj->visit &&
         setinwhat(copy->obj) &&
         memcmp(copy->obj->obj, copy->copy, copy->size) != 0 ) {
      /* OK, make a copy of this object */
      cmixDataObjectCopy *newcopy = NEW(cmixDataObjectCopy) ;
      *newcopy = *copy ;
      newcopy->copy = cmixCreateMemoCopy(copy->obj->obj, copy->size);
      newcopy->next = newcopy ;
      restore = append(restore,newcopy);
    }
    copy = copy->next ;
  } while ( copy != beginstate );
  return restore;
}

/*==========================================================================*
 * Restore a saved state						    *
 *==========================================================================*/

static void
cmixRestoreState(cmixSavedState copy)
{
  cmixDataObjectCopy *c = copy ;

  DEBUG_MEM(("cmixRestoreState: %p\n", (void*)copy));
  if (copy!=NULL)
    do {
      DEBUG_MEM(("cmixRestoreState: %p: %p <- %p (%lu bytes)\n",
                 (void*)c, (void*)c->obj, (void*)c->copy,
                 (unsigned long)c->size));
      memcpy(c->obj->obj, c->copy, c->size);
      c = c->next;
    } while (c != copy);
}

#if 0

/*==========================================================================*
 * Delete a saved state							    *
 *==========================================================================*/

static void
cmixFreeState(cmixSavedState c)
{
  cmixDataObjectCopy *ob, *next;
  if (c==NULL)
    return;
  ob = c->next;
  c->next = 0;		/* Break the list */
  for( ; ob; ob = next) {
    next = ob->next;
    if (ob->obj)
      cmixFreeMemoCopy(ob->copy, ob->size);
    FREE(ob);
  }
}

#endif

/*==========================================================================*
 * Compare saved state to current state					    *
 *==========================================================================*/

static int compare_one(cmixDataObject *,unsigned,
                       cmixDataObjectCopy **,void const *);
static int
compare_referenced_object(cmixDataObjectCopy **copy, void const* ref)
{
  int i ;
  DEBUG_MEM(("compare refecenced object: %p, copy: %p (%p)\n",
             ref, (void*)(*copy), (void*)(*copy)->copy));
  
  if (ref == NULL) {
    DEBUG_MEM(("null, succeed\n"));
    cmixCollectStatistic(STAT_CMP_NULL,0);
    return 1;
  }
  
  if (heap_low <= ref && ref <= heap_high) {
    HeapObject *foo ;
    cmixCollectStatistic(STAT_CMP_CHECK_HEAP,0);
    for(foo = heap_objects; foo; foo = foo->next) {
      if (foo->obj.obj <= ref &&
          ref < VOIDPTR_PLUS(foo->obj.obj,foo->obj.size) ) {
        cmixCollectStatistic(STAT_CMP_HEAP,0);
        return compare_one(&foo->obj,foo->length,copy,ref);
      }
    }
  }

  if (globals_low <= ref && ref <= globals_high) {
    cmixCollectStatistic(STAT_CMP_CHECK_GLOBALS,0);
    for(i = 0; globals[i].obj; i++) {
      if (globals[i].obj <= ref &&
          ref < VOIDPTR_PLUS(globals[i].obj,globals[i].size) ) {
        cmixCollectStatistic(STAT_CMP_GLOBALS,0);
        return compare_one(&globals[i],1,copy,ref);
      }
    }
  }

  if (stack_low <= ref && ref <= stack_high) {
    cmixCollectStatistic(STAT_CMP_CHECK_STACK,0);
    for(i = 0; i <= data_sp; i++) {
      if (data_stack[i]->obj <= ref &&
          ref < VOIDPTR_PLUS(data_stack[i]->obj,data_stack[i]->size) ) {
        cmixCollectStatistic(STAT_CMP_STACK,0);
        return compare_one(data_stack[i],1,copy,ref);
      }
    }
  }

  DEBUG_MEM(("compare_ref: pointer not found: %p\n", ref));
  cmixCollectStatistic(STAT_CMP_NOT_FOUND,0);
  /* If the pointer is not found it is probably because its
   * cmixDataObject has been optimized away because it is never
   * relevant to memoize it or follow its pointers.
   */
  return 1;
}

static int
compare_one(cmixDataObject *d,unsigned length,
            cmixDataObjectCopy **copy,void const *ref)
{
  if (!maycopy(d)) return 1 ;

  DEBUG_MEM(("base compare: copy: %p (%p, size: %lu) ID=%lu\n",
             (void*)*copy, (void*)(*copy)->copy,
             (unsigned long)(*copy)->size, d->ID));
  
  if ( (*copy)->offset != VOIDPTR_MINUS(ref,d->obj) ) {
    DEBUG_MEM(("base compare failed: wrong offset\n"));
    return 0 ;
  }
  
  if ( (*copy)->size != d->size ) {
    DEBUG_MEM(("base compare failed: bad size %ld!=%ld\n",
               (long)d->size, (long)(*copy)->size));
    return 0 ;
  }
  
  if ( memcmp(d->obj, (*copy)->copy, d->size) != 0 ) {
    DEBUG_MEM(("base compare failed (bad data)\n"));
    return 0 ;
  }

  /* move copy pointer to the next object in the list */
  (*copy) = (*copy)->next;
  
  return d->follow_pointers(compare_referenced_object,copy,d->obj,length);
}

static int
cmixCmpState(cmixDataObject locals[], cmixSavedState copy)
{
  cmixDataObjectCopy *cpy ;
  unsigned i ;
  
  DEBUG_MEM(("cmixCmpState: copy: %p\n",(void*)copy));
  /* The empty state always matches */
  if (copy==NULL)
    return 1;
  cmixCollectStatistic(STAT_CMP_STATE,0);
  clear_visit_flags();
  cpy = copy->next;
  for(i = 0; globals[i].obj; i++)
    if (!compare_one(&globals[i],1,&cpy,globals[i].obj)) {
      DEBUG_MEM(("cmixCmpState failed at globals[%d] = ID %lu\n",
                 i,globals[i].ID));
      goto mismatch;
    }
  for(i = 0; locals[i].obj; i++)
    if (!compare_one(&locals[i],1,&cpy,locals[i].obj)) {
      DEBUG_MEM(("cmixCmpState failed at locals[%d] = ID %lu\n",
                 i,locals[i].ID));
      goto mismatch;
    }
  cmixCollectStatistic(STAT_CMP_STATE_MATCH,0);
  return 1;
  
 mismatch:
#if 0
  if (debug_mem && !strcmp(cur_fun->decl(), "run")) {
    printf("cmixCmpState: mismatch. state: %p, copy: %p\n",
           (void*)state, (void*)copy);
    dump_copied_state(copy);
  }
#endif
  return 0;
}

/*==========================================================================*
 * Interface for function memoization					    *
 *==========================================================================*/

static cmixSavedState last_nonfound = NULL ;
struct cmixFunPool *last_pool = NULL ;

/* Check whether a function has been specialized to this state.
 * If found then return a resiudal expression containing the name of the
 * residual function. Otherwise prepare for creating the new function
 */
Code
cmixFindOrPushFun(struct cmixFunPool *pool,
                  cmixDataObject params[],cmixMemoWhat what)
{
  struct cmixFun *f ;

  global_what = what ;
  assert(last_pool == NULL);
    
  /* Go through residual functions and check... */
  for ( f = pool->list ; f ; f = f->next_share ) {
    if ( !cmixCmpState(params, f->state) )
      continue ;
    if ( f->end_state )
      cmixRestoreState(f->end_state);
    f->shared = 1 ;
    return f->resname;
  }

  last_nonfound = cmixCopyState(params);
  last_pool = pool ;
  return NULL ;
}

void
cmixPushFun(void)
{
  assert(last_pool == NULL);
  /* last_pool = NULL */
  last_nonfound = NULL ;
}

void
cmixFinishPushedFun(struct cmixFun *f)
{
  if( last_pool ) {
    f->next_share = last_pool->list ;
    last_pool->list = f ;
  }
  f->state = last_nonfound ;
  f->end_state = 0 ;
  f->shared = 0 ;
  last_pool = NULL ;
}

void
cmixSaveExitState(cmixDataObject locals[],cmixMemoWhat what)
{
  global_what = what ;
  cmixCurFun->end_state = cmixExtractEndState(cmixCurFun->state,locals);
}

/*==========================================================================*
 * Heap allocation helper functions					    *
 *==========================================================================*/

#if 0

void cmix_free(void *ref)
{
  if (ref == NULL)
    return;
  List<class cmixDataObject *> *ho, **prev;
  prev = &heap_objects;
  ho = heap_objects;
  while (ho) {
    if (ho->elem()->obj == ref) {
      /* possibly delete the object, but not until we have a working
       * reference count scheme in place.
       */
      return;
    }
    prev = &(ho->link);
    ho = ho->next();
  }
  fprintf(stderr, "cmix_free: Object not found: %p\n", ref);
}

#endif

void *
cmixAllocStatic (unsigned long ID, size_t size, unsigned nelem,
                 cmixPointerFollower f)
{
  void *data = calloc(nelem+1,size);
  HeapObject *hop = NEW(HeapObject) ;

  if ( data == NULL )
    return NULL ;
  
  size *= nelem ;
  size++ ;
  
  hop->next = heap_objects ;
  heap_objects = hop ;
  hop->length = nelem ;
  hop->obj.ID = ID ;
  hop->obj.obj = data ;
  hop->obj.size = size ;
  hop->obj.follow_pointers = f ;
  hop->obj.visit = 0 ;
  
  if (!heap_low || data < heap_low)
    heap_low = data ;
  if (!heap_high || VOIDPTR_PLUS(data,size) > heap_high)
    heap_high = VOIDPTR_PLUS(data,size) ;
  
  return data ;
}

/*==========================================================================*
 * Debugging functions							    *
 *==========================================================================*/

void
dump_copied_state(cmixSavedState copy)
{
  printf("dump_copied_state: copy: %p\n", (void*)copy);
  if (copy==NULL) {
    printf("  empty");
  } else {
    cmixDataObjectCopy *c = copy->next;
    do {
      unsigned i ;
      printf("  %p %p %6u - ",
             (void*)c->copy, (void*)c->obj, (unsigned)c->size);
      for (i = 0; i <= 4 && i*sizeof(unsigned) < c->size; i++)
        printf("%x ", ((unsigned *)c->copy)[i] );
      printf("\n");
      c = c->next;
    } while (c != copy->next);
  }
  printf("\n");
}

