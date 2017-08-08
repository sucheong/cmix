/*	File:	  code.h
 *      Author:	  Henning Makholm (makholm@diku.dk)
 *             	  Sebastian Skalberg (skalberg@diku.dk)
 *      Content:  CMix Cogen library: Code definitions.
 *
 *      Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 *      Redistribution and modification are allowed under certain
 *      terms; see the file COPYING.cmix for details.
 */
  
#ifndef __code__
#define __code__

struct Interval ;
struct cmixDecl ;
union cmixExpr ;

/*----------------------------------------------------------------------*
 * Expressions								*
 *----------------------------------------------------------------------*/

/* choice of magic: the Roman numeral CMIX means 0x38D */
enum cmixExprTag { LiftFloat = MKMAGIC(0x38D38D00,0x38D0),
                   LiftDouble, LiftLongDouble,
                   LiftTrueLongDouble,
                   Inner, NameRequest, Name, LiftChar, LiftInt };

struct cmixExprCommon {
    enum cmixExprTag tag ;
};
struct cmixExprInner {
    enum cmixExprTag tag ;
    char const *string ;
    union cmixExpr *child[1] ;
    /* struct hack used here */
};
struct cmixExprName {
    enum cmixExprTag tag ;
    unsigned index ;
    unsigned seq ;
    struct cmixDecl *decl ;
};
struct cmixExprLiftInt {
    enum cmixExprTag tag ;
    unsigned long data ;
};
struct cmixExprLiftFloat {
    enum cmixExprTag tag ;
    double data ;
    void (*write)(FILE *fp,int,double);
};
struct cmixExprLiftLD {
    enum cmixExprTag tag ;
    long double data ;
    void (*write)(FILE *fp,long double);
};
union cmixExpr {
    struct cmixExprCommon common ;
    struct cmixExprInner inner ;
    struct cmixExprName name ;
    struct cmixExprLiftInt lift_int ;
    struct cmixExprLiftFloat lift_float ;
    struct cmixExprLiftLD lift_longdouble ;
};

int /* bool */ cmixInnerOK(struct cmixExprInner*);
/* defined in unparse.cc because that is where printInner,
 * which parses the format string similarly, is
 */

/*----------------------------------------------------------------------*
 * Statements								*
 *----------------------------------------------------------------------*/

/* choice of magic: the current C-Mix maintainer's birthday as BCD
   with a 08 changed to 80 so it won't conflict with real BCD dates
   (whoever uses those in C programs. Perhaps cobol2c tranators?) */
enum cmixStmtTag { Label = MKMAGIC(0x19738013,0x1380),
                   Plain, Goto, If, Return, Abort };

union cmixStmt ;
struct cmixStmtCommon {
    enum cmixStmtTag tag ;
    union cmixStmt *next ;
};
struct cmixStmtLabel {
    enum cmixStmtTag tag ;
    union cmixStmt *next ;
    struct Interval *interval ;
    unsigned refcount ;
    unsigned number ;
    int defined ;
    int compressed ;
};    
struct cmixStmtPlain {
    enum cmixStmtTag tag ;
    union cmixStmt *next ;
    struct cmixExprInner expr ;
};
struct cmixStmtGoto {
    enum cmixStmtTag tag ;
    union cmixStmt *next ;
    struct cmixStmtLabel *target ;
};
struct cmixStmtIf {
    enum cmixStmtTag tag ;
    union cmixStmt *next ;
    struct cmixStmtLabel *then_target ;
    struct cmixStmtLabel *else_target ;
    struct cmixExprInner *cond ;
};
union cmixStmt {
    struct cmixStmtCommon common ;
    struct cmixStmtLabel label ;
    struct cmixStmtPlain plain ;
    struct cmixStmtGoto jump ;
    struct cmixStmtIf cond ;
};

/*----------------------------------------------------------------------*
 * Declarations								*
 *----------------------------------------------------------------------*/

/* choice of magic: the Roman numeral CMIX means 909 decimal! */
enum cmixDeclStatus { Sure = MKMAGIC(0x90990900,0x909), Maybe, Done };
struct cmixDecl {
    struct cmixDecl *next ; /* next in declaration order */

    struct cmixDecl *sibling ; /* next declaration for same name */
    enum cmixDeclStatus status ;
    struct cmixDecl *members ;
    struct cmixExprInner decl ;
} ;

/*----------------------------------------------------------------------*
 * Functions								*
 *----------------------------------------------------------------------*/

typedef struct cmixDataObjectCopy *cmixSavedState ;
struct cmixFun {
  struct cmixFun *next ;
  struct cmixFun *next_share ;
  struct cmixFun *prev_cur ;
  struct cmixDecl **prev_lastlocal ;
  
  Code resname ;
  struct cmixDecl *locals ;
  union cmixStmt *stmts, **last_stmt ;

  int shared ;
  cmixSavedState state, end_state ;
  struct cmixExprInner heading ;
};
extern struct cmixFun *cmixCurFun ;
void cmixFinishPushedFun(struct cmixFun *);

/*----------------------------------------------------------------------*
 * Globals								*
 *----------------------------------------------------------------------*/

extern struct cmixDecl *cmixDecls[cmixDECLCLASSES] ;
extern struct cmixFun *cmixFuns ;

/*---------------------------------------------------------------------------*
 * Restructuring
 *---------------------------------------------------------------------------*/

/* no magic numbers are used here, as the data structures are only
   created _after_ the specialization phase ends.
*/

extern int cmixRestruct;

struct Interval;
struct Loop;

typedef struct iList {
  struct Interval *interval;
  struct iList *next;
} iList;

typedef enum iType { basic, ilist } iType;

typedef union iMember {
  struct basic {
    struct cmixStmtLabel *label;
    union cmixStmt *control;
    struct Interval *ifFollow;
    struct Interval *immDom;
    struct Loop *loop;
    int printed;
  } basic;
  struct iList *ilist;
} iMember;

typedef struct Interval {
  unsigned number;
  struct Interval *parent;
  int loopCheckDone;
  unsigned path ;
  iType type;
  iMember member;
  iList *back_refs;
  iList *forw_refs;
} Interval;

enum loopType { wile, dowile, endless };

typedef struct Loop {
  enum loopType type;
  Interval *header;
  Interval *latch;
  Interval *follow;
  struct Loop *outerLoop;
} Loop;

typedef struct Restruct {
  Interval **order;
  unsigned count;
} Restruct;

Restruct *cmixRestructStmts(union cmixStmt *);

enum controlType { conditional, loop };

typedef struct Control {
  enum controlType type;
  union {
    Loop *loop;
    Interval *ifFollow;
  } info;
  struct Control *next;
} Control;
  
#endif /* __code__ */
