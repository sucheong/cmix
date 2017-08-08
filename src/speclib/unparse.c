/*	File:	  unparse.c
 *      Author:	  Henning Makholm <makholm@diku.dk>
 *      	  Sebastian Skalberg <skalberg@diku.dk>
 *      Content:  C-Mix speclib: Manage names and print residual program
 *
 *      Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 *      Redistribution and modification are allowed under certain
 *      terms; see the file COPYING.cmix for details.
 */
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#define cmixSPECLIB_SOURCE
#include "speclib.h"
#include "code.h"

/***************************************************************************/
/* Adding taboos to a name table                                           */
/***************************************************************************/

static void
addTaboo(struct cmixNameRec names[],unsigned const namesSorted[],
         unsigned toohigh, const char *taboo)
{
  char mycopy[31] ;
  char *pc ;
  unsigned thisseq ;
  unsigned lower ;
  if ( strlen(taboo)>=31 ||
       taboo[0] == '\0' ||
       isdigit((unsigned char)taboo[0]) )
    return ; /* generated names have maximal length of 30 */
  strcpy(mycopy,taboo);
  
  /* Strip of any trailing sequence of digits in the taboo */
  for ( pc=mycopy ; pc[0] ; pc++ )
    ;
  while ( isdigit((unsigned char)pc[-1]) )
    pc-- ;
  if ( strlen(pc) > 4 && (pc != mycopy+1 || mycopy[0] != 'v') )
    return; /* generated names have at most 4 digits at the end */
  
  if ( pc[0] ) {
    thisseq = cmix_atoi(pc);
    pc[0] = '\0' ;
  } else
    thisseq = 0 ;
  
  /* NOW do a binary search in the name table */
  lower = 0 ;
  while(lower < toohigh) {
    unsigned mid = (lower+toohigh)/2 ;
    int cmpres = strcmp(names[namesSorted[mid]].base,mycopy);
    if ( cmpres == 0 ) {
      unsigned index = namesSorted[mid] ;
      if ( names[index].nextseq_global <= thisseq )
        names[index].nextseq_global = thisseq+1 ;
      return ;
    }
    if ( cmpres < 0 )
      lower = mid+1 ;
    else
      toohigh = mid ;
  }
  /* the name part wasn't found: all is well */
}   

/***************************************************************************/
/* Traversing expressions                                                  */
/***************************************************************************/

typedef void (*Callback)(struct cmixExprName *,struct cmixNameRec[]) ;

static void traverseExpr(union cmixExpr *,Callback,struct cmixNameRec[]);
static void
traverseInner(struct cmixExprInner *e,Callback cb,struct cmixNameRec names[])
{
  unsigned i ;
  for ( i=0; e->child[i] ; i++ )
    traverseExpr(e->child[i],cb,names);
}
static void
traverseExpr(union cmixExpr *e,Callback cb,struct cmixNameRec names[])
{
  if ( e == NULL )
    return ;
  switch(e->common.tag) {
  case Inner:
    traverseInner(&e->inner,cb,names);
    break ;
  case NameRequest:
  case Name:
    cb(&e->name,names);
    break ;
  default:
    break ;
  }
}

/***************************************************************************/
/* Computing needed declarations                                           */
/***************************************************************************/

static void mentionName(struct cmixExprName *,struct cmixNameRec []);

static void
mentionDecl(struct cmixDecl *d)
{
  if ( d->status == Done )
    return ;
  d->status = Done ;
  traverseInner(&d->decl,mentionName,NULL);
  for ( d = d->members ; d ; d = d->next )
    if ( d->status == Sure )
      mentionDecl(d) ;
}

static void
mentionName(struct cmixExprName *n,struct cmixNameRec dummy[])
{
  struct cmixDecl *d ;
  for ( d = n->decl ; d ; d = d->sibling )
    mentionDecl(d);
}

static void
mentionFun(struct cmixFun *f)
{
  union cmixStmt *s ;
  int stmtReachable ;
  
  stmtReachable = 1 ;
  for ( s = f->stmts ; s ; s=s->common.next )
    switch(s->common.tag) {
    case Label:
      if ( stmtReachable )
        s->label.refcount++ ;
      stmtReachable = 1 ;
      break ;
    case Plain:
      traverseInner(&s->plain.expr,mentionName,NULL);
      break ;
    case Goto:
      s->jump.target->refcount++ ;
      stmtReachable = 0 ;
      break ;
    case If:
      s->cond.then_target->refcount++;
      s->cond.else_target->refcount++;
      traverseInner(s->cond.cond,mentionName,NULL);
      stmtReachable = 0 ;
      break ;
    case Return:
      traverseInner(&s->plain.expr,mentionName,NULL);
      stmtReachable = 0 ;
      break ;
    case Abort:
      traverseInner(&s->plain.expr,mentionName,NULL);
      stmtReachable = 0 ;
      break ;
    }
}

