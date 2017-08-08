/*	  File:    restruct.c
 *	  Author:  Sebastian Skalberg <skalberg@diku.dk>
 *	  Content: C-Mix restructuring: functions.
 *
 *	  Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 *	  Redistribution and modification are allowed under certain
 *	  terms; see the file COPYING.cmix for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define cmixSPECLIB_SOURCE
#include "speclib.h"
#include "code.h"

static unsigned makeInterval( struct cmixStmtLabel * );
static void addReference( Interval *, Interval * );

static iList
*makeIList( Interval *interval, iList *next )
{
  iList *rv;
  
  rv = (iList *)malloc(sizeof(iList));
  rv->interval = interval;
  rv->next = next;
  
  return rv;
}

static void
addReference( Interval *from, Interval *to )
{
  iList *newRef;
  
  assert( from != NULL );
  assert( to != NULL );
  
  /* Add forward reference from->to */
  newRef = makeIList( to, from->forw_refs );
  from->forw_refs = newRef;
  
  /* Add backward reference to->from */
  newRef = makeIList( from, to->back_refs );
  to->back_refs = newRef;
}

static void
swapBranches( Interval *interval )
{
  struct cmixStmtIf *cond;
  struct cmixStmtLabel *swapLabel;
  iList *swapIList;
  
  /* Only swap branches on conditionals */
  assert( interval->type == basic );
  assert( interval->member.basic.control->common.tag == If );
  
  cond = &interval->member.basic.control->cond;
  cond->cond = &cmixMkExp( "! (?)", cond->cond )->inner;
  
  swapLabel = cond->then_target;
  cond->then_target = cond->else_target;
  cond->else_target = swapLabel;
  
  /* Using the fact that basic intervals with conditionals have precisely
     two forward references. */
  swapIList = interval->forw_refs;
  interval->forw_refs = interval->forw_refs->next;
  interval->forw_refs->next = swapIList;
  swapIList->next = NULL;
}

static unsigned
makeInterval( struct cmixStmtLabel *label )
{
  unsigned noOfIntervals = 0;
  union cmixStmt *stmt, **pstmt;

  /* If this block has already been assigned an interval, just report that
     no new intervals where created with this invocation of makeInterval() */

  if ( label->interval != NULL )
    return 0;

  pstmt = NULL;
  stmt = (union cmixStmt *)label;

  while ( stmt->common.next != NULL &&
          stmt->common.next->common.tag != Label ) {
    pstmt = &stmt->common.next;
    stmt = stmt->common.next;
  }

  /* At this point, stmt is the control statement, ie., neither plain nor
     a label, following the label.  pstmt is a pointer to this statement,
     so we can change the control statement if needed. */

  assert( pstmt != NULL );
  assert( *pstmt == stmt );

  /* Initialize new interval */

  label->interval = (Interval *)malloc(sizeof(Interval));
  label->interval->parent = NULL;
  label->interval->number = -1;
  label->interval->type = basic;
  label->interval->member.basic.label = label;
  label->interval->member.basic.control = stmt;
  label->interval->member.basic.immDom = NULL;
  label->interval->member.basic.ifFollow = NULL;
  label->interval->member.basic.loop = NULL;
  label->interval->member.basic.printed = 0;
  label->interval->back_refs = NULL;
  label->interval->forw_refs = NULL;
  label->interval->loopCheckDone = 0;
  label->interval->path = 0;

  /* We created a new interval */

  noOfIntervals = 1;

  /* process control statement */

  switch ( stmt->common.tag ) {
  case If:
    /* No degenerate conditionals */
    assert ( stmt->cond.then_target != stmt->cond.else_target );
    /* No plain prefixes */
    //    assert ( label->next == stmt );

    noOfIntervals += makeInterval( stmt->cond.then_target );
    noOfIntervals += makeInterval( stmt->cond.else_target );
    
    addReference( label->interval, stmt->cond.then_target->interval );
    addReference( label->interval, stmt->cond.else_target->interval );
    break;

  case Goto:
    noOfIntervals += makeInterval( stmt->jump.target );
    addReference( label->interval, stmt->jump.target->interval );
    break;
  case Abort:
  case Return:
    break;
  default:
    cmixFatal("Unknown statement tag in makeInterval");
    break;
  }

  return noOfIntervals;
}

