/* Author:   Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: HTTP protocol helper module
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

		
#include <stdio.h>

void http_begin(FILE *fo,int code,char *explanation);
void http_data(FILE *fo,char *type) ;
void http_nodata(FILE *fo) ;

void http_service(FILE *fi,FILE *fo) ;
void http_get(char *path,FILE *fo) ;

void http_expanded(unsigned*ints,unsigned nrints,char *rest,FILE *fo);


extern char URLbase[] ;
		