/***************************************************************************/
/* Name management functions                                               */
/***************************************************************************/

static void
assignGlobalName(struct cmixExprName *e,struct cmixNameRec names[])
{
  if ( e->tag != NameRequest )
    return ;
  e->tag = Name ;
 tryagain:
  if ( e->seq < names[e->index].nextseq_global )
    e->seq = names[e->index].nextseq_global ;
  if ( e->seq >= 10000 && e->index ) {
    /* panic! too many of these names defined - revert to
     * index 0 whose base name is "v".
     */
    e->index = 0 ;
    e->seq = 0 ;
    goto tryagain ;
  }
  names[e->index].nextseq_global = e->seq + 1 ;
}

static void
prepareLocalName(struct cmixExprName *e,struct cmixNameRec names[])
{
  names[e->index].nextseq_local = names[e->index].nextseq_global ;
}

static void
assignLocalName(struct cmixExprName *e,struct cmixNameRec names[])
{
  if ( e->tag != NameRequest )
    return ;
  e->tag = Name ;
 tryagain:
  if ( e->seq < names[e->index].nextseq_local )
    e->seq = names[e->index].nextseq_local ;
  if ( e->seq >= 10000 && e->index ) {
    /* panic! too many of these names defined - revert to
     * index 0 whose base name is "v".
     */
    e->index = 0 ;
    e->seq = 0 ;
    goto tryagain ;
  }
  names[e->index].nextseq_local = e->seq + 1 ;
}

/***************************************************************************/
/* Printing a cmixExpr                                                     */
/***************************************************************************/

static int
ExprOK(union cmixExpr *exp)
{
  if ( exp == NULL )
    return 0 ;
  if (exp->common.tag != Inner)
    return 1 ;
  return cmixInnerOK(&exp->inner);
}

int
cmixInnerOK(struct cmixExprInner *exp)
{
  char const *pc ;
  unsigned i = 0 ;
  unsigned nolevel = 0 ;
  unsigned yeslevel = 0 ;
  int inPling = 0 ;
  int printing = 1 ;
  for ( pc = exp->string ; *pc ; pc++ )
    switch(*pc) {
    case '?':
      if ( printing && !ExprOK(exp->child[i]) )
        return 0 ;
      i++ ;
      break ;
    case ':':
      if ( nolevel != 0 || exp->child[i] == NULL )
        nolevel++, printing = 0 ;
      else
        yeslevel++ ; /* printing == 1 */
      break ;
    case '\'':
      if ( inPling )
        printing = nolevel == 0 ;
      else {
        if ( nolevel )
          nolevel-- , printing = nolevel == 0 ;
        else if ( yeslevel )
          yeslevel-- , printing = 0 ;
      }
      inPling = !inPling ;
      break ;
    default:
      break ;
    }
  return 1 ;
}

static void printExpr(union cmixExpr *,struct cmixNameRec names[],FILE *f);

static void
printInner(struct cmixExprInner *e,struct cmixNameRec names[],FILE *f)
{
  char const *pc ;
  unsigned i = 0 ;
  unsigned nolevel = 0 ;
  unsigned yeslevel = 0 ;
  int inPling = 0 ;
  int printing = 1 ;
  if ( f == NULL ) return;
  for ( pc = e->string ; *pc ; pc++ )
    switch(*pc) {
    case '#':
      if ( printing ) switch(*++pc) {
      case '=':
        putc('#',f);
        break ;
      case '!':
        putc('?',f);
        break ;
      case ';':
        putc(':',f);
        break ;
      case ',':
        putc('\'',f);
        break ;
      default:
        cmixFatal("strange char in expr format\n");
      }
      break ;
    case '?':
      if ( printing ) printExpr(e->child[i],names,f);
      i++ ;
      break ;
    case ':':
      if ( inPling )
        cmixFatal("Pling imbalance in skip string\n");
      if ( nolevel != 0 || e->child[i] == NULL )
        nolevel++, printing = 0 ;
      else
        yeslevel++ ; /* printing == 1 */
      break ;
    case '\'':
      if ( inPling )
        printing = nolevel == 0 ;
      else {
        if ( nolevel )
          nolevel-- , printing = nolevel == 0 ;
        else if ( yeslevel )
          yeslevel-- , printing = 0 ;
        else
          cmixFatal("Too many plings in skip string\n");
      }
      inPling = !inPling ;
      break ;
    default:
      if ( printing ) putc(*pc,f);
      break ;
    }
  if ( nolevel || yeslevel || inPling )
    cmixFatal("Too many hashes in skip string\n");
  return ;
}