static unsigned
enumIntervals( Interval *interval, unsigned lastFree, Interval *order[] )
{
  iList *ref;

  if ( interval->number != -1 )
    return lastFree;

  interval->number = 0;

  for ( ref = interval->forw_refs; ref != NULL; ref = ref->next )
    lastFree = enumIntervals( ref->interval, lastFree, order );

  interval->number = lastFree;
  if ( interval->type == basic )
    interval->member.basic.label->number = lastFree;

  order[lastFree--] = interval;

  return lastFree;
}

static unsigned
iterateIntervals( Interval *interval )
{
  iList *ref, *ref2;
  Interval *parent;
  int changed;
  unsigned noOfIntervals = 0;

  /* This interval has already been assigned a new interval. */
  if ( interval->parent != NULL )
    return 0;

  interval->parent = (Interval *)malloc(sizeof(Interval));
  parent = interval->parent;

  parent->parent = NULL;
  parent->number = interval->number;
  parent->type = ilist;
  parent->member.ilist = makeIList( interval, NULL );
  parent->back_refs = NULL;
  parent->forw_refs = NULL;
  parent->loopCheckDone = 0;

  /* So we have created a new interval. */
  noOfIntervals = 1;
  
  /* Keep running the loop until a fixpoint is reached, ie., until no
     new intervals are added to the new (parent) interval. */
  do {
    changed = 0;
    for ( ref2 = parent->member.ilist; ref2 != NULL; ref2 = ref2->next )
      for ( ref = ref2->interval->forw_refs; ref != NULL; ref = ref->next ) {
        assert( ref->interval != NULL );
        if ( ref->interval->parent == NULL ) {
	  /* We've got hold of a node reachable from the current nodes in
	     the new interval that is not already in another interval. */
          iList *bref;
          int all_same = 1;
	  
	  /* Set all_same to 0 if any node not already in the new interval
	     can directly reach the current (ref) node. */
          for ( bref = ref->interval->back_refs;
                bref != NULL;
                bref = bref->next ) {
            if ( bref->interval->parent == NULL ||
                 bref->interval->parent != parent ) {
              all_same = 0;
              break;
            }
          }
	  
	  /* OK, so the current node (ref) is only reachable through nodes
	     already included in the new interval. */
          if ( all_same ) {
            ref->interval->parent = parent;
            parent->member.ilist =
	      makeIList( ref->interval, parent->member.ilist );
            changed = 1;
          }
        }
      }
  } while ( changed );

  /* Create new intervals for all intervals reachable from the current. */
  for ( ref2 = parent->member.ilist; ref2 != NULL; ref2 = ref2->next ) {
    for ( ref = ref2->interval->forw_refs; ref != NULL; ref = ref->next ) {
      iList *pref;

      /* Internal link, ie., a link within the just created interval. */
      if ( ref->interval->parent == parent )
        continue;

      noOfIntervals += iterateIntervals( ref->interval );

      /* Add the interval of the reachable node, if it hasn't already been. */
      for ( pref = parent->forw_refs; pref != NULL; pref = pref->next )
        if ( pref->interval == ref->interval->parent )
          break;
      if ( pref == NULL )
        addReference( parent, ref->interval->parent );
    }
  }

  return noOfIntervals;
}

/******************************************************************************
 *
 * Functions to compute the immediate dominators
 *
 *****************************************************************************/

static Interval *
commonDom( Interval *currImmDom, Interval *predImmDom )
{
  if ( currImmDom == NULL )
    return predImmDom;

  if ( predImmDom == NULL )
    return currImmDom;

  while ( currImmDom != NULL &&
          predImmDom != NULL &&
          currImmDom != predImmDom ) {
    if ( currImmDom->number < predImmDom->number )
      predImmDom = predImmDom->member.basic.immDom;
    else
      currImmDom = currImmDom->member.basic.immDom;
  }
  
  return currImmDom;
}

static void
findDominators( unsigned numIs, Interval *order[] )
{
  unsigned idx;
  iList *ref;
  
  for ( idx = 1; idx <= numIs; idx++ )
    for ( ref = order[idx]->back_refs; ref != NULL; ref = ref->next ) {
      order[idx]->member.basic.immDom
	= commonDom( order[idx]->member.basic.immDom, ref->interval );
    }
}

/******************************************************************************
 *
 * Loop detection functions
 *
 *****************************************************************************/

