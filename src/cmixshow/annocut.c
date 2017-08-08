/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: Annotation selector
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include "annofront.h"

FILE *dest ;
char *MyUrl ;

static int ComputeInterest(magic type) {
    struct TypeRec *t = typerec(type);
    t->InterestingSubtypes = 0 ;
    for ( type = t->FirstChild ; type ; type=typerec(type)->NextSibling )
        if ( ComputeInterest(type) )
            t->InterestingSubtypes++ ;
    if (!t->trees && t->InterestingSubtypes == 1)
        t->Showmode = SHOW_YES ;
    return t->InterestingSubtypes || t->trees ;
}

static void ListThem(magic type, struct TypeRec *t) ;

static void ListSubtypes(struct TypeRec *t) {
    magic i ;
    if ( t->InterestingSubtypes ) {
        fprintf(dest,"<ul>");
        for ( i = t->FirstChild; i; i=t->NextSibling ) {
            t = typerec(i);
            ListThem(i,t);
        }
        fprintf(dest,"</ul>");
    }
}
    
static void ListThem(magic type, struct TypeRec *t) {
    magic i ;
    struct TypeRec *r;

 restart:
    if ( !t->trees ) switch( t->InterestingSubtypes ) {
    case 0:
        return ;
    case 1:
        for ( i = t->FirstChild; i; i=r->NextSibling ) {
            r = typerec(i);
            if ( (r->trees || r->InterestingSubtypes) ) {
                if ( r->Showmode == SHOW_HIDDEN )
                    r->Showmode = SHOW_HIDE ;
                type = i ;
                t = r ;
                goto restart ;
            }
        }
        return; /* ???? */
    }
    
    switch(t->Showmode) {
    case SHOW_HIDDEN:
        fprintf(dest,"<li>%s: Unexpectedly hidden. How did you do that??",
                     t->Name);
        break ;
    case SHOW_YES:
        fprintf(dest,"<li><a href=\"%s/0/"PCT_MAGIC"\"><tt><i>[-]"
                     "</i></tt></a> <b>%s</b>",
                     MyUrl,type,t->Name);
        if (t->trees)
            fprintf(dest,": shown");
        ListSubtypes(t);
        break ;
    case SHOW_HIDE:
        fprintf(dest,"<li><a href=\"%s/1/"PCT_MAGIC"\"><i><tt>[+]"
                     "</i></tt></a> (not shown: %s)\n",
                     MyUrl,type,t->Name);
        break ;
    default:
        die("Unknown showmode %d",t->Showmode);
    }
    if ( t->ShownAsColor ) {
        const char *pc ;
        fprintf(dest," - marked with ") ;
        for ( pc = t->ShownAsColor ; *pc ; pc++ )
            if ( *pc == '@' )
                fprintf(dest,"emphasis") ;
            else
                putc(*pc,dest);
    }
}

void DisplayCut(FILE *f,char* myurl) {
    magic i ;
    struct TypeRec *r ;
    
    dest = f ;
    MyUrl = myurl ;

    for ( i = RootType; i; i=r->NextSibling ) {
        r = typerec(i);
        ComputeInterest(i);
        fprintf(f,"<b>%s</b>: Always shown\n",r->Name);
        ListSubtypes(r);
        fprintf(f,"<p>\n");
    }
}

static void Flood(struct TypeRec *t,int before,int after) {
    magic i ;
    for ( i = t->FirstChild ; i ; i = t->NextSibling ) {
        t = typerec(i);
        if ( t->Showmode == before ) {
            t->Showmode = after ;
            Flood(t,before,after);
        }
    }
}

void CutOperation(magic enable,magic type) {
    struct TypeRec *t = typerec(type);
    if ( enable ) {
        t->Showmode = SHOW_YES ;
        Flood(t,SHOW_HIDDEN,SHOW_YES);
    } else {
        t->Showmode = SHOW_HIDE ;
        Flood(t,SHOW_YES,SHOW_HIDDEN);
    }
}


static void Normalize2(struct TypeRec *t) {
    magic i ;
    for ( i = t->FirstChild; i; i = t->NextSibling ) {
        t = typerec(i);
        if ( t->Showmode == SHOW_YES )
            t->Showmode = SHOW_HIDDEN ;
        Normalize2(t);
    }
}

static void Normalize1(struct TypeRec *t) {
    magic i ;
    for ( i = t->FirstChild; i; i = t->NextSibling ) {
        t = typerec(i) ;
        switch(t->Showmode) {
        case SHOW_HIDDEN:
            t->Showmode = SHOW_HIDE ;
            /* fall through */
        case SHOW_HIDE:
            Normalize2(t);
            break ;
        case SHOW_YES:
            Normalize1(t);
            break ;
        default:
            die("Unknown showmode %d",t->Showmode);
        }
    }
}

void NormalizeCut(void) {
    magic i ;
    struct TypeRec *t ;
    for ( i = RootType ; i ; i = t->NextSibling ) {
        t = typerec(i) ;
        t->Showmode = SHOW_YES ;
        Normalize1(t);
    }
}
   