static void
printExpr(union cmixExpr *e,struct cmixNameRec names[],FILE *f)
{
  if ( e == NULL ) {
    fprintf(f,"{0}") ;
    return ;
  }
  switch(e->common.tag) {
  case Inner:
    printInner(&e->inner,names,f);
    break ;
  case Name:
    if ( e->name.seq == 0 )
      fprintf(f,"%s",names[e->name.index].base) ;
    else
      fprintf(f,"%s%u",names[e->name.index].base,e->name.seq) ;
    break ;
  case LiftChar:
    switch( e->lift_int.data ) {
    case '\\': fprintf(f,"'\\\\'"); break ;
    case '\'': fprintf(f,"'\\''"); break ;
    case '\a': fprintf(f,"'\\a'"); break ;
    case '\b': fprintf(f,"'\\b'"); break ;
    case '\f': fprintf(f,"'\\f'"); break ;
    case '\n': fprintf(f,"'\\n'"); break ;
    case '\r': fprintf(f,"'\\r'"); break ;
    case '\t': fprintf(f,"'\\t'"); break ;
    case '\v': fprintf(f,"'\\v'"); break ;
    default: {
      unsigned char data = e->lift_int.data ;
      if ( isprint(data) )
        fprintf(f,"'%c'",data);
      else if ( data < 512 ) /* i.e representable as 3 octal digits */
        fprintf(f,"'\\%o'",data);
      else
        fprintf(f,"'\\x%X'",data);
      break ; }
    }
    break ;
  case LiftInt:
    fprintf(f,"%lu",e->lift_int.data);
    break ;
  case LiftFloat:
  case LiftDouble:
  case LiftLongDouble:
    e->lift_float.write(f,e->common.tag,e->lift_float.data);
    break ;
  case LiftTrueLongDouble:
    e->lift_longdouble.write(f,e->lift_longdouble.data);
    break;
  case NameRequest:
    cmixFatal("Some name didn't get managed\n");
  default:
    fprintf(stderr,"magic=%x\n",e->common.tag);
    cmixFatal("bad expression magic in unparse()\n");
  }
}

/***************************************************************************/
/* Printing declarations and function bodies                               */
/***************************************************************************/

static void
printDecl(struct cmixDecl *d,struct cmixNameRec names[], FILE *of)
{
  printInner(&d->decl,names,of);
  if ( d->members ) {
    int gotany = 0 ;
    struct cmixDecl *m ;
    fprintf(of," {");
    for(m=d->members; m; m=m->next)
      if ( m->status == Done ) {
        gotany = 1 ;
        printDecl(m,names,of);
      }
    if ( !gotany )
      fprintf(of," int:1; }");
    else
      fprintf(of,"\n}");
  }
}

static void
printIndent(int indent,FILE *f)
{
  if (f==NULL) return;
  while(indent--) {
    putc(' ',f);
    putc(' ',f);
  }
}

static void
printGoto(int indent,struct cmixStmtLabel *target,FILE *f)
{
  printIndent(indent,f);
  fprintf(f,"goto L%u;\n",target->number);
}

