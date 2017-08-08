/* File:     speclib.h
 * Author:   Lars Ole Andersen (lars@diku.dk)
 *           Peter Holst Andersen (txix@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix specialization library definitions
 *
 * Copyright © 1998-2000. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef cmixSPECLIB_INCLUDED
#define cmixSPECLIB_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define cmixSPECLIB_VERSION 2011

#ifdef cmixSPECLIB_SOURCE
#  include <cmixconf.h>

#  ifdef __cplusplus
#    error speclib is not compiled with C++ anymore
#  endif
#  define NEW(T) ((T*)cmixMalloc(sizeof(T)))
#  define HACKNEW(T,Extra) ((T*)cmixMalloc(sizeof(T)+(Extra)))
#  define FREE(P) free(P)

#  define VOIDPTR_PLUS(a,b) ((void*)((char*)(a)+(b)))
#  define VOIDPTR_MINUS(a,b) ((char*)(a)-(char*)(b))
  
#  if SIZEOF_INT >= 4
#    define MKMAGIC(a,b) a
#  else
#    define MKMAGIC(a,b) b
#  endif
#endif /* cmixSPECLIB_SOURCE */

/*---------------------------------------------------------------------------*
 * Resource statistics                                                       *
 *---------------------------------------------------------------------------*/

void cmixCheckMemusage(void);
void cmixPrintMemusage(void);
void cmixCollectStatistic(int, int);

#ifdef cmixSPECLIB_SOURCE
#  define STAT_CMP_STATE		0
#  define STAT_CMP_STATE_MATCH		1
#  define STAT_LOOKUP_SHARED		2
#  define STAT_LOOKUP_SHARED2		3
#  define STAT_SHARED_FOUND		4
#  define STAT_ALLOCATED		5
#  define STAT_CMP_NULL			6
#  define STAT_CMP_CHECK_GLOBALS	7
#  define STAT_CMP_GLOBALS		8
#  define STAT_CMP_CHECK_STACK		9
#  define STAT_CMP_STACK		10
#  define STAT_CMP_CHECK_HEAP		11
#  define STAT_CMP_HEAP			12
#  define STAT_CMP_NOT_FOUND		13
#  define STAT_COPY_NULL		14
#  define STAT_COPY_CHECK_GLOBALS	15
#  define STAT_COPY_GLOBALS		16
#  define STAT_COPY_CHECK_STACK		17
#  define STAT_COPY_STACK		18
#  define STAT_COPY_CHECK_HEAP		19
#  define STAT_COPY_HEAP		20
#  define STAT_COPY_NOT_FOUND		21
#  define STAT_LAST			22
#endif

/*---------------------------------------------------------------------------*
 * Static-object management                                                  *
 *---------------------------------------------------------------------------*/

struct cmixDataObjectCopy;
typedef struct cmixDataObjectCopy cmixDataObjectCopy ;

typedef int (*cmixPointerCallback)(cmixDataObjectCopy **,void const *);
typedef int (*cmixPointerFollower)
    (cmixPointerCallback,cmixDataObjectCopy**,void*,unsigned);

typedef struct cmixDataObject {
  unsigned long ID ;
  void *obj ;
  size_t size ;
  cmixPointerFollower follow_pointers ;
  int visit ;
} cmixDataObject ;

/* Trivial pointer-following function */
int cmix_follow0(cmixPointerCallback,cmixDataObjectCopy**,void*,unsigned);

/*---------------------------------------------------------------------------*
 * Code type                                                                 *
 *---------------------------------------------------------------------------*/

union cmixExpr ;

typedef union cmixExpr* Code ;

#define cmixPutName0(cmixThis,cmixIt) (*(cmixThis)=(cmixIt))
Code cmixMkExp(const char*,...);

/* Lift operators */
Code cmixLiftChar(unsigned char);
Code cmixLiftSigned(long);
Code cmixLiftUnsigned(unsigned long);
Code cmixLiftFloat(int length,double);
Code cmixLiftLongDouble(long double);
Code cmixLiftString(char const*);
Code cmixLiftCode(Code *);
#define cmixLiftPtr(a) ((a)?*(a):NULL)
#define cmixLiftStructPtr(a) ((a)?(a)->code:NULL)

/*---------------------------------------------------------------------------*
 * Code generation                                                           *
 *---------------------------------------------------------------------------*/

void cmixDeclare(int,Code,const char *,...);
/* Declarations fall i multiple classes, as selected by the first
 * parameter to cmixDeclare */

/* forward declarations for struct and union types are output
 * directly by cmixGenExit. */