static void
markLoop( Loop *loop, Interval *interval )
{
  iList *ref;

  /* Note that the header of the loop is already marked as being
     part of the loop.  Thus, the algorithm stops when it has marked
     all nodes it meets going up to the header. */
  
  /* We have already marked this interval as being in the loop */
  if ( interval->member.basic.loop == loop )
    return;
  
  if ( interval->member.basic.loop != NULL ) {

    /* It can't be the loop we're marking at the moment, so it must
       be an inner loop.  So, change the interval under consideration
       to the header of this inner loop.  Here we're taking advantage of
       the fact that the header of this loop must be dominated by the
       header of the current loop.  Hence, any node reaching the inner
       loop header is also in the current loop. */

    interval = interval->member.basic.loop->header;
    assert( interval->member.basic.loop->outerLoop == NULL ||
	    interval->member.basic.loop->outerLoop == loop );
    interval->member.basic.loop->outerLoop = loop;
  } else
    interval->member.basic.loop = loop;
  
  /* Mark all nodes reaching this one. */

  for ( ref = interval->back_refs; ref != NULL; ref = ref->next )
    /* This is to avoid infinite loops */
    if ( ref->interval->number < interval->number )
      markLoop( loop, ref->interval );
}

static void
loopDetect( Interval *interval, Interval *order[] )
{
  iList *ref;
  Interval *header;

  assert( interval != NULL );
  /* This is a toplevel interval */
  assert( interval->parent == NULL );

  if ( interval->loopCheckDone == 1 )
    return;

  interval->loopCheckDone = 1;

  header = interval;
  while ( header->type == ilist ) {
    ref = header->member.ilist;
    while ( ref != NULL ) {
      header = ref->interval;
      ref = ref->next;
    }
  }

  /* Now 'header' points to the header node of the interval. */

  /* If this interval already is in a loop, skip it. */
  if ( header->member.basic.loop == NULL ) {
    Interval *latch;
    Interval *lparent;

    /* Find possible latch-node for this interval. */
    latch = NULL;

    /* Every node pointing to the current is a potential latch-node. */
    for ( ref = header->back_refs; ref != NULL; ref = ref->next ) {

      lparent = ref->interval;
      while ( lparent->parent != NULL )
        lparent = lparent->parent;

      /* lparent is now the toplevel interval for this node, ie., the
	 current generation. */

      /* If the node in question (ref) is in the same toplevel interval,
	 is not already in a loop, and is lower in the tree, ie., "further
	 away", than the currently proposed latch-node (if any), set this 
	 as the latch-node. */
      if ( lparent == interval &&
           ref->interval->member.basic.loop == NULL &&
           ( latch == NULL || latch->number < ref->interval->number ) )
        latch = ref->interval;
    }

    /* No latch-node, no loop. */
    if ( latch != NULL ) {
      Loop *loop;

      loop = (Loop *)malloc(sizeof(Loop));
      loop->header = header;
      loop->latch = latch;
      loop->type = endless;
      loop->outerLoop = NULL;
      loop->follow = NULL;

      header->member.basic.loop = loop;
      markLoop( loop, latch );
    }
  }

  /* Do loop detection for all intervals reachable from this one */
  for ( ref = interval->forw_refs; ref != NULL; ref = ref->next )
    /* No need to do loop detection for nodes higher up in the tree, we've
       already done them */
    if ( ref->interval->number > interval->number )
      loopDetect( ref->interval, order );

}

static int
inLoop( Interval *interval, Loop *loop )
{
  Loop *l = interval->member.basic.loop;

  assert ( loop != NULL );

  /* The interval is in no loops at all. */
  if ( l == NULL )
    return 0;

  /* The interval is directly in the loop. */
  if ( l == loop )
    return 1;
  
  /* The interval is in a loop, but it isn't the one wanted, and
     there are no outer loops. */
  if ( l->outerLoop == NULL )
    return 0;
  
  /* Ask whether the interval is in an outer loop. */
  return inLoop( l->outerLoop->header, loop );
}

static int
structWhileLoop( Loop *loop )
{
  Interval *then_branch, *else_branch;

  /* Either there is a plain prefix on the header, or it's a goto.
     We won't make a while loop of this. */
  if ( loop->header->member.basic.label->next->common.tag != If )
    return 0;

  then_branch = loop->header->forw_refs->next->interval;
  else_branch = loop->header->forw_refs->interval;

  if ( inLoop( then_branch, loop ) ) {
    if ( inLoop( else_branch, loop ) )
      /* Both branches go in to the loop body.  This isn't a while loop. */
      return 0;
    else
      /* Only the then-branch is in the loop.  Consequently, the follow
	 node must be the else-branch. */
      loop->follow = else_branch;
  } else
    /* If the then-branch doesn't reach, then the else-branch must. */
    loop->follow = then_branch;

  /* If the proposed follow loop isn't in the outerloop, then, again, this
     is not a while loop. */

  if ( loop->outerLoop != NULL && ! inLoop( loop->follow, loop->outerLoop ) )
    return 0;
  
  loop->type = wile;
  if ( loop->follow == then_branch )
    swapBranches( loop->header );
  return 1;
}

