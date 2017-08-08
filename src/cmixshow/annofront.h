/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: annotation tree structure
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __annofront__
#define __annofront__

#include <stdio.h>
#include <stdlib.h>

char* stringDup(const char* str);      /* Duplicate a string (dynamically). */
void* safe_malloc(size_t s);
#define alloc(t) ((t*)(safe_malloc(sizeof(t))))
void die(const char* format,...);

typedef unsigned long magic ;
#define PCT_MAGIC "%lu"

#define TREE_LIST 5671
#define TREE_CONCAT 5672
#define TREE_LINK 5673
#define TREE_LABEL 5674
#define TREE_STRING 5675
#define TREE_UNINDENT 5676

#define SEP_NEWLINE 1621
#define SEP_BREAK 1622
#define SEP_NEVER 1623

#define SHOW_YES 19871
#define SHOW_HIDDEN 19872
#define SHOW_HIDE 19873

struct AnnoTree ;

struct BlockElement {
    struct AnnoTree *Contents ;
    int SeparatorTag ;
    int Spaces ;
    int NextOffset ;
    struct BlockElement *next ;
} ;


struct AnnoTree {
    int TreeTag ;
    int NativeWidth ; /* -1 if not prepared for PP yet */
    unsigned PrettyHeight ; /* May not be set if the node is not root */

    magic Type, Label, Attribute ;
    int ConsistentBlock ;
    int Indentation ;

    struct AnnoTree *TreeContent1, *TreeContent2 ;
    char *TextContent ;
    struct BlockElement *BlockContent ;
} ;

struct TreelistElt {
    struct TreelistElt *next ;
    struct AnnoTree *data ;
} ;

extern magic RootType ;

struct TypeRec {
    char *Name ;
    int defined ;
    /* for annocut.c */
    magic NextSibling ;
    magic FirstChild ;
    int InterestingSubtypes ;
    /* for annolink.c */
    int Showmode ;
    const char *ShownAsColor ;
    magic LinkDisplayed ;
    magic LinkToDisplay ;
    /* for annohash.c */
    int HasBeenHashed ;
    struct TreelistElt *trees ;
} ;

struct AnnoTree* alloctree(void);

struct TypeRec* typerec(magic) ;
void DoForAllTypes(void (*callback)(magic,struct TypeRec *)) ;

void AddAnnoTree(magic,struct AnnoTree*) ;
void DoForAllTrees(void (*callback)(magic,struct AnnoTree *)) ;

struct AnnoTree* FollowLabel(magic,magic);
struct AnnoTree* GetMainTree(void);

void FreeEverything(void);

/* annocut.c */

void DisplayCut(FILE *f,char*);
void CutOperation(magic,magic);
void NormalizeCut(void);

/* annolink.c */

void FinishTypeRec(const char *producer, struct TypeRec *tr);

/* webfront.c */

void webfront(const char *progname);

/* parser interface */

void readthetext(const char *s);
void perhapsreread(void);
const char *getfilename(void);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

enum services { NULL_PAGE, THE_PROGRAM, ANNOMULTI, ANNOSINGLE,
                CUTSELECTOR, CUTACTION
} ;

struct latch ;

extern struct inpp_hooks {
    void (*Reset)(FILE *);
    void (*BeginLink)(magic type,magic label,magic *oldlabel);
    void (*EndLink)(magic type,magic label,magic *oldlabel);
    void (*LinkAjour)(struct latch *);
    void (*LinkKill)(struct latch *);
    void (*putc)(struct latch *,char c);
    struct latch *(*FreshLine)(void);
    void (*DoneLine)(struct latch *,FILE *);
    const char *marker, *targetwarning ;
} const *inpp ;

extern struct inpp_hooks const framed_html_inpp ;
extern struct inpp_hooks const single_html_inpp ;
extern struct inpp_hooks const option_b_inpp ;
extern struct inpp_hooks const option_u_inpp ;
extern struct inpp_hooks const option_l_inpp ;
extern struct inpp_hooks const option_r_inpp ;

void debugprinter(struct AnnoTree *,int zero);
unsigned GetPrettyHeight(struct AnnoTree*);
void PrettyPrint(FILE *,struct AnnoTree *,magic label);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int yylex(void);

#if !defined(NULL)
#define NULL ((void*)0)
#endif

#endif /* __annofront__ */