static void
printStmtSeq(union cmixStmt *s,int indent,struct cmixNameRec names[], FILE *f)
{
  while(1) {
    if ( s == NULL ) {
      fprintf(f,"/* FALLING OFF THE END OF THE FUNCTION! */\n");
      return ;
    }
    switch(s->common.tag) {
    case Label:
      if ( s->label.refcount > 1 ) {
        printGoto(indent,&s->label,f);
        return ;
      }
      /* else print nothing */
      break ;
    case Plain:
      printIndent(indent,f);
      printInner(&s->plain.expr,names,f);
      fprintf(f,";\n");
      break ;
    case Goto:
      if ( s->jump.target->refcount > 1 ) {
        printGoto(indent,s->jump.target,f);
        return ;
      } else {
        s = s->jump.target->next ;
        continue ;
      }
    case If:
      printIndent(indent,f);
      fprintf(f,"if (");
      printInner(s->cond.cond,names,f);
      putc(')',f);
      if ( s->cond.then_target->refcount > 1 ) {
        putc('\n',f);
        printGoto(indent+1,s->cond.then_target,f);
        printIndent(indent,f);
      } else {
        fprintf(f," {\n");
        printStmtSeq(s->cond.then_target->next,indent+1,names,f);
        printIndent(indent,f);
        fprintf(f,"} ");
      }
      fprintf(f,"else");
      if ( s->cond.else_target->refcount > 1 ) {
        putc('\n',f);
        printGoto(indent+1,s->cond.else_target,f);
      } else {
        fprintf(f," {\n");
        printStmtSeq(s->cond.else_target->next,indent+1,names,f);
        printIndent(indent,f);
        fprintf(f,"}\n");
      }
      return ;
    case Return:
      printIndent(indent,f);
      fprintf(f,"return ");
      printInner(&s->plain.expr,names,f);
      fprintf(f,";\n");
      return ;
    case Abort:
      printIndent(indent,f);
      fprintf(f,"abort();\n");
      printIndent(indent,f);
      fprintf(f,"/* residual deref of stray spectime pointer at `{0}':\n");
      printIndent(indent,f);
      fprintf(f," * ");
      printInner(&s->plain.expr,names,f);
      fprintf(f," */\n");
      return ;
    }
    s = s->common.next ;
  }
}

static void
printStmts(union cmixStmt *s,struct cmixNameRec names[],FILE *f)
{
  union cmixStmt *s2 ;
  unsigned labelcounter = 1;
  /* first, assign numbers to the important labels */
  for ( s2 = s ; s2 ; s2 = s2->common.next )
    if ( s2->common.tag == Label && s2->label.refcount > 1 )
      s2->label.number = labelcounter++ ;
  /* then we can print the body */
  printStmtSeq(s,1,names,f);
  for ( ; s ; s = s->common.next )
    if ( s->common.tag == Label && s->label.refcount > 1 ) {
      fprintf(f,"L%u:\n",s->label.number);
      printStmtSeq(s->label.next,1,names,f);
    }
}

/*****************************************************************************/

static void fprintf2( FILE *f, const char *fmt, ... )
{
  va_list pa;
  if ( f == NULL ) return;
  va_start(pa,fmt);
  vfprintf(f,fmt,pa);
  va_end(pa);
}

static unsigned nextFree;

static Interval *
ifFollow( Interval *interval )
{
  return interval->member.basic.ifFollow;
}

static Interval *
thenBranch( Interval *interval )
{
  return interval->forw_refs->next->interval;
}

static Interval *
elseBranch( Interval *interval )
{
  return interval->forw_refs->interval;
}

static Loop *
getLoop( Interval *interval )
{
  return interval->member.basic.loop;
}

static Interval *
loopFollow( Interval *interval )
{
  return interval->member.basic.loop->follow;
}

static Interval *
loopLatch( Interval *interval )
{
  return interval->member.basic.loop->latch;
}

static int
printed( Interval *interval )
{
  return interval->member.basic.printed;
}

static Control *
addLoop( Loop *l, Control *list )
{
  Control *c = (Control*)malloc(sizeof(Control));

  assert( loop != NULL );

  c->type = loop;
  c->info.loop = l;
  c->next = list;

  return c;
}

static Control *
addConditional( Interval *iffollow, Control *list )
{
  Control *c = (Control*)malloc(sizeof(Control));

  assert( ifFollow != NULL );

  c->type = conditional;
  c->info.ifFollow = iffollow;
  c->next = list;

  return c;
}