static int
structDoWhileLoop( Loop *loop )
{
  Interval *then_branch, *else_branch;

  /* By construction, a do-while loop must have a conditional latch. */

  if ( loop->latch->member.basic.control->common.tag != If )
    return 0;

  then_branch = loop->latch->forw_refs->next->interval;
  else_branch = loop->latch->forw_refs->interval;

  /* Note that only one of the branches point to the header,
     since we have explicitly assumed that all conditionals have different
     branches. */

  if ( then_branch == loop->header )
    loop->follow = else_branch;
  else
    loop->follow = then_branch;

  /* If there is an outer loop, then the follow of this loop must be in
     it.  Otherwise this isn't a do-while loop. */

  if ( loop->outerLoop != NULL && ! inLoop( loop->follow, loop->outerLoop ) )
    return 0;

  loop->type = dowile;
  if ( loop->follow == then_branch )
    swapBranches( loop->latch );
  return 1;
}

static Interval *
findFollow( Loop *loop, Interval *interval, Interval *bestFollow )
{
  Loop *iloop = interval->member.basic.loop;
  iList *ref;

  if ( ! inLoop( interval, loop ) )
    return bestFollow;

  if ( iloop != loop ) {
    assert( iloop->outerLoop == loop );
    interval = iloop->header;
  }

  for ( ref = interval->forw_refs; ref != NULL; ref = ref->next )
    if ( ! inLoop( ref->interval, loop ) )

      /* The branch leaves the loop, now if we don't have a follow yet,
	 or this one is closer, then we have found a new follow. */

      if ( bestFollow == NULL || ref->interval->number < bestFollow->number )
	bestFollow = ref->interval;

  for ( ref = interval->back_refs; ref != NULL; ref = ref->next )
    if ( ref->interval->number < interval->number )
      bestFollow = findFollow( loop, ref->interval, bestFollow );

  return bestFollow;
}

static void
structEndlessLoop( Loop *loop )
{
  /* Any loop can be casted as an endless loop, the only problem is finding
     the follow node. */

  loop->type = endless;
  loop->follow = findFollow( loop, loop->latch, NULL );
}

static void
structLoops( unsigned count, Interval *order[] )
{
  int idx;

  for ( idx = 1; idx <= count; idx++ )
    if ( order[idx]->member.basic.loop != NULL &&
	 order[idx]->member.basic.loop->header == order[idx] ) {
      Loop *loop = order[idx]->member.basic.loop;
      if ( ! structWhileLoop( loop ) &&
	   ! structDoWhileLoop( loop ) )
	structEndlessLoop( loop );
#ifdef SKALBERG_DEBUG
      printf( "Structured loop from %u to %u with ",
	      loop->header->number,
	      loop->latch->number );
      if ( loop->follow != NULL )
	printf( "follow %u.\n", loop->follow->number );
      else
	printf( "no follow.\n" );
#endif
    }
}

/******************************************************************************
 *
 * Conditional detection functions
 *
 *****************************************************************************/

static unsigned pathNum = 0;

static int
pathExists( Interval *from, Interval *to )
{
  iList *ref;

  if ( from == to )
    return 1;

  if ( from->number > to->number )
    return 0;

  from->path = pathNum;
  for ( ref = from->forw_refs; ref != NULL; ref = ref->next )
    if ( ref->interval->number > from->number &&
	 ref->interval->path != pathNum &&
	 pathExists( ref->interval, to ) )
      return 1;
  return 0;
}

