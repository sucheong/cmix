/* Author:   Henning Makholm (makholm@diku.dk)
 * Contents: C-Mix speclib: collection of residual code
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#define cmixSPECLIB_SOURCE
#include "speclib.h"
#include "code.h"

struct cmixDecl *cmixDecls[cmixDECLCLASSES] ;
struct cmixFun *cmixFuns ;

static struct cmixDecl **cmixLast[cmixDECLCLASSES] ;
struct cmixFun *cmixCurFun ;

#define DEBUG_CODE(args)
/* #define DEBUG_CODE(args)   printf args, fflush(stdout) */

static unsigned
CountInner(char const *fmt)
{
    unsigned count = 0 ;
    while ( *fmt )
        count += *fmt++ == '?' ;
    return count ;
}

#define ALLOC_INNER(type,fmt) \
     cmixMalloc(sizeof(type)+CountInner(fmt)*sizeof(union cmixExpr *))

static int
ArgOK(union cmixExpr *exp)
{
  unsigned i ;
  if ( exp == NULL )
    return 1;
  switch(exp->common.tag) {
  case LiftFloat: case LiftDouble: case LiftLongDouble:
  case LiftTrueLongDouble:
  case NameRequest: case Name: case LiftChar: case LiftInt:
    return 1;
  case Inner:
    for( i = CountInner(exp->inner.string) ; i-- ; )
      if ( !ArgOK(exp->inner.child[i]) )
        return 0;
    return 1 ;  
  }
  return 0 ;
}
  
static void
MakeInner(struct cmixExprInner *exp,char const *fmt,va_list ap)
{
    unsigned max = CountInner(fmt);
    unsigned i ;
    exp->tag = Inner ;
    exp->string = fmt ;
    for ( i = 0 ; i < max ; i++ ) {
      union cmixExpr *arg ;
      arg = va_arg(ap,union cmixExpr*) ;
      if ( ArgOK(arg) )
        exp->child[i] = arg ;
      else
        exp->child[i] = NULL ;
    }
    exp->child[i] = NULL ;
}

/*****************************************************************************/

Code
cmixMkExp(const char *fmt,...)
{
    Code r ;
    va_list pa ;
    va_start(pa,fmt) ;
    if ( fmt[0] == '?' && fmt[1] == '\0' )
        r = va_arg(pa,union cmixExpr*);
    else {
        r = (union cmixExpr*)ALLOC_INNER(struct cmixExprInner,fmt);
        MakeInner(&r->inner,fmt,pa);
    }
    va_end(pa);
    return r ;
}

Code
cmixRequestName(unsigned index, unsigned wseq)
{
    Code r = (union cmixExpr*) cmixMalloc(sizeof(struct cmixExprName));
    r->name.tag = NameRequest ;
    r->name.index = index ;
    r->name.seq = wseq ;
    r->name.decl = NULL ;
    return r ;
}

/***************************************************************************/
/* Lift operators							   */
/***************************************************************************/

Code
cmixLiftChar(unsigned char c)
{
    Code e = (union cmixExpr*) cmixMalloc(sizeof(struct cmixExprLiftInt)) ;
    e->lift_int.tag = LiftChar ;
    e->lift_int.data = c ;
    return e ;
}
Code
cmixLiftSigned(long d)
{
    if ( d >= 0 )
        return cmixLiftUnsigned(d);
    else
        return cmixMkExp("-?",cmixLiftUnsigned(-d));
}
Code
cmixLiftUnsigned(unsigned long l)
{
    Code e = (union cmixExpr*) cmixMalloc(sizeof(struct cmixExprLiftInt)) ;
    e->lift_int.tag = LiftInt ;
    e->lift_int.data = l ;
    return e ;
}