static void
printBB( FILE *f, int indent, struct cmixNameRec names[], Interval *interval )
{
  union cmixStmt *stmt;

  assert( ! printed( interval ) );
  interval->member.basic.printed = 1;

  if ( interval->number > 0 )
    fprintf2( f, "L%u:\n", interval->number );

  for ( stmt = interval->member.basic.label->next;
	stmt->common.tag == Plain;
	stmt = stmt->common.next ) {
    printIndent( indent, f );
    printInner(&stmt->plain.expr,names,f);
    fprintf2(f,";\n");
  }

  assert( stmt == interval->member.basic.control );

  switch ( stmt->common.tag ) {
  case Return:
    printIndent(indent,f);
    fprintf2(f,"return ");
    printInner(&stmt->plain.expr,names,f);
    fprintf2(f,";\n");
    break;
  case Abort:
    printIndent(indent,f);
    fprintf2(f,"abort();\n");
    printIndent(indent,f);
    fprintf2(f,"/* residual deref of stray spectime pointer at `{0}':\n");
    printIndent(indent,f);
    fprintf2(f," * ");
    printInner(&stmt->plain.expr,names,f);
    fprintf2(f," */\n");
    break;
  default:
  }
}

static int
trivialJump( Interval *target, Control *control )
{
  if ( control != NULL ) {
    switch ( control->type ) {
    case conditional:
      /* Falling out of a structured conditional moves you to
	 the follow. */
      if ( target == control->info.ifFollow )
	return 1;
      break;
    case loop:
      /* So we reached the latch of the loop.  This will be
	 printed automatically immediately after. */
      if ( target == control->info.loop->latch )
	return 1;
      break;
    default:
      cmixFatal( "Unknown control structure in trivialJump" );
    }
  }

  return 0;
}

static void
printJump( FILE *f, int indent, Interval *target, Control *control )
{
  Control *cref;

  if ( trivialJump( target, control ) )
    return;

  printIndent( indent, f );
  
  /* Check if a break or continue is applicable */
  for ( cref = control; cref != NULL; cref = cref->next )
    if ( cref->type == loop ) {
      if ( target == cref->info.loop->follow ) {
	fprintf2( f, "break;\n" );
	return;
      } else
	switch ( cref->info.loop->type ) {
	case wile:
	case endless:
	  if ( target == cref->info.loop->header ) {
	    fprintf2( f, "continue;\n" );
	    return;
	  }
	  break;
      case dowile:
	if ( target == cref->info.loop->latch ) {
	  fprintf2( f, "continue;\n" );
	  return;
	}
	break;
	default:
	  cmixFatal( "Unknown loop type in printJump" );
	}
      break;
    }
  
  if ( f == NULL && target->number == 0 )
    target->number = nextFree++;
  
  assert( target->number > 0 );
  fprintf2( f, "goto L%u;\n", target->number );
}

static void printInterval( FILE *, int, struct cmixNameRec[], Interval *,
			   Control * );

static void
printPlain( FILE *f, int indent, struct cmixNameRec names[],
	    Interval *interval, Control *control )
{
  printBB( f, indent, names, interval );
  if ( interval->forw_refs != NULL ) {
    assert( interval->forw_refs->next == NULL );
    printInterval( f, indent, names, interval->forw_refs->interval, control );
  }
}

static void
printConditional( FILE *f, int indent, struct cmixNameRec names[],
		  Interval *interval, Control *control )
{
  struct basic *info = &interval->member.basic;

  assert( info->control->common.tag == If );

  printBB( f, indent, names, interval );

  printIndent( indent, f );
  fprintf2( f, "if (" );
  printInner( info->control->cond.cond, names, f );
  fprintf2( f, ") {\n" );

  if ( ifFollow( interval ) != NULL ) {
    Control *newControl = addConditional( info->ifFollow, control );

    printInterval( f, indent + 1, names, thenBranch( interval ), newControl );

    if ( elseBranch( interval ) != ifFollow( interval ) ) {
      printIndent( indent, f );
      fprintf2( f, "} else {\n" );
      printInterval( f, indent + 1, names, elseBranch( interval ),
		     newControl );
    }

    free( newControl );

    printIndent( indent, f );
    fprintf2( f, "}\n" );
    
    printInterval( f, indent, names, ifFollow( interval ), control );
  } else {
    printInterval( f, indent + 1, names, thenBranch( interval ), control );
    printIndent( indent, f );
    fprintf2( f, "} else {\n" );
    printInterval( f, indent + 1, names, elseBranch( interval ), control );
    printIndent( indent, f );
    fprintf2( f, "}\n" );
  }
}