#define cmixStruct     0 /* proper declarations of struct and union types */
#define cmixGlobal     1 /* initializer-less declarations of globals */
/* function definitions go here */
#define cmixGinit      2 /* initializers for initialized global variables */
#define cmixLocal      3 /* pseudo-section for local variables */
#define cmixMemberDecl 4 /* pseudo-section for member declarations */
#define cmixDECLCLASSES 5

/* Residual statement generators (they emit code to the current function) */
typedef union cmixStmt *cmixLabelTag ;

cmixLabelTag cmixMakeLabel() ;
void cmixLabel(cmixLabelTag);
void cmixStmt(const char *,...); /* expression stmt - speclib supplies a ";" */
void cmixGoto(cmixLabelTag);
void cmixIf(cmixLabelTag thenLabel, cmixLabelTag elseLabel, const char *,...);
void cmixReturn(const char *,...); /* Generate return with expr */

/* Functions */
typedef char const *cmixMemoWhat ;
struct cmixFun ;
struct cmixFunPool {
  struct cmixFun *list ;
};
void cmixPushFun(void);
Code cmixFindOrPushFun(struct cmixFunPool*, cmixDataObject[], cmixMemoWhat);
void cmixFunHeading(Code residname, const char *heading,...);
void cmixSaveExitState(cmixDataObject[], cmixMemoWhat);
void cmixPopFun(void);

/*---------------------------------------------------------------------------*
 * Memoization                                                               *
 *---------------------------------------------------------------------------*/

#ifdef cmixSPECLIB_SOURCE
  void* cmixMalloc(size_t);

  void *cmixCreateMemoCopy(void *org,size_t size);
  void cmixFreeMemoCopy(void *copy,size_t size);
#endif
  
/* =================Tracking objects for Function Memoization ===============*/

void cmixPushGlobals(cmixDataObject[]);
void cmixPushLocals(cmixDataObject[]);
void cmixPopLocals(int);
void *cmixAllocStatic(unsigned long ID, size_t, unsigned nelem,
                      cmixPointerFollower);

/* ========================= Basic-Block Memoization ======================= */

typedef unsigned char const *cmixLocalMemoWhat ;

void cmixPushPend(cmixDataObject[],cmixLocalMemoWhat[],unsigned);
cmixLabelTag cmixPendinsert(unsigned);
unsigned cmixPending(void);
void cmixPopPend(void);

  /* internal data structures are in pending.c */

/*---------------------------------------------------------------------------*
 * Unparsing, name management                                                *
 *---------------------------------------------------------------------------*/

struct cmixNameRec {
    char const *base ;
    unsigned nextseq_global ;
    unsigned nextseq_local ;
} ;
Code cmixRequestName(unsigned index,unsigned wseq);

void cmixUnparse(struct cmixNameRec names[],unsigned const namesSorted[],
                 unsigned cNames, FILE *);

/*---------------------------------------------------------------------------*
 * Errors etc                                                                *
 *---------------------------------------------------------------------------*/
void cmixFatal(char *); /* Print error mesg. and abort. */

#ifdef cmixSPECLIB_SOURCE
#  ifdef NDEBUG
#    define assert(X) (void)0
#  else
#    define assert(X) if (X) ; else cmixAssertfail1(#X,__FILE__,__LINE__)
#    define cmixAssertfail1(A,F,L) cmixAssertfail(A,F,L)
#    define cmixAssertfail(A,F,L) \
        cmixFatal("Assertation failed: `" A "'\n" F ":" #L \
                  ": location of assert")
#  endif
#endif

/*---------------------------------------------------------------------------*
 * Utility functions to parse static arguments from P_gen command line args  *
 *---------------------------------------------------------------------------*/
char cmix_atoc(char *);
long cmix_atoi(char *);
double cmix_atof(char *);

#define cmixCONCAT2(a,b) cmixCONCAT(a,b)
#define cmixCONCAT(a,b) a ## b
#define cmixSpeclibInit cmixCONCAT2(cmixVersionStamp,cmixSPECLIB_VERSION)
void cmixSpeclibInit();

#ifdef __cplusplus
#define externC extern "C"
#else
#define externC
#endif

externC void cmixGenExit(FILE*) ; /* may be forward referenced */
externC void cmixGenInit();       /* ditto */

#ifdef __cplusplus
}
#endif
  
#else /* cmixSPECLIB_INCLUDED */
#  ifdef cmixSPECLIB_SOURCE
     /* all is well */
#  else
#    error Something is wrong: *-gen.c should not include this file twice!
#  endif  
#endif /* cmixSPECLIB_INCLUDED */