/* Lift a string to an expression */
Code
cmixLiftString(char const *s)
{
    char const *pc ;
    char *copy ;
    unsigned counter = 3 ;
    Code e = (union cmixExpr*) cmixMalloc(sizeof(struct cmixExprInner));
    e->inner.tag = Inner ;
    e->inner.child[0] = NULL ;
    for ( pc = s ; *pc ; pc++, counter++ ) {
        if ( *pc == '?' || *pc == ':' || *pc == '\'' || *pc == '#' ||
             *pc == '"' || *pc == '\\' )
            counter++ ;
        else if ( !isprint((unsigned char)*pc) )
            counter += 3 ;
    }
    copy = (char*) cmixMalloc(counter);
    e->inner.string = copy ;
    *copy++ = '"' ;
    for ( ; *s; s++ )
        switch(*s) {
        case '?': *copy++ = '#' ; *copy++ = '!' ; break ;
        case ':': *copy++ = '#' ; *copy++ = ';' ; break ;
        case '\'': *copy++ = '#' ; *copy++ = ',' ; break ;
        case '#': *copy++ = '#' ; *copy++ = '=' ; break ;
        case '"':
        case '\\': *copy++ = '\\' ; *copy++ = *s ; break ;
        case '\a': *copy++ = '\\' ; *copy++ = '\a' ; break ;
        case '\b': *copy++ = '\\' ; *copy++ = '\b' ; break ;
        case '\f': *copy++ = '\\' ; *copy++ = '\f' ; break ;
        case '\n': *copy++ = '\\' ; *copy++ = '\n' ; break ;
        case '\r': *copy++ = '\\' ; *copy++ = '\r' ; break ;
        case '\t': *copy++ = '\\' ; *copy++ = '\t' ; break ;
        case '\v': *copy++ = '\\' ; *copy++ = '\v' ; break ;
        default:
            if ( isprint((unsigned char)*pc) )
                *copy++ = *pc ;
            else {
                *copy++ = '\\' ;
                *copy++ = '0' + ( *pc >> 6 & 7 );
                *copy++ = '0' + ( *pc >> 3 & 7 );
                *copy++ = '0' + ( *pc & 7 );
            }
        }
    *copy++ = '"' ;
    *copy = '\0';

    return e ;
}

/* Lift a pointer to code */
Code
cmixLiftCode(Code *code)
{
    if (code == NULL)
        return cmixMkExp("0") ;
    else
        return cmixMkExp("&?",*code) ;
}

/***************************************************************************/
/* Declarations								   */
/***************************************************************************/

void
cmixDeclare(int cls, Code defvar, const char *fmt,...)
{
    va_list ap ;
    struct cmixDecl *d
        = (struct cmixDecl*) ALLOC_INNER(struct cmixDecl,fmt);
    d->next = NULL ;
    d->members = NULL ;
    if ( defvar ) {
        if ( defvar->common.tag != NameRequest )
            cmixFatal("Fatal: Definition for non-managed name\n");
        d->sibling = defvar->name.decl ;
        defvar->name.decl = d ;
        d->status = Maybe ;
    } else {
        d->sibling = NULL ;
        d->status = Sure ;
    }
    va_start(ap,fmt);
    MakeInner(&d->decl,fmt,ap);
    va_end(ap);
    if ( cls == cmixStruct )
        cmixLast[cmixMemberDecl] = &d->members ;
    *cmixLast[cls] = d ;
    cmixLast[cls] = &d->next ;
}

/***************************************************************************/
/* Statements								   */
/***************************************************************************/

static void
EmitStmt(union cmixStmt *s)
{
    *cmixCurFun->last_stmt = s ;
    cmixCurFun->last_stmt = &s->common.next ;
    s->common.next = NULL ;
}

cmixLabelTag
cmixMakeLabel()
{
    union cmixStmt *l
        = (union cmixStmt*) cmixMalloc(sizeof(struct cmixStmtLabel));
    l->label.tag = Label ;
    l->label.interval = NULL ;
    l->label.refcount = 0 ;
    l->label.number = 0 ;
    l->label.defined = 0 ;
    l->label.compressed = 0 ;
    return l ;
}

void
cmixLabel(cmixLabelTag l)
{
    if ( l->label.tag != Label )
        cmixFatal("non-label used as label\n");
    if ( l->label.defined )
        cmixFatal("label multiply defined\n");
    l->label.defined = 1 ;
    EmitStmt(l);
}

