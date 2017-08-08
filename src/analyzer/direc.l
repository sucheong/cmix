/* -*- Fundamental -*-
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Lexer for specification files
 *
 * Copyright © 1997-1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

%{
    #include <stdarg.h>
    #include <stdio.h>
    #include "auxilary.h"
    #include "diagnostic.h"
    #include "directives.h"
    #include "direc-tab.h"
    #define yylval cmxlval

%}

%option noyywrap
%option nounput
%option prefix="cmx"

id	[a-zA-Z_][a-zA-Z0-9_]*
space	[ \t\v\r\f]
D	[0-9]
W	{space}*

%s STANDARD WORD_MODE

%%

<INITIAL>"define"	BEGIN(WORD_MODE);	return DEFINE;
<INITIAL>"source"	BEGIN(WORD_MODE);	return SOURCE;
<INITIAL>"outputbase"   BEGIN(WORD_MODE);	return OUTPUTBASE;
<INITIAL>"header"	BEGIN(WORD_MODE);	return HEADER;
<INITIAL>"spectime"				return SPECTIME;
<INITIAL>"residual"				return RESIDUAL;
<INITIAL>"anytime"				return ANYTIME;
<INITIAL>"dangerous"				return DANGEROUS;
<INITIAL>"pure"					return PURE;
<INITIAL>"stateless"				return STATELESS;
<INITIAL>"rostate"				return ROSTATE;
<INITIAL>"rwstate"				return RWSTATE;
<INITIAL>"visible"				return VISIBLE;
<INITIAL>"well-known"				return WELLKNOWN;
<INITIAL>"constant"				return CONSTANT;
<INITIAL>"generator"				return GENERATOR;
<INITIAL>"goal"					return GOAL;
<INITIAL>"taboo"				return TABOO;
<INITIAL>"debug"				return DEBUG;
<INITIAL>"unsigned"				return UNSIGNED;
<INITIAL>"chars"				return CHARS;
<INITIAL>"are"					return ARE;
<INITIAL>"glyphs"				return GLYPHS;
<INITIAL>"lift"					return LIFT;
<INITIAL>"long"					return LONG;
<INITIAL>"double"				return DOUBLE;
<STANDARD>"specializes"				return SPECIALIZES;
<STANDARD>"producing"				return PRODUCING;
<STANDARD>"at"					return AT ;
<STANDARD>"most"				return MOST ;
"::"						return COLONCOLON;
<INITIAL>":"		BEGIN(STANDARD);	return ':';
";"			BEGIN(INITIAL);		return ';';
^{W}#{W}pragma{space}{W}cmix{space}	BEGIN(INITIAL);	return ';';

<WORD_MODE>[^ \t\n:;][^ \t\n;]*	yylval.str = stringDup(yytext); return WORD;
				
-?{D}+			yylval.str = stringDup(yytext); return INT;
{id}			yylval.str = stringDup(yytext); return IDENT;

"$"{D}+			yylval.numeric = atoi(yytext +1); return DOLLAR;

<STANDARD>\"(\\.|[^\\\n\"])*\"  yylval.str = stringDup(yytext); return STRING;

"\n"{space}*"\n"	cmxhere+=2; BEGIN(INITIAL); return ';';
"@"[^\n]*"\n"		cmxhere++; /* at sign starts a comment */
"\n"			cmxhere++;
{space}*		{  }
.			return yytext[0] ;

%%

LexerTracker cmxhere;
char const *cmx_jammedin ;
int cmx_yyerror_called ;

/* class cmxLexWrapper is defined in directives.h */
int cmxLexWrapper::InUse = 0 ;

void cmxLexWrapper::Entry(Position &pos) {
    if ( InUse++ )
        Diagnostic(INTERNAL,pos) << "tried to reenter CMX scanner";
}

cmxLexWrapper::cmxLexWrapper(const char *filename,Position ref) : ok(1) {
    Entry(ref);
    f = fopen(filename,"rt");
    if ( f == NULL ) {
        Diagnostic(ERROR,ref) << "can't open file " << filename ;
        ok = 0;
        return ;
    }
    yy_switch_to_buffer(yy_create_buffer(f,YY_BUF_SIZE)) ;
    cmx_jammedin = NULL ;
    cmxhere = filename ;
    cmxhere = 1 ;
}

cmxLexWrapper::cmxLexWrapper(const char *buffer, size_t length, Position pos)
    : ok(1), f(NULL)
{
    Entry(pos);
    cmx_jammedin = buffer ;
    yy_scan_bytes(buffer,length);
    cmxhere = pos ;
}

int cmxLexWrapper::parse() {
    int cmxparse();

    BEGIN(INITIAL);
    if ( !ok )
	return 0 ;
    cmx_yyerror_called = 0 ;
    return !cmxparse() && !cmx_yyerror_called ;
}

cmxLexWrapper::~cmxLexWrapper() {
    if ( ok )
        yy_delete_buffer(YY_CURRENT_BUFFER);
    if ( f )
        fclose(f);
    InUse-- ;
}

    