static void
printLoop( FILE *f, int indent, struct cmixNameRec names[],
	   Interval *interval, Control *control )
{
  struct basic *info = &interval->member.basic;
  Control *newControl = addLoop( info->loop, control );

  switch ( info->loop->type ) {
  case wile:
    assert( info->control->common.tag == If );
    printBB( f, indent + 1, names, interval );
    printIndent( indent, f );
    fprintf2( f, "while (" );
    printInner( info->control->cond.cond, names, f );
    fprintf2(f,") {\n");
    break;
  case dowile:
    assert( info->loop->latch->member.basic.control->common.tag == If );
    printIndent( indent, f );
    fprintf2( f, "do {\n" );
    if ( info->control->common.tag == If && loopLatch( interval ) != interval )
      printConditional( f, indent + 1, names, interval, newControl );
    else
      printBB( f, indent + 1, names, interval );
    break;
  case endless:
    printIndent( indent, f );
    fprintf2( f, "for (;;) {\n" );
    if ( info->control->common.tag == If )
      printConditional( f, indent + 1, names, interval, newControl );
    else
      printBB( f, indent + 1, names, interval );
    break;
  default:
    cmixFatal( "Unknown loop type in printLoop" );
  }

  if ( interval != loopLatch( interval ) ) {

    /* Now print the body of the loop */

    if ( info->loop->type == wile )
      printInterval( f, indent + 1, names, interval->forw_refs->next->interval,
		     newControl );
    else if ( info->control->common.tag != If )
      printInterval( f, indent + 1, names, interval->forw_refs->interval,
		     newControl );

    /* Any latch can only belong to one loop. */

    if ( loopLatch( interval )->member.basic.control->common.tag == If &&
	 info->loop->type != dowile )
      printConditional( f, indent + 1, names, loopLatch( interval ),
			newControl );
    else
      printBB( f, indent + 1, names, loopLatch( interval ) );
  }

  free( newControl );

  /* At this point, the latch node has already been printed.  Either because
     it was also the header node, or explicitly above. */

  assert( printed( loopLatch( interval ) ) );

  switch ( info->loop->type ) {
  case wile:
    printIndent( indent, f );
    fprintf2( f, "}\n" );
    break;
  case dowile:
    printIndent( indent, f );
    fprintf2( f, "} while (" );
    printInner( loopLatch( interval )->member.basic.control->cond.cond,
		names, f );
    fprintf2( f, ");\n" );
    break;
  case endless:
    printIndent( indent, f );
    fprintf2( f, "}\n" );
    break;
  default:
    cmixFatal( "Unknown loop type in printLoop" );
  }

  if ( loopFollow( interval ) != NULL )
    printInterval( f, indent, names, loopFollow( interval ), control );
}

/* printElsewhere decides whether the interval should be printed
   elsewhere.  The return code is:

   0 - Print here.
   >0 - print later:
     1 - Already printed.
     2 - Latch of loop.
     3 - Follow of loop or conditional.
*/
static int
printElsewhere( Interval *interval, Control *control )
{
  Control *cref;

  /* If the interval has already been printed, then it should obviously
     not be printed again.  This case also takes care of jumps to
     sorrounding loops, since the header is (of course) the first
     interval printed from a loop. */

  if ( printed( interval ) )
    return 1;

  /* If the interval is the latch of a loop, it will be printed when
     the loop is printed.  On the other hand, if the interval is also
     the header of the loop, then this may be the only place to print
     it. */

  if ( interval->member.basic.loop != NULL &&
       interval == interval->member.basic.loop->latch &&
       interval != interval->member.basic.loop->header)
    return 2;

  /* If the interval is the follow of any structure, we should wait till
     we're out of the structure before printing it. */

  for ( cref = control; cref != NULL; cref = cref->next )
    if ( ( cref->type == loop && cref->info.loop->follow == interval ) ||
	 ( cref->type == conditional && cref->info.ifFollow == interval ) )
      return 3;
  
  return 0;
}

static void
printInterval( FILE *f, int indent, struct cmixNameRec names[],
	       Interval *interval, Control *control )
{
  struct basic *info;

  assert( interval != NULL );
  assert( interval->type == basic );

  info = &interval->member.basic;

  /* If this interval will be or is printed elsewhere, print a jump. */
  if ( printElsewhere( interval, control ) ) {
    printJump( f, indent, interval, control );
    return;
  }
  
  if ( getLoop( interval ) != NULL && interval == info->loop->header )
    printLoop( f, indent, names, interval, control );
  else if ( info->control->common.tag == If )
    printConditional( f, indent, names, interval, control );
  else
    printPlain( f, indent, names, interval, control );
}

