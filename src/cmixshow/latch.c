/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: latch for finished HTML data
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "latch.h"

#define CHUNKSIZE 1020

struct chain {
    char chunk[CHUNKSIZE] ;
    struct chain *next ;
} ;

struct latch {
    char chunk[CHUNKSIZE] ;
    struct chain *first ;
    struct chain **last ;
    char *filler ;
    unsigned left ;
} ;

void latchf(struct latch *o,const char* format,...) {
    char buffer[CHUNKSIZE] ;
    int i ;
    va_list vl ;
    va_start(vl,format) ;
    i = vsprintf(buffer,format,vl);
    assert( i >= 0 && i < CHUNKSIZE );
    latchs(o,buffer);
}

void latchs(struct latch *o,const char *s) {
    while ( *s )
        latchc(o,*s++) ;
}

void latchc(struct latch *o,char c) {
    if ( o->left <= 0 ) {
        struct chain *new = alloc(struct chain) ;
        new->next = NULL ;
        o->filler = new->chunk ;
        o->left = CHUNKSIZE ;
        *(o->last) = new ;
        o->last = &(new->next) ;
    }
    *(o->filler++) = c ;
    o->left-- ;
}

static void prepare_latch(struct latch *o) {
    o->first = NULL ;
    o->last = &(o->first);
    o->filler = o->chunk ;
    o->left = CHUNKSIZE ;
}

static int latchinuse = 0 ;

static struct latch thelatch ;

struct latch *newlatch() {
    assert( !latchinuse );
    latchinuse = 1 ;
    prepare_latch(&thelatch);
    return &thelatch ;
}
void deletelatch(struct latch *o) {
    struct chain *temp1, *temp2 ;
    temp1 = o->first ;
    while ( temp1 != NULL ) {
        temp2 = temp1->next ;
        free(temp1);
        temp1 = temp2 ;
    }
    
    assert( latchinuse );
    latchinuse = 0 ;
}

void outputlatch(struct latch *o, FILE *f) {
    char *pc = o->chunk ;
    unsigned left = CHUNKSIZE ;
    struct chain *next = o->first ;
    while ( pc != o->filler ) {
        if ( left != 0 ) {
            putc(*pc++,f) ;
            left-- ;
        } else {
            assert(next != NULL);
            pc = next->chunk ;
            left = CHUNKSIZE ;
            next = next->next ;
        }
    }
}
            
        
