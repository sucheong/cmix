/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: text without links
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include "annofront.h"
#include "latch.h"
#include <string.h>

#ifdef putc
#undef putc
#endif

static int printing_static = 0 ;
static int printing_dynamic = 0 ;
static int any_annos = 0 ;

static magic the_static ;
static magic the_dynamic ;

static void
teststring(struct latch *o,const char *str)
{
    while(*str)
        inpp->putc(o,*str++);
}

static void
cb_findthem(magic type,struct TypeRec *t)
{
    if ( t->ShownAsColor == NULL )
        return ;
    if ( strstr(t->ShownAsColor,"<B>") != NULL )
        the_dynamic = type ;
    else
        the_static = type ;
}

static void
text_Reset(FILE *f)
{
    struct latch *Latch ;
    printing_static = printing_dynamic = 0 ;
    any_annos = 1 ;
    Latch = inpp->FreshLine();
    teststring(Latch,"/* Legend: ");
    printing_static = 1 ;
    teststring(Latch,"spectime");
    printing_static = 0 ;
    teststring(Latch,", ");
    printing_dynamic = 1 ;
    teststring(Latch,"residual");
    printing_dynamic = 0 ;
    teststring(Latch," */");
    inpp->DoneLine(Latch,f);
    fprintf(f,"\n");
    any_annos = 0 ;
    DoForAllTypes(cb_findthem);
}

static void
text_BeginLink(magic type,magic label,magic *oldlabel)
{
    if ( type == the_static )
        printing_static++ ;
    else if ( type == the_dynamic )
        printing_dynamic++ ;
}

static void
text_EndLink(magic type,magic label,magic *oldlabel)
{
    if ( type == the_static )
        printing_static-- ;
    else if ( type == the_dynamic )
        printing_dynamic-- ;
}

static void
text_LinkAjour(struct latch *dummy)
{
    any_annos = 1 ;
}

static void
text_LinkKill(struct latch *dummy) {
    any_annos = 0 ;
}

static void
text_putc(struct latch *o,char c,int bflag, int ulflag)
{
    latchc(o,c) ;
    if ( !any_annos )
        return ;
    if ( bflag )
        latchc(o,'\b'), latchc(o,c);
    if ( ulflag )
        latchc(o,'\b'), latchc(o,'_');
}

static void
putc_b(struct latch *o,char c)
{
    text_putc(o,c,printing_dynamic,0);
}

static void
putc_u(struct latch *o,char c)
{
    text_putc(o,c,0,printing_dynamic);
}

static void
putc_l(struct latch *o,char c)
{
    text_putc(o,c,printing_dynamic,printing_static);
}

static void
putc_r(struct latch *o,char c)
{
    text_putc(o,c,printing_static,printing_dynamic);
}

static void
text_DoneLine(struct latch *o,FILE *f)
{
    latchc(o,'\n');
    outputlatch(o,f);
    deletelatch(o);
}

static const char text_marker[] =
   "------------------------------------------------------------------\n" ;

struct inpp_hooks const option_b_inpp = {
    text_Reset,
    text_BeginLink, text_EndLink,
    text_LinkAjour, text_LinkKill,
    putc_b,
    newlatch, text_DoneLine,
    text_marker, ""
};

struct inpp_hooks const option_u_inpp = {
    text_Reset,
    text_BeginLink, text_EndLink,
    text_LinkAjour, text_LinkKill,
    putc_u,
    newlatch, text_DoneLine,
    text_marker, ""
};

struct inpp_hooks const option_l_inpp = {
    text_Reset,
    text_BeginLink, text_EndLink,
    text_LinkAjour, text_LinkKill,
    putc_l,
    newlatch, text_DoneLine,
    text_marker, ""
};

struct inpp_hooks const option_r_inpp = {
    text_Reset,
    text_BeginLink, text_EndLink,
    text_LinkAjour, text_LinkKill,
    putc_r,
    newlatch, text_DoneLine,
    text_marker, ""
};

