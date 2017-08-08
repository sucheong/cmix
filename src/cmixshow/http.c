/* Author:   Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: HTTP protocol code
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <assert.h>
#include "http.h"
#include <string.h>
#include <ctype.h>

#define METHODLEN 10
#define PATHLEN 100
#define NRINTS 20

static int getrequestword(FILE *fi,char *buffer,unsigned buflen) {
	unsigned i ;
	int c ;

	do {
		c = getc(fi) ;
		if ( EOF == c )
			return EOF ;
	} while ( c == ' ' ) ;

	for ( i = 0 ; i < buflen-1 ; ) switch(c) {
	  case ' ':
	  case '\n':
	  case '\r':
	  case EOF:
		buffer[i]='\0' ;
		return c ;
	  default:
		buffer[i++]=c ;
		c = getc(fi) ;
	}
	/* string too long for buffer; terminate string */
	buffer[i]='\0' ;
	return c ;
}
	

void http_begin(FILE *fo,int code,char *explanation) {
	fprintf(fo,"HTTP/1.0 %03d %s\r\n",code,explanation);
}

void http_data(FILE *fo,char *type) {
	fprintf(fo,"Content-Type: %s\r\n\r\n",type);
}

void http_nodata(FILE *fo) {
	fprintf(fo,"\r\n");
}

void http_service(FILE *fi,FILE *fo) {
	char method[METHODLEN] ;
	char path[PATHLEN] ;
	char *src,*dst ;

	int delimiter ;

	int c ;
	int atbeginning ;
	unsigned u ;

	delimiter = getrequestword(fi,method,METHODLEN);
	if ( delimiter != ' ' ) /* This is not a HTTP request, abort */
		return ;

	delimiter = getrequestword(fi,path,PATHLEN);

	if (delimiter == EOF)
		return ; /* ??? */

	/* Handle the case where the path is too long: read along until
	 * we know if this is a HTTP/0.9 or HTTP/1.x request
	 */
	if ( delimiter != '\r' && delimiter != '\n' && delimiter != ' ' ) {
		do {
			delimiter = getc(fi) ;
			if ( delimiter == EOF )
				return ; /* ??? */
		} while ( delimiter != '\n' && delimiter != ' ' ) ;
		if ( delimiter == ' ' )
			delimiter = '*' ;
	}

	if (delimiter == '\r' || delimiter == '\n') {
		/* This is not HTTP/1.x. Refuse politely. */
		fprintf(fo,"<html><h1>Not supported</h1>Sorry, you need\r\n"
		           "a user agent speaking HTTP/1.x to use this\r\n"
		           "interface.</html>\r\n");
		return ;
	}

	/* read and ignore the rest of the request
	 * note: the HTTP spec advises us to try to also accept requests
	 * with missing CR's in line endings. */
	for ( atbeginning = 0 ;;) {
		c = getc(fi) ;
		if ( c == EOF )
			return ; /* Hey, where did he go? */
		if ( c == '\r' )
			continue ;
		if ( c != '\n' )
			atbeginning = 0 ;
		else if ( !atbeginning )
			atbeginning = 1 ;
		else break ;
	}

	if ( 0 != strcmp(method,"GET") ) {
		http_begin(fo,502,"We only do GET requests");
		http_data(fo,"text/html");
		fprintf(fo,"<html><h1>Service not supported</h1>\n"
		          "This pseudoserver does not know the\n"
			  "%s method.\n</html>\n",method);
		return ;
	}

	if ( delimiter != ' ' ) {
		http_begin(fo,500,"URL too long");
		http_data(fo,"text/html");
		fprintf(fo,"<html><h1>This shouldn't happen!</h1>\n"
			"The URL requested was too long for the URL buffer "
			"to hold.\n<html>\n");
		return ;
	}
	
	/* translate the URL encoding */
	for ( src = dst = path ; *src ; dst++ )
	  if ( src[0] == '%' && isxdigit(src[1]) && isxdigit(src[2]) ) {
		src[0]=src[1];
		src[1]=src[2];
		src[2]='\0';
		sscanf(src,"%x",&u) ;
		*dst = u ;
		src += 3 ;
	  } else
		*dst = *(src++) ;

	/* hand the translated URL to the data-producing backend */
	http_get(path,fo) ;
}
	
void http_get(char *path,FILE *fo) {
	unsigned intarray[NRINTS] ;
	int ip ;

	if ( *path != '/' ) {
		http_begin(fo,501,"This is not a proxy server");
		http_data(fo,"text/html");
		fprintf(fo,"<html>Either the URL was malformed or you're\n"
		   "trying to use this as a proxy server.</html>");
		return ;
	}

	ip = -1 ;
	while(1)
  	  if ( *path == '/' ) {
		ip++ ;
		path++ ;
		if ( !isdigit(*path) || ip == NRINTS ) {
			if ( *path == '/' ) path++ ;
			http_expanded(intarray,ip,path,fo);
			return ;
		}
		intarray[ip] = 0 ;
	  } else
	  if ( isdigit(*path) )
		intarray[ip] = 10 * intarray[ip] + ( *(path++) - '0' ) ;
	  else
	  if ( *path == '\0' ) {
	  	http_expanded(intarray,ip+1,"",fo) ;
		return ;
	  } else {
		http_begin(fo,400,"Malformed URL");
		http_data(fo,"text/html");
		fprintf(fo,"<html>The URL given does not look like one I "
		   "understand. There must be an error somewhere.</html>\n");
		return ;
	  }
}

		