void
cmixStmt(const char *fmt,...)
{
    union cmixStmt *s
        = (union cmixStmt*) ALLOC_INNER(struct cmixStmtPlain,fmt);
    va_list ap ;
    va_start(ap,fmt);
    MakeInner(&s->plain.expr,fmt,ap);
    va_end(ap);
    if ( cmixInnerOK(&s->plain.expr) )
        s->plain.tag = Plain ;
    else
        s->plain.tag = Abort ;
    EmitStmt(s);
}

void cmixGoto(cmixLabelTag l)
{
    union cmixStmt *s
        = (union cmixStmt*) cmixMalloc(sizeof(struct cmixStmtGoto));
    s->jump.tag = Goto ;
    s->jump.target = &l->label ;
    EmitStmt(s);
}

void
cmixReturn(const char *fmt,...)
{
    union cmixStmt *s
        = (union cmixStmt*) ALLOC_INNER(struct cmixStmtPlain,fmt);
    va_list ap ;
    va_start(ap,fmt);
    MakeInner(&s->plain.expr,fmt,ap);
    va_end(ap);
    if ( cmixInnerOK(&s->plain.expr) )
        s->plain.tag = Return ;
    else
        s->plain.tag = Abort ;
    EmitStmt(s);
}

void
cmixIf(cmixLabelTag then_lab, cmixLabelTag else_lab, const char *fmt,...)
{
    union cmixStmt *s
        = (union cmixStmt*)malloc(sizeof(struct cmixStmtIf));
    va_list ap;
    s->cond.then_target = &then_lab->label ;
    s->cond.else_target = &else_lab->label ;
	s->cond.cond =
        (struct cmixExprInner *)ALLOC_INNER(struct cmixExprInner,fmt);
    va_start(ap,fmt);
    MakeInner(s->cond.cond,fmt,ap);
    va_end(ap);
    if ( cmixInnerOK(s->cond.cond) )
        s->cond.tag = If ;
    else {
        free(s);
        s = (union cmixStmt*) ALLOC_INNER(struct cmixStmtPlain,fmt) ;
        va_start(ap,fmt);
        MakeInner(&s->plain.expr,fmt,ap);
        va_end(ap);
        s->plain.tag = Abort ;
    }
    EmitStmt(s);
}

/***************************************************************************/
/* Functions								   */
/***************************************************************************/

/* Push current residual function definition on stack and initialize new */
void
cmixFunHeading(Code residname, const char *heading,...)
{
    va_list ap ;
    struct cmixFun *f
        = (struct cmixFun*) ALLOC_INNER(struct cmixFun,heading);
    DEBUG_CODE(("cmixFunHeading\n"));

    /* Link the new function into the chain */
    f->next = cmixFuns ;
    cmixFuns = f ;

    /* Mark it as the current function */
    f->prev_cur = cmixCurFun ;
    f->prev_lastlocal = cmixLast[cmixLocal] ;
    cmixCurFun = f ;
    
    /* initialize the new function */
    va_start(ap,heading);
    MakeInner(&f->heading,heading,ap);
    va_end(ap);

    f->resname = residname ;
    if ( ( residname->common.tag != Inner ||
           residname->inner.child[0] != NULL ) &&
         residname->common.tag != NameRequest )
        cmixFatal("strange function name\n");

    f->locals = NULL ; cmixLast[cmixLocal] = &f->locals ;
    f->stmts = NULL ; f->last_stmt = &f->stmts ;

    cmixFinishPushedFun(f);
}

/* Pop residual function definition from stack */
void
cmixPopFun()
{
  cmixLast[cmixLocal] = cmixCurFun->prev_lastlocal ;
  cmixCurFun = cmixCurFun->prev_cur ;
}

/* ===================== Initialization function ======================== */

void
cmixSpeclibInit()
{
    int i ;
    
    cmixFuns = NULL ;

    for(i=0; i < cmixDECLCLASSES ; i++ ) {
        cmixDecls[i] = NULL ;
        cmixLast[i] = &cmixDecls[i] ;
    }
}
