/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: active HTML
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include "annofront.h"
#include "http.h"
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define ARGLIST (FILE*fo,unsigned *ints,unsigned nrints,char *rest)
#define METHOD(x) static void x ARGLIST

METHOD(Null_Page);
METHOD(The_Program);
METHOD(AnnoMulti);
METHOD(AnnoSingle);
METHOD(CutSelector);
METHOD(CutAction);
static void toppage(FILE *fo) ;
typedef void (*Method) ARGLIST ;

METHOD(badurl) {
	unsigned i ;

	http_begin(fo,404,"No such thing");
	http_data(fo,"text/html");
	fprintf(fo,"<html><h1>This should not happen</h1>\n"
		"Your URL, <samp>%s",
		URLbase);
	for ( i = 0 ; i < nrints ; i++ )
		fprintf(fo,"%u/",ints[i]) ;
	fprintf(fo,"/%s</samp> has no defined meaning.</html>\n",rest);
}

static Method dispatch(int x) {
	switch(x) {
        case NULL_PAGE: return Null_Page;
        case THE_PROGRAM: return The_Program;
        case ANNOMULTI: return AnnoMulti;
        case ANNOSINGLE: return AnnoSingle;
        case CUTSELECTOR: return CutSelector;
        case CUTACTION: return CutAction;
        default: return badurl;
	}
}

void http_expanded(unsigned*ints,unsigned nrints,char *rest,FILE *fo) {
	if ( nrints == 0 )
		toppage(fo) ;
	else
		dispatch(ints[0])(fo,ints,nrints,rest) ;
}

static void redirect(FILE *fo,char *format,...) {
	va_list vl ;

	http_begin(fo,302,"Data are here");
	fprintf(fo,"Location: %s",URLbase);
	va_start(vl,format);vfprintf(fo,format,vl);va_end(vl);
	fprintf(fo,"\r\n");
	http_data(fo,"text/html");
	fprintf(fo,"<html>Sigh. <a href=\"%s",URLbase);
	va_start(vl,format);vfprintf(fo,format,vl);va_end(vl);
	fprintf(fo,">Click here</a>.</html>\n");
}

#if 0
static void urlcpy
  (char *dest,int max,unsigned*ints,unsigned nrints,char *rest) {
	unsigned j ;
	while ( nrints > 0 && max > 20 ) {
		j = sprintf(dest,"%d/",*ints) ;
		ints++, nrints-- ;
		dest += j ;
		max -= j ;
	}
	*dest = '/' ;
	strncpy(dest+1,rest,max-1);
	dest[max-1]='\0' ;
}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void toppage(FILE *fo) {
    perhapsreread();
            
    http_begin(fo,200,"OK");
    http_data(fo,"text/html");
    fprintf(fo,"<html><head><title>C-Mix/II annotation viewer - %s\n"
	    "</title></head>\n"
	    "<frameset rows=\"4*,*\">\n"
	    " <frame src=\"/%d\" name=\"program\">\n"
	    " <frame src=\"/%d\" name=\"anno\">\n"
	    " <noframes>\n"
	    "  <h1>There aren't any frames here!</h1>\n"
	    "  You need to have a frames-capable browser to use\n"
	    "  cmixshow with option -s.\n"
	    " </noframes>\n"
	    "</frameset>\n"
	    "</html>\n",
	       getfilename(),THE_PROGRAM,NULL_PAGE );
}

METHOD( Null_Page ) {
    http_begin(fo,200,"OK");
    http_data(fo,"text/html");
    fprintf(fo,"<html><html>\n");
}

static void showbigwin(FILE *fo,struct AnnoTree *tree,magic label) {
    http_begin(fo,200,"OK");
    http_data(fo,"text/html");
    fprintf(fo,"<html><head><base target=anno></head><body>\n");
    if ( tree == GetMainTree() )
        fprintf(fo,"<form action=\"/%d/\" method=GET target=_top>"
                   "<input type=submit value=\"Select visible annotations\">"
                   "</form>\n",CUTSELECTOR);
    else
        fprintf(fo,"<i><a href=\"/%d\" target=program>(show program text)"
                   "</a></i><hr>\n",THE_PROGRAM);
    PrettyPrint(fo,tree, label);
    fprintf(fo,"</body></html>\n");
}

METHOD( The_Program ) {
    showbigwin(fo,GetMainTree(),0);
}

METHOD( AnnoSingle ) {
    if ( nrints != 3 )
        badurl(fo,ints,nrints,rest);
    else
        showbigwin(fo,FollowLabel(ints[1],ints[2]),ints[2]);
}

METHOD( AnnoMulti ) {
    int j ;
    int treshold = 40 / (nrints+1) ;
    if ( treshold < 4 ) treshold = 3 ;
    http_begin(fo,200,"OK");
    http_data(fo,"text/html");
    fprintf(fo,"<html><head><base target=anno></head><body><ul>\n");
    for(j=1; j<nrints-1; j+=2 ) {
        magic type = ints[j];
        magic label = ints[j+1] ;
        struct AnnoTree *tree = FollowLabel(type,label);
        fprintf(fo,"<li><b>%s</b>",typerec(type)->Name);
        if ( GetPrettyHeight(tree) > treshold ) {
            fprintf(fo,": <i><a target=program href=\""
                       "/%d/"PCT_MAGIC"/"PCT_MAGIC"#a\">(long text)</a></i>\n",
                    ANNOSINGLE,type,label);
        } else {
            fprintf(fo,"<br>\n");
            PrettyPrint(fo,tree,0);
        }
    }
    fprintf(fo,"</ul></body></html>\n");
}

METHOD( CutSelector ) {
    char buffer[20];
    perhapsreread();
    
    http_begin(fo,200,"OK");
    http_data(fo,"text/html");
    fprintf(fo,"<html><head><title>C-Mix/II: Annotation selector - %s</title>"
               "\n</head><body><h1>Annotation selector</h1>\n",
               getfilename());
    sprintf(buffer,"/%d",CUTACTION);
    DisplayCut(fo,buffer);
    fprintf(fo,"<form action=\"/\" method=GET>\n"
               "<input type=submit value=\"Ok, show me those!\"></form>\n"
               "</body></html>\n");
}

METHOD( CutAction ) {
    if ( nrints != 3 )
        badurl(fo,ints,nrints,rest);
    else {
        CutOperation(ints[1],ints[2]);
        redirect(fo,"%d/",CUTSELECTOR);
    }
}