static void
condDetect( unsigned count, Interval *order[] )
{
  /* unref is a list of conditionals for which there has not been assigned
     any follow node. */
  iList *unref = NULL, *newRef, *ref, **pref;
  int idx, idx2;

  /* The order is important! */
  for ( idx = count; idx >= 1; idx-- ) {
    if ( order[idx]->member.basic.control->common.tag == If ) {

      Loop *loop = order[idx]->member.basic.loop;
      
      /* We want to structure all conditional, except those occuring
	 actively as headers or latches in loops. */

      if ( loop != NULL && loop->type == wile && order[idx] == loop->header )
	continue;

      if ( loop != NULL && loop->type == dowile && order[idx] == loop->latch )
	continue;

      /* Add order[idx] to the unref list. */
      newRef = (iList *)malloc(sizeof(iList));
      newRef->interval = order[idx];
      newRef->next = unref;
      unref = newRef;

      for ( idx2 = count; idx2 > idx; idx2-- ) {
        if ( order[idx2]->member.basic.immDom == order[idx] &&
             order[idx2]->back_refs->next != NULL ) {

	  /* If the condtional is within a loop, the follow must also be. */

	  if ( loop != NULL && ! inLoop( order[idx2], loop ) )
	    continue;

	  /* order[idx2] is now the lowest node dominated by the conditional
	     under examination with more than one ingoing link. */

	  /* For every conditional in the unref list which is above the
	     proposed follow, if there is a path from the conditional to
	     the proposed follow node, use the follow node and remove the
	     conditional from unref. */

	  assert( unref != NULL );

	  pref = &unref;
	  while ( *pref != NULL && (*pref)->interval->number < idx2 ) {

	    ref = *pref;

	    if ( ref->interval->member.basic.loop != NULL &&
		 ! inLoop( order[idx2], ref->interval->member.basic.loop ) ) {
	      pref = &ref->next;
	      continue;
	    }

	    pathNum++;
	    if ( pathExists( ref->interval, order[idx2] ) ) {
	      assert( ref->interval->member.basic.ifFollow == NULL );
              ref->interval->member.basic.ifFollow = order[idx2];

	      /* We want the else-branch to be the follow. */
	      if ( ref->interval->forw_refs->next->interval == order[idx2] )
		swapBranches( ref->interval );
#ifdef SKALBERG_DEBUG
	      printf( "Conditional %u has follow %u.\n", ref->interval->number,
		      order[idx2]->number );
#endif
	      (*pref) = ref->next;
	    } else
	      pref = &ref->next;
	  }
          break;
        }
      }
    }
  }

  /* Cleanup, structure remaining conditionals in loops. */

  while ( unref != NULL ) {
    if ( unref->interval->member.basic.loop != NULL ) {
      Loop *loop = unref->interval->member.basic.loop;

      if ( inLoop( unref->interval->forw_refs->next->interval, loop ) ) {
	if ( inLoop( unref->interval->forw_refs->interval, loop ) ) {

	  /* If both branches were in the loop, then either both branches
	     point downwards, in which case we must already have structured
	     the conditional (and hence we wouldn't encounter the conditional
	     here), or one of the branches point to the header, in which case
	     we want that branch *not* to be the header. */

	  if ( unref->interval->forw_refs->interval == loop->header )
	    swapBranches( unref->interval );
	  else
	    assert ( unref->interval->forw_refs->next->interval ==
		     loop->header );
	} else
	  swapBranches( unref->interval );
      }
      
      /* So now the else-branch is the follow. */

      unref->interval->member.basic.ifFollow =
	unref->interval->forw_refs->interval;
#ifdef SKALBERG_DEBUG
      printf( "Condtional %u forced %u as follow.\n", unref->interval->number,
	      unref->interval->forw_refs->interval->number );
#endif

    }
    unref = unref->next;
  }
}

/******************************************************************************
 *
 * Graph reduction functions
 *
 *****************************************************************************/

static void
collapseCompounds( struct cmixStmtIf *xif )
{
  struct cmixStmtIf *yif;
  
  while ( 1 ) {
    if ( xif->then_target->next->common.tag == If &&
         xif->then_target->refcount == 1 ) {
      yif = &xif->then_target->next->cond;
      
      if ( yif->else_target == xif->else_target &&
           yif->then_target != xif->then_target ) {
        xif->cond = &cmixMkExp( "(?) && (?)", xif->cond, yif->cond )->inner;
        xif->then_target = yif->then_target;
        continue;
      }
      
      if ( yif->then_target == xif->else_target &&
           yif->else_target != xif->then_target ) {
        xif->cond =
                   &cmixMkExp( "! (?) || (?)", xif->cond, yif->cond )->inner;
        xif->then_target = yif->then_target;
        xif->else_target = yif->else_target;
        continue;
      }
      
    }
    
    if ( xif->else_target->next->common.tag == If &&
         xif->else_target->refcount == 1 ) {
      yif = &xif->else_target->next->cond;
      
      if ( yif->then_target == xif->then_target &&
           yif->else_target != xif->else_target ) {
        xif->cond = &cmixMkExp( "(?) || (?)", xif->cond, yif->cond )->inner;
        xif->else_target = yif->else_target;
        continue;
      }
      
      if ( yif->else_target == xif->then_target &&
           yif->then_target != xif->else_target ) {
        xif->cond = &cmixMkExp( "! (?) && (?)", xif->cond, yif->cond )->inner;
        xif->then_target = yif->then_target;
        xif->else_target = yif->else_target;
        continue;
      }

    }

    break;
  }
}

