/*	File:	  pending.c
 *      Author:	  Lars Ole Andersen (lars@diku.dk)
 *		  Peter Holst Andersen (txix@diku.dk)
 *                Arne John Glenstrup (panic@diku.dk)
 *                Henning Makholm (makholm@diku.dk)
 *                Jens Peter Secher (jpsecher@diku.dk)
 *      Created:  Tue Jul 27 10:36:33 1993
 *      Content:  C-Mix speclib: pending list.
 *
 *      Copyright © 1998-2000 The TOPPS group at DIKU, U of Copenhagen.
 *      Redistribution and modification are allowed under certain
 *      terms; see the file COPYING.cmix for details.
 */
  
#include <stdio.h>
#include <string.h>
#define cmixSPECLIB_SOURCE
#include "speclib.h"

#ifdef PGENLABEL
#error vpending.c is not used anymore
#endif

typedef struct Pend_node {
  struct Pend_node *seen_next;	/* Memoization link     */
  struct Pend_node *pend_next;  /* Pending link         */
  unsigned label;		/* Specialization label */
  cmixLabelTag reslab;		/* Residual label       */
  int memoblock[1]; /* STRUCT HACK */
} Pend_node ;

struct Pend_pointmemo {
  Pend_node *memoized ;
  size_t datasize ;
};

typedef struct Pend_stack {
  struct Pend_stack *next;	/* Next in stack        */
  cmixDataObject *objects;      /* object descriptions  */
  unsigned pgen_points;         /* number of possible pending points */
  cmixLocalMemoWhat *memoset;   /* memoization sets     */
  Pend_node *pending ;          /* stack of pending points             */
  struct Pend_pointmemo memo[1]; /* STRUCT HACK */
} Pend_stack ;

/* Pending stack */
static Pend_stack *cur_pend = NULL ;
static int pendingIsCurrent = 0;

/*======================================================================*
 * Auxiliary routines and macros					*
 *======================================================================*/

#define MEMOSTEP(cp,pdo) \
          cmixDataObject *pdo = cur_pend->objects-1 ; \
          if (!*cp) break ; \
          while( (unsigned char)*cp == 255 ) pdo += 255, cp++ ; \
          pdo += (unsigned char)*cp++ ;

static void
pointmemo_init(unsigned lab)
{
  cmixLocalMemoWhat i = cur_pend->memoset[lab] ;
  size_t total = 0 ;
  for(;;) {
    MEMOSTEP(i,data);
    total += data->size ;
  }
  cur_pend->memo[lab].datasize = total ;
}

/*======================================================================*
 * Pending list routines						*
 *======================================================================*/

/* Insert specialization point into current pending list and return label */
cmixLabelTag
cmixPendinsert(unsigned lab)
{
  Pend_node *p,*q;
  void *fillp ;
  cmixLocalMemoWhat i = cur_pend->memoset[lab] ;

  assert(lab>0 && lab<=cur_pend->pgen_points);
  
  /* First make sure that the summary information about the subject
   * pp has been computed
   */
  if( !cur_pend->memo[lab].memoized )
    pointmemo_init(lab);
  
  /* Create a new complete memoization-copy. If we do not find a
   * fitting residual label we would need to do this anyway; and
   * even if there is a matching label the time used to create
   * the complete block is probably saved if there is a single
   * non-matching block.
   */
  p = HACKNEW(Pend_node,cur_pend->memo[lab].datasize-sizeof(int));
  fillp = &p->memoblock ;
  for(;;) {
    MEMOSTEP(i,data);
    memcpy(fillp,data->obj,data->size);
    fillp = VOIDPTR_PLUS(fillp,data->size);
  }
  
  /* Check to see whether it's already here\
   */
  for( q=cur_pend->memo[lab].memoized; q; q=q->seen_next )
    if( 0 == memcmp(&p->memoblock,&q->memoblock,
                    cur_pend->memo[lab].datasize) ) {
      FREE(p) ;
      return q->reslab ;
    }
  
  /* Not found: insert the new node in the memoization
   * and pending lists */
  p->seen_next = cur_pend->memo[lab].memoized ;
  cur_pend->memo[lab].memoized = p ;
  p->pend_next = cur_pend->pending ;
  cur_pend->pending = p ;
  p->label = lab;
  p->reslab = cmixMakeLabel() ;
  pendingIsCurrent = 1;
  return p->reslab;
}

/* Check pending list and return next specialization point if non-empty */
unsigned
cmixPending(void)
{
  Pend_node *p = cur_pend->pending ;
  
  /* Return 0 if current pending list is empty */
  if( !p )
    return 0;
  
  /* Pop the pending list */
  cur_pend->pending = p->pend_next;

  /* Only waste time on restoring if the node is not recent */
  if( !pendingIsCurrent ) {
    void* restore = &p->memoblock ;
    cmixLocalMemoWhat i = cur_pend->memoset[p->label] ;
    for(;;) {
      MEMOSTEP(i,data);
      memcpy(data->obj,restore,data->size);
      restore = VOIDPTR_PLUS(restore,data->size) ;
    }
  }

  /* Start the specialization */
  cmixLabel(p->reslab);
  pendingIsCurrent = 0 ;
  return p->label;
}

/*======================================================================*
 * Pending stack routines						*
 *======================================================================*/

/* Push current pending and seen lists onto pending stack and initialize new */
void
cmixPushPend(cmixDataObject objects[],
             cmixLocalMemoWhat memosets[], unsigned pgen_points)
{
  unsigned u ;
  Pend_stack *p = HACKNEW(Pend_stack,
                          sizeof(struct Pend_pointmemo)*pgen_points);
  p->next = cur_pend ;
  cur_pend = p ;
  p->objects = objects ;
  p->pgen_points = pgen_points ;
  p->memoset = memosets ;
  p->pending = NULL ;
  for( u=1; u<=pgen_points; u++ )
    p->memo[u].memoized = NULL ;
}

/* Pop pending and seen lists from pending stack and make them current */
void
cmixPopPend(void)
{
  unsigned u ;
  Pend_node *p, *p1;
  Pend_stack *q;

  assert(cur_pend != NULL);
  assert(cur_pend->pending == NULL);

  /* Check memusage before we free memory */
  cmixCheckMemusage();

  /* unhook the old pending list */
  q = cur_pend ;
  cur_pend = q->next ;

  /* delete it */
  for( u=1; u<=q->pgen_points; u++ )
    for( p=q->memo[u].memoized; p; p = p1) {
      p1 = p->seen_next;
      FREE(p);
    }
  FREE(q);
}
