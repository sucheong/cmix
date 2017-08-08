/* -*-fundamental-*-
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  ANSI C lexer: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

%{
    #include <stdarg.h>
    #include <stdlib.h>
    #include <strstream.h>
    #include "errno.h"
    #include "auxilary.h"
    #include "fileops.h"
    #include "parser.h"
    #include "gram-tab.h"
    #include "symboltable.h"
    #include "diagnostic.h"
    #include "options.h"
    #include "directives.h"

    #define yylval ccclval
    
    extern Scope<ObjectDecl> names;

    static int typedef_name(char *str);

%}

%option noyywrap
%option nounput
%option prefix="ccc"

D			[0-9]
O			[0-7]
A			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
F			(f|F)
U                       (u|U)
L                       (l|L)
W                       [ \t\v\f\r]+

%%

"auto"			{ return (AUTO); }
"break"			{ return (BREAK); }
"case"			{ return (CASE); }
"char"			{ return (CHAR); }
"const"			{ return (CONST); }
"continue"		{ return (CONTINUE); }
"default"		{ return (DEFAULT); }
"do"			{ return (DO); }
"double"		{ return (DOUBLE);}
"else"			{ return (ELSE); }
"enum"			{ return (ENUM); }
"extern"		{ return (EXTERN); }
"float"			{ return (FLOAT);}
"for"			{ return (FOR); }
"goto"			{ return (GOTO); }
"if"			{ return (IF); }
"int"			{ return (INT); }
"long"			{ return (LONG); }
"register"		{ return (REGISTER); }
"return"		{ return (RETURN); }
"short"			{ return (SHORT); }
"signed"		{ return (SIGNED); }
"sizeof"		{ return (SIZEOF); }
"static"		{ return (STATIC); }
"struct"		{ return (STRUCT); }
"switch"		{ return (SWITCH); }
"typedef"		{ return (TYPEDEF); }
"union"			{ return (UNION); }
"unsigned"		{ return (UNSIGNED); }
"void"			{ return (VOID); }
"volatile"		{ return (VOLATILE); }
"while"			{ return (WHILE); }

{A}({A}|{D})*		{ yylval.str = strdup(yytext);
                          if (typedef_name(yytext))
			      return (TYPEDEFname);
                          else
			      return (IDENTIFIER); 
		 	}
"__CMIX"{W}*"("[^()]*   { char const *cp = yytext+6 ;
                          while(*cp != '(') cp++ ;
                          yylval.str = strdup(cp+1);
                          return CMIXTAG;
                        }
(0[xX]{H}+|0{O}+|{D}+)	{ yylval.str = strdup(yytext); return (INTEGERconstant); }

(0[xX]{H}+{U}|0{O}+{U}|{D}+{U})  { yylval.str = strdup(yytext); return (UINTEGERconstant); }

(0[xX]{H}+{L}|0{O}+{L}|{D}+{L})   { yylval.str = strdup(yytext); return (LONGconstant); }

(0[xX]{H}+({U}{L}|{L}{U})|0{O}+({U}{L}|{L}{U})|{D}+({U}{L}|{L}{U}))  { yylval.str = strdup(yytext); return (ULONGconstant); }

'(\\.|[^\\'])+'		{ yylval.str = strdup(yytext); return (CHARACTERconstant); }

({D}+{E}|{D}*"."{D}+({E})?|{D}+"."{D}*({E})?){F}	{ yylval.str = strdup(yytext); return (FLOATINGconstant); }

({D}+{E}|{D}*"."{D}+({E})?|{D}+"."{D}*({E})?)	{ yylval.str = strdup(yytext); return (DOUBLEconstant); }

\"(\\.|[^\\"])*\"	{ yylval.str = strdup(yytext); return (STRINGliteral); }

"..."			{ return (ELLIPSIS); }
">>="			{ return (RSassign); }
"<<="			{ return (LSassign); }
"+="			{ return (PLUSassign); }
"-="			{ return (MINUSassign); }
"*="			{ return (MULTassign); }
"/="			{ return (DIVassign); }
"%="			{ return (MODassign); }
"&="			{ return (ANDassign); }
"^="			{ return (ERassign); }
"|="			{ return (ORassign); }
">>"			{ return (RIGHTSHIFT); }
"<<"			{ return (LEFTSHIFT); }
"++"			{ return (ICR); }
"--"			{ return (DECR); }
"->"			{ return (ARROW); }
"&&"			{ return (ANDAND); }
"||"			{ return (OROR); }
"<="			{ return (LESSEQUAL); }
">="			{ return (GREATEQUAL); }
"=="			{ return (EQUAL); }
"!="			{ return (NOTEQUAL); }
";"			{ return(';'); }
"{"			{ return('{'); }
"}"			{ return('}'); }
","			{ return(','); }
":"			{ return(':'); }
"="			{ return('='); }
"("			{ return('('); }
")"			{ return(')'); }
"["			{ return('['); }
"]"			{ return(']'); }
"."			{ return('.'); }
"&"			{ return('&'); }
"!"			{ return('!'); }
"~"			{ return('~'); }
"-"			{ return('-'); }
"+"			{ return('+'); }
"*"			{ return('*'); }
"/"			{ return('/'); }
"%"			{ return('%'); }
"<"			{ return('<'); }
">"			{ return('>'); }
"^"			{ return('^'); }
"|"			{ return('|'); }
"?"			{ return('?'); }

"\n"			{ ccchere++; }
{W}     		{  }
^(#" "{D}+" "\".*\").*"\n"  {
    			  if (strlen(yytext) > 1024) {
                              Diagnostic(WARNING,ccchere)
                                 << "truncating filename (> 1024)";
			      yytext[1024] = 0;
			  }
                          {
                              int newlineno ;
                              char newfilename[1024] ;
    			      sscanf(yytext, "# %d \"%[^\"]",
                                   &newlineno, newfilename);
                              ccchere = newfilename ;
			      ccchere = newlineno ;
                          }
		        }
^(#" "{D}+).*"\n"       { ccchere = atoi(yytext+2); }
^{W}?#pragma{W}cmix{W}.* {{
                            const char *pc = yytext ;
                            while ( *pc != 'x' ) pc++ ;
                            JamInDirective(pc+1,ccchere) ;
                        }}
^{W}?#pragma.*          {{  const char *pc = yytext ;
                            while ( *pc != '#' ) pc++ ;
                            Diagnostic(WARNING,ccchere) << "ignoring " << pc ;
                        }}
^{W}?#error.*           {{  const char *pc = yytext ;
                            while ( *pc != 'o' ) pc++ ;
                            pc += 2 ;
                            while ( *pc == ' ' || *pc == '\t' ) pc++ ;
                            Diagnostic(ERROR,ccchere) << pc;
                        }}
.			{ Diagnostic(ERROR,ccchere) << "bad character '"
                                                    << (char)yytext[0] << '\'';
                        }

%%

static int typedef_name(char *str) {
    const ObjectDecl* t = names.lookup(str);
    if (t) {
        if (t->linkage == Typedef || t->linkage == AbstractType )
            return 1;
    }
    return 0;
}

LexerTracker ccchere;

int cccLexWrapper::InUse = 0 ;

void cccLexWrapper::Entry(Position &pos) {
    if ( InUse++ )
        Diagnostic(INTERNAL,pos) << "tried to reenter C scanner";
}

#define MAXCMD 2048

cccLexWrapper::cccLexWrapper
       (const char *filename,const char*cppargs,Position ref) :ok(0),f(NULL) {
    Entry(ref);

    // Check that the file exists.
    if (!CheckFile(filename)) {
        Diagnostic(ERROR,ref) << filename << " doesn't exist";
        return ;
    }

    // Make an executable command consisting of the C preprocessor
    // called with user definitions, include directories, and the set
    // of C files.
    char cmd[MAXCMD];
    ostrstream ost(cmd,MAXCMD);
    if ( getenv("CPP") )
        ost << getenv("CPP") ;
    else
        ost << CPP ;
    ost << " " << cppargs
        << " -D__CMIX=__CMIX -D__STDC__ -I" ;
    if ( getenv("CMIX_SHADOW_DIR") )
	ost << getenv("CMIX_SHADOW_DIR") ;
    else
        ost << CMIX_SHADOW_DIR ;
    ost << ' ' << filename << ends;
    if (ost.fail()) {
        Diagnostic(ERROR,ref)
            << "CPP command too large. Enlarge MAXCMD in parser.cc";
        return ;
    }

    if ( trace_c_parser )
	cerr << cmd << endl ;
    // Use the C preprocessor as a filter. Its output is passed to the lexer.
    f = popen(cmd, "r");
    if (f == NULL) {
        Diagnostic d(ERROR,ref);
        d << "failed to spawn preprocessor with popen()";
	d.addline() << strerror(errno) ;
        d.addline() << "command was: " << cmd ;
        return ;
    }
    if ( only_preprocess_mode ) {
      int c ;
      cout << "/* C-MIX/II PREPROCESSED: " << filename << " */\n" ;
      while( (c=getc(f)) != EOF )
          cout << (unsigned char)c;
    } else  {
        yy_switch_to_buffer(yy_create_buffer(f,YY_BUF_SIZE)) ;
        ccchere = filename ;
        ccchere = 1 ;
        ok = 1 ;
    }
}

void cccLexWrapper::parse() {
    if ( ok )
        parser.ParseNow();
}

cccLexWrapper::~cccLexWrapper() {
  if ( ok )
    yy_delete_buffer(YY_CURRENT_BUFFER);
  if ( f )
    if ( pclose(f) != 0 )
      Diagnostic(ERROR,Position()) << "preprocessor failed" ;
  InUse-- ;
}