static struct cmixStmtLabel *
compress( struct cmixStmtLabel *label )
{
  struct cmixStmtLabel *result = NULL;
  union cmixStmt *stmt, **pstmt;

  stmt = label->next;
  pstmt = &label->next;

  if ( label->compressed ) {
    if ( stmt->common.tag == Goto )
      return stmt->jump.target;
    else
      return label;
  }

  label->compressed = 1;

  while ( stmt->common.tag == Plain ) {
    pstmt = &stmt->common.next;
    stmt = stmt->common.next;
    result = label;
  }

  assert ( stmt != NULL );

  switch ( stmt->common.tag ) {
    
  case Abort:
  case Return:
    result = label;
    break;

  case If:
    stmt->cond.then_target->refcount--;
    stmt->cond.then_target = compress( stmt->cond.then_target );
    stmt->cond.then_target->refcount++;
    
    stmt->cond.else_target->refcount--;
    stmt->cond.else_target = compress( stmt->cond.else_target );
    stmt->cond.else_target->refcount++;

    collapseCompounds( &stmt->cond );

    if ( stmt->cond.then_target == stmt->cond.else_target ) {
      union cmixStmt *newGoto =
	(union cmixStmt *)malloc(sizeof(struct cmixStmtGoto));

      stmt->cond.then_target->refcount--;
      
      newGoto->jump.tag = Goto;
      newGoto->jump.next = stmt->common.next;
      newGoto->jump.target = stmt->cond.then_target;
      *pstmt = newGoto;
      
      free( stmt ); /* hack: stmt->cond.cond not free'd */

      if ( result == NULL )
	result = newGoto->jump.target;
    }

    if ( result == NULL )
      result = label;
    break;

  case Goto:
    stmt->jump.target->refcount--;
    stmt->jump.target = compress( stmt->jump.target );
    stmt->jump.target->refcount++;

    if ( result == NULL )
      result = stmt->jump.target;
    break;
    
  case Plain:
  case Label:
  default:
    cmixFatal("Unexpected statement tag in compress");
  }

  assert ( result != NULL );
  return result;
}

/******************************************************************************
 *
 * Miscellaneous functions
 *
 *****************************************************************************/

#ifdef SKALBERG_DEBUG
static void
printIntervals( unsigned numIs, Interval *order[] )
{
  unsigned idx;
  iList *ref;

  for ( idx = 1; idx <= numIs; idx++ ) {
    printf( "%u: ", idx );
    for ( ref = order[idx]->forw_refs; ref != NULL; ref = ref->next ) {
      printf( "%u ", ref->interval->number );
      }
    if ( order[idx]->member.basic.immDom != NULL )
      printf( "(%u)", order[idx]->member.basic.immDom->number );
    printf( "\n" );
  }
}
#endif

Restruct *
cmixRestructStmts( union cmixStmt *stmt )
{
  Restruct *result;
  unsigned count, lastCount;
  union cmixStmt *firstLabel;
  Interval *currentTop;

  /* Ensure that the first statement is a label - compress only
     accepts labels. */
  if ( stmt->common.tag != Label ) {
    firstLabel = cmixMakeLabel();
    firstLabel->label.next = stmt;
    firstLabel->label.refcount = 0;
  } else
    firstLabel = stmt;

  firstLabel = (union cmixStmt *)compress( &firstLabel->label );

  result = (Restruct *)malloc(sizeof(Restruct));

  count = makeInterval( &firstLabel->label );
  currentTop = firstLabel->label.interval;

  result->order = (Interval **)malloc((count+1)*sizeof(Interval *));
  result->count = count;

  enumIntervals( currentTop, count, result->order );

  findDominators( count, result->order );

#ifdef SKALBERG_DEBUG
  printf( "/**\n");
  printIntervals( count, result->order );
#endif

  lastCount = 0;
  while ( count != lastCount ) {
    loopDetect( currentTop, result->order );
    lastCount = count;
    count = iterateIntervals( currentTop );
    currentTop = currentTop->parent;
  }

  structLoops( result->count, result->order );

  condDetect( result->count, result->order );

  //resetLabels( result->count, result->order );

#ifdef SKALBERG_DEBUG
  printf( "**/\n" );
#endif
  return result;
}
