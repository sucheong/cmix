/* Authors:  Jens Peter Secher (jpsecher@diku.dk)          -*-fundamental-*-
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: lexer.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

%{
    #include <stdlib.h>
    #include "annofront.h"
    #include "y-tab.h"
%}

%option noyywrap
%option nounput

D			[0-9]
W                       [ \t\v\f\n]+

%%

"Producer"	       { return (PRODUCER); }
"Types"                { return (TYPES); }
"Output"               { return (OUTPUT); }
"Attributes"           { return (ATTRIBUTES); }

{D}+	               { yylval.num = strtoul(yytext,NULL,10);
                         return (NUMBER); }

{W}+                   { }

\"(\\.|[^\\"])+\"      { /* Cut off the pings, translate escapes */
                           char *src = stringDup(yytext+1);
                           char *dst = src ;
                           yylval.str = dst ;
                           while(*src != '"') {
                               if (*src == '\\') src++ ;
                               *dst++ = *src++ ;
                           } ;
                           *dst = '\0' ;
                           return (STRING);
                       }
\"\"		       { yylval.str = "" ; return STRING; }

. return yytext[0];

%%

