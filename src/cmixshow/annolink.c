/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: active HTML
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include "annofront.h"
#include "latch.h"
#include <string.h>
#include <assert.h>

/********************************************************************/
/*  Initialization for fancy formatting                             */
/********************************************************************/

struct fancydef { const char *name, *color; } ;

void FinishTypeRec(char const *producer, struct TypeRec *t) {
    static struct fancydef fancydefs[] = {
        { "cmixII", NULL },
        { "Spectime", "<FONT COLOR=\"#008000\"><I>@</I></FONT>" },
        { "Residual", "<FONT COLOR=\"#FF0000\"><B>@</B></FONT>" },
        { NULL, NULL } } ;
    static int cache ;
    int i ;
        
    if ( strcmp(producer, fancydefs[cache].name) != 0 ) {
        cache = 0 ;
        while ( fancydefs[cache].name != NULL &&
                strcmp(producer, fancydefs[cache].name) != 0 ) {
            do cache++ ; while ( fancydefs[cache].color != NULL ) ;
        }
        if ( fancydefs[cache].name == NULL ) {
            t->ShownAsColor = NULL ;
            cache = 0 ;
            return ;
        }
    }

    for ( i = cache+1 ; fancydefs[i].color != NULL ; i++ )
        if ( strcmp(t->Name, fancydefs[i].name) == 0 ) {
            t->ShownAsColor = fancydefs[i].color ;
            return ;
        }
    t->ShownAsColor = NULL ;
}

/********************************************************************/
/*  Routines called by the pretty-printer                           */
/********************************************************************/

static int Atag_open ;
static int LinkChanged ;

static void cb_ResetAll(magic type,struct TypeRec *t) {
    t->LinkDisplayed = 0 ;
    t->LinkToDisplay = 0 ;
}

static void
html_Reset(FILE *dummy) {
    DoForAllTypes(cb_ResetAll);
    Atag_open = 0 ;
    LinkChanged = 0 ;
}

static void
html_BeginLink(magic type,magic label,magic *oldlabel) {
    struct TypeRec *t = typerec(type);
    *oldlabel = t->LinkToDisplay ;
    t->LinkToDisplay = label ;
    if ( t->Showmode == SHOW_YES || t->ShownAsColor != NULL )
        LinkChanged = 1 ;
}

static void
html_EndLink(magic type,magic label,magic *oldlabel) {
    struct TypeRec *t = typerec(type);
    t->LinkToDisplay = *oldlabel ;
    if ( t->Showmode == SHOW_YES || t->ShownAsColor != NULL )
        LinkChanged = 1 ;
}

static int ActiveLinks ;
static magic LastLinkType, LastLinkLabel ;
static struct latch *LinkLatch ;
static char *ColorStopFiller = "" ;  /* NB! This is filled *backwards* */

static void cb_FindDifferences(magic type,struct TypeRec *t) {
    if ( t->Showmode != SHOW_YES && t->ShownAsColor == NULL )
        return ;
    if ( t->LinkToDisplay != t->LinkDisplayed )
        LinkChanged = 1 ;
    if ( t->LinkToDisplay && t->Showmode == SHOW_YES ) {
        LastLinkType = type ;
        LastLinkLabel = t->LinkToDisplay ;
        ActiveLinks++ ;
    }
}

static void cb_OutputLinkComponent(magic type,struct TypeRec *t) {
    if ( t->LinkToDisplay && t->Showmode == SHOW_YES )
        latchf(LinkLatch,"/"PCT_MAGIC"/"PCT_MAGIC,type,t->LinkToDisplay);
}

static void cb_FinishoffLink(magic type,struct TypeRec *t) {
    int i ;
    const char *pc ;
    t->LinkDisplayed = t->LinkToDisplay;
    if ( t->ShownAsColor && t->LinkDisplayed ) {
        for ( pc = t->ShownAsColor ; *pc && *pc != '@' ; pc++ )
            latchc(LinkLatch,*pc);
        if ( *pc == '@' ) {
            pc++ ;
            i = strlen(pc) ;
            strncpy(ColorStopFiller-=i,pc,i) ;
        }
    }
}

static void cb_ResetDisplayedLinks(magic type,struct TypeRec *t) {
    t->LinkDisplayed = 0 ;
}

static void
html_LinkKill(struct latch *o) {
    latchs(o,ColorStopFiller);
    ColorStopFiller = "" ;
    if ( Atag_open ) {
        latchs(o,"</A>");
        Atag_open = 0 ;
    }
    DoForAllTypes(cb_ResetDisplayedLinks);
    LinkChanged = 1 ;
}

static void
html_LinkAjour(struct latch *o) {
    if ( !LinkChanged )
        return ;
    LinkChanged = 0 ;
    ActiveLinks = 0 ;
    DoForAllTypes(cb_FindDifferences);
    if ( !LinkChanged )
        return ;
    html_LinkKill(o);
    LinkChanged = 0 ;

    LinkLatch = o ;
    if ( ActiveLinks ) {
        if ( ActiveLinks == 1 &&
             GetPrettyHeight(FollowLabel(LastLinkType,LastLinkLabel)) >= 10 ) {
            latchf(LinkLatch,
                  "<a target=program href=\"/%d/"PCT_MAGIC"/"PCT_MAGIC"#a\">",
                  ANNOSINGLE,LastLinkType,LastLinkLabel);
        } else {
            latchf(LinkLatch,"<a href=\"/%d",ANNOMULTI);
            DoForAllTypes(cb_OutputLinkComponent);
            latchs(LinkLatch,"\">");
        }
        Atag_open = 1 ;
    }
    if (1) {
        static char ColorStopper[100] ;
        ColorStopFiller = ColorStopper+99 ;
        *ColorStopFiller = '\0' ;
        DoForAllTypes(cb_FinishoffLink);
    }
}

static void
html_putc(struct latch *o,char c) {
    switch(c) {
    case ' ':
        latchc(o,160) ; /* Non-breaking space */
        break ;
    case '<':
        latchs(o,"&lt;") ;
        break ;
    case '>':
        latchs(o,"&gt;") ;
        break ;
    case '&':
        latchs(o,"&amp;") ;
        break ;
    default:
        latchc(o,c) ;
        break ;
    }
}

const char html_marker[] = "<hr noshade>" ;

const char html_targetwarning[] = "<a name=\"a\"></a>"  ;

static struct latch *
html_FreshLine(void) {
    struct latch *o = newlatch() ;
    latchs(o,"\xA0<tt>");
    return o ;
}

static void
html_DoneLine(struct latch *o,FILE *f) {
    latchs(o,"</tt><br>\n");
    outputlatch(o,f);
    deletelatch(o);
}

struct inpp_hooks const framed_html_inpp = {
    html_Reset,
    html_BeginLink, html_EndLink,
    html_LinkAjour, html_LinkKill,
    html_putc,
    html_FreshLine, html_DoneLine,
    html_marker, html_targetwarning
};

struct inpp_hooks const single_html_inpp = {
    html_Reset,
    html_BeginLink, html_EndLink,
    html_LinkAjour, html_LinkKill,
    html_putc,
    html_FreshLine, html_DoneLine,
    html_marker, html_targetwarning
};