static void
resetLabels( unsigned count, Interval *order[] )
{
  int idx;

  for ( idx = 1; idx <= count; idx++ )
      order[idx]->number = 0;
}

static void
resetPrint( unsigned count, Interval *order[] )
{
  int idx;

  for ( idx = 1; idx <= count; idx++ )
    order[idx]->member.basic.printed = 0;
}

/*****************************************************************************/

int cmixRestruct = 1;

void
cmixUnparse(struct cmixNameRec names[], unsigned const namesSorted[],
            unsigned namesCount, FILE *outfile)
{
  int cls ;
  struct cmixFun *f ;
  struct cmixDecl *d, *m ;
  
  /* step 1: add the latter-day taboos from the generator functions.
   */
  for ( f = cmixFuns ; f ; f=f->next ) {
    if ( f->resname->common.tag == Inner )
      addTaboo(names,namesSorted,namesCount,
               f->resname->inner.string);
  }
  
  /* step 2: walk through all of the code, marking those declarations
   * that are actually mentioned, and counting predecessors for each
   * label.
   */
  for ( f = cmixFuns ; f ; f=f->next )
    mentionFun(f);
  for( cls = 0 ; cls < cmixDECLCLASSES ; cls++ )
    for(d = cmixDecls[cls] ; d ; d=d->next )
      if ( d->status == Sure )
        mentionDecl(d);
  
  /* step 3: assign names to global variables and functions
   */
  for( cls = 0 ; cls < cmixDECLCLASSES ; cls++ )
    for(d = cmixDecls[cls] ; d ; d=d->next )
      if ( d->status == Done )
        traverseInner(&d->decl,assignGlobalName,names);
  for(f = cmixFuns ; f ; f=f->next )
    if ( f->resname->common.tag == NameRequest )
      assignGlobalName(&f->resname->name,names);
  
  /* step 4a: assign names within each function
   */
  for(f = cmixFuns ; f ; f=f->next ) {
    traverseInner(&f->heading,prepareLocalName,names);
    for(d = f->locals ; d ; d=d->next )
      traverseInner(&d->decl,prepareLocalName,names);
    names[0].nextseq_local = names[0].nextseq_global ;
    
    traverseInner(&f->heading,assignLocalName,names);
    for(d = f->locals ; d ; d=d->next )
      if ( d->status == Done )
        traverseInner(&d->decl,assignLocalName,names);
  }
  /* step 4b: .. and within each struct
   */
  for( cls = 0 ; cls < cmixDECLCLASSES ; cls++ )
    for(d = cmixDecls[cls]; d ; d=d->next ) {
      for( m=d->members; m ; m=m->next )
        traverseInner(&m->decl,prepareLocalName,names);
      for( m=d->members; m ; m=m->next )
        if ( m->status == Done )
          traverseInner(&m->decl,assignLocalName,names);
    }
  
  /* step 5: print the program
   */
  for( cls = 0 ; cls < cmixDECLCLASSES ; cls++ ) {
    for(d = cmixDecls[cls] ; d ; d=d->next ) {
      if ( d->status == Done ) {
        printDecl(d,names,outfile);
        fprintf(outfile,";\n");
      }
    }
    putc('\n',outfile);
    if ( cls == cmixGlobal ) {
      for(f = cmixFuns ; f ; f=f->next )
        if ( f->shared ) {
          printInner(&f->heading,names,outfile);
          fprintf(outfile,";\n");
        }
      for(f = cmixFuns ; f ; f=f->next ) {
        putc('\n',outfile);
        printInner(&f->heading,names,outfile);
        fprintf(outfile,"\n{\n");
        for(d = f->locals ; d ; d=d->next )
          if (d->status == Done) {
            fprintf(outfile,"  ");
            printDecl(d,names,outfile);
            fprintf(outfile,";\n");
          }
        if ( cmixRestruct ) {
          Restruct *res = cmixRestructStmts(f->stmts);
	  nextFree = 1;
	  resetLabels( res->count, res->order );
	  printInterval( NULL, 1, names, res->order[1], NULL );
	  resetPrint( res->count, res->order );
	  printInterval( outfile, 1, names, res->order[1], NULL );
        } else {
          printStmts(f->stmts,names,outfile);
        }
        fprintf(outfile,"}\n");
      }
      putc('\n',outfile);
    }
  }
}
