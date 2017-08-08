
/*  A Bison parser, made from direc.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse cmxparse
#define yylex cmxlex
#define yyerror cmxerror
#define yylval cmxlval
#define yychar cmxchar
#define yydebug cmxdebug
#define yynerrs cmxnerrs
#define	DEFINE	258
#define	SOURCE	259
#define	GENERATOR	260
#define	OUTPUTBASE	261
#define	HEADER	262
#define	SPECTIME	263
#define	RESIDUAL	264
#define	VISIBLE	265
#define	DANGEROUS	266
#define	ANYTIME	267
#define	PURE	268
#define	STATELESS	269
#define	ROSTATE	270
#define	RWSTATE	271
#define	WELLKNOWN	272
#define	CONSTANT	273
#define	TABOO	274
#define	SPECIALIZES	275
#define	PRODUCING	276
#define	GOAL	277
#define	COLONCOLON	278
#define	DOLLAR	279
#define	DEBUG	280
#define	AT	281
#define	MOST	282
#define	UNSIGNED	283
#define	CHARS	284
#define	ARE	285
#define	GLYPHS	286
#define	LIFT	287
#define	LONG	288
#define	DOUBLE	289
#define	WORD	290
#define	INT	291
#define	IDENT	292
#define	STRING	293

#line 10 "direc.y"


#include <cmixconf.h>
// magic incantation from the autoconf manual
#ifdef __GNUC__
#  define alloca __builtin_alloca
#else
#  if HAVE_ALLOCA_H
#    include<alloca.h>
#  else
#    ifdef _AIX
#      pragma alloca
#    else
#      ifndef alloca /* predefined by HP cc +Olibcalls */
         char *alloca();
#      endif
#    endif
#  endif
#endif

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_REGEX_H
#include <regex.h>  // for OSF alphas with buggy GCC installations
#endif
#include <ctype.h>
#include "auxilary.h"
#include "diagnostic.h"
#include "directives.h"
#include "options.h"
#include "fileops.h"

    extern char const *cmx_jammedin ;
    static void yyerror(const char*message) {
	cmx_yyerror_called = 1 ;
        Diagnostic d(ERROR,cmxhere) ;
	d << message;
	if ( cmx_jammedin )
	  d.addline() << "failed pragma was: " << cmx_jammedin ;
    }

#define YYERROR_VERBOSE 1

#define YYDEBUG 1

#ifdef YYDEBUG
    static void settheflag();
#else
    #define settheflag()
#endif


#line 60 "direc.y"
typedef union{
    char* str;
    int numeric;
    AnnoAtom *annotation ;
    GeneratorDirective* generator ;
    Plist<GeneratorDirective::Param>* params ;
    GeneratorDirective::Param* param ;
    PositionInUnion pos;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		126
#define	YYFLAG		-32768
#define	YYNTBASE	45

#define YYTRANSLATE(x) ((unsigned)(x) <= 293 ? yytranslate[x] : 72)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    41,
    42,     2,     2,    43,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    40,    39,     2,
     2,     2,    44,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     6,     7,     9,    11,    14,    16,    19,
    21,    23,    28,    32,    34,    38,    41,    43,    45,    49,
    52,    54,    56,    59,    64,    69,    72,    75,    77,    79,
    81,    83,    85,    87,    89,    91,    94,    97,   100,   103,
   105,   107,   109,   112,   115,   122,   127,   133,   134,   136,
   138,   142,   144,   146,   148,   150,   157,   161,   165,   167,
   168,   172,   176,   181,   183,   187,   190,   192,   196,   199,
   209,   212,   213,   215,   217,   219,   221,   223
};

static const short yyrhs[] = {    -1,
    46,    47,     0,    48,     0,     0,    60,     0,    50,     0,
    49,    50,     0,    49,     0,    48,    39,     0,    39,     0,
     1,     0,    28,    29,    30,    31,     0,    32,    33,    34,
     0,    51,     0,    52,    71,    35,     0,     3,    40,     0,
    51,     0,    53,     0,    54,    71,    35,     0,     4,    40,
     0,    53,     0,    55,     0,    56,    70,     0,    56,    70,
    23,    70,     0,    56,    70,    41,    42,     0,    56,    43,
     0,    60,    40,     0,    55,     0,    13,     0,    14,     0,
    15,     0,    16,     0,     8,     0,     9,     0,    12,     0,
    10,     8,     0,    10,     9,     0,    11,     8,     0,    17,
    18,     0,    58,     0,    57,     0,    59,     0,    58,    57,
     0,    57,    58,     0,     5,    40,    71,    70,    20,    62,
     0,    22,    40,    71,    62,     0,    70,    71,    41,    63,
    42,     0,     0,    64,     0,    65,     0,    64,    43,    65,
     0,    44,     0,    24,     0,    36,     0,    38,     0,    61,
    21,    41,    38,    66,    42,     0,    61,    21,    38,     0,
    61,    21,    70,     0,    61,     0,     0,    66,    43,    24,
     0,     6,    40,    35,     0,     7,    40,    71,    35,     0,
    67,     0,    17,    40,    70,     0,    67,    70,     0,    68,
     0,    19,    40,    70,     0,    68,    70,     0,    25,    40,
    71,    70,    69,    21,    26,    27,    36,     0,    41,    42,
     0,     0,    37,     0,    20,     0,    21,     0,    26,     0,
    27,     0,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   104,   106,   107,   108,   109,   112,   113,   114,   117,   118,
   121,   125,   127,   132,   135,   141,   142,   147,   150,   153,
   154,   159,   162,   166,   170,   174,   177,   178,   181,   182,
   183,   184,   187,   188,   189,   192,   193,   194,   195,   198,
   199,   200,   201,   202,   207,   215,   222,   229,   230,   233,
   234,   241,   243,   245,   247,   251,   257,   263,   273,   286,
   287,   297,   306,   336,   339,   340,   344,   347,   348,   352,
   356,   356,   361,   362,   363,   364,   365,   367
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","DEFINE",
"SOURCE","GENERATOR","OUTPUTBASE","HEADER","SPECTIME","RESIDUAL","VISIBLE","DANGEROUS",
"ANYTIME","PURE","STATELESS","ROSTATE","RWSTATE","WELLKNOWN","CONSTANT","TABOO",
"SPECIALIZES","PRODUCING","GOAL","COLONCOLON","DOLLAR","DEBUG","AT","MOST","UNSIGNED",
"CHARS","ARE","GLYPHS","LIFT","LONG","DOUBLE","WORD","INT","IDENT","STRING",
"';'","':'","'('","')'","','","'?'","flag_setter","@1","grammar_dispatch","specification_file",
"specification_semi","directive","define_directive","define_start","source_directive",
"source_start","anno_directive","anno_start","uanno_state","uanno_funtime","uanno_vartime",
"user_annotation","generator_directive","goal","goal_parameters","goal_plist",
"goal_parm","name_plist","wellknown_directive","taboo_directive","opt_parens",
"identifier","marker", NULL
};
#endif

static const short yyr1[] = {     0,
    46,    45,    47,    47,    47,    48,    48,    48,    49,    49,
    50,    50,    50,    50,    51,    52,    52,    50,    53,    54,
    54,    50,    55,    55,    55,    55,    56,    56,    57,    57,
    57,    57,    58,    58,    58,    59,    59,    59,    59,    60,
    60,    60,    60,    60,    61,    61,    62,    63,    63,    64,
    64,    65,    65,    65,    65,    50,    50,    50,    50,    66,
    66,    50,    50,    50,    67,    67,    50,    68,    68,    50,
    69,    69,    70,    70,    70,    70,    70,    71
};

static const short yyr2[] = {     0,
     0,     2,     1,     0,     1,     1,     2,     1,     2,     1,
     1,     4,     3,     1,     3,     2,     1,     1,     3,     2,
     1,     1,     2,     4,     4,     2,     2,     1,     1,     1,
     1,     1,     1,     1,     1,     2,     2,     2,     2,     1,
     1,     1,     2,     2,     6,     4,     5,     0,     1,     1,
     3,     1,     1,     1,     1,     6,     3,     3,     1,     0,
     3,     3,     4,     1,     3,     2,     1,     3,     2,     9,
     2,     0,     1,     1,     1,     1,     1,     0
};

static const short yydefact[] = {     1,
     0,    11,     0,     0,     0,     0,     0,    33,    34,     0,
     0,    35,    29,    30,    31,    32,     0,     0,     0,     0,
     0,     0,    10,     2,     3,     0,     6,    14,    78,    18,
    78,    28,     0,    41,    40,    42,     5,    59,    64,    67,
    16,    20,    78,     0,    78,    36,    37,    38,    39,     0,
     0,    78,    78,     0,     0,     9,     7,     0,     0,     0,
    74,    75,    76,    77,    73,    26,    23,    44,    43,    27,
     0,    66,    69,     0,    62,     0,    65,    68,     0,     0,
     0,    13,    15,    19,     0,     0,    57,     0,    58,     0,
    63,    46,    78,    72,    12,    24,    25,    60,     0,     0,
     0,     0,     0,    45,    48,    71,     0,    56,     0,    53,
    54,    55,    52,     0,    49,    50,     0,    61,    47,     0,
     0,    51,    70,     0,     0,     0
};

static const short yydefgoto[] = {   124,
     1,    24,    25,    26,    27,    28,    29,    30,    31,    32,
    33,    34,    35,    36,    37,    38,    92,   114,   115,   116,
   103,    39,    40,   102,    93,    59
};

static const short yypact[] = {-32768,
    60,-32768,   -34,   -29,   -23,   -15,    -8,-32768,-32768,     7,
    15,-32768,-32768,-32768,-32768,-32768,   -13,    -7,    -3,     3,
     1,    14,-32768,-32768,    13,    97,-32768,     5,-32768,    20,
-32768,     2,   -17,    42,    43,-32768,    44,    57,   100,   100,
-32768,-32768,-32768,    48,-32768,-32768,-32768,-32768,-32768,   100,
   100,-32768,-32768,    56,    53,-32768,-32768,    44,    54,    55,
-32768,-32768,-32768,-32768,-32768,-32768,   -10,-32768,-32768,-32768,
     8,-32768,-32768,   100,-32768,    58,-32768,-32768,   100,   100,
    63,-32768,-32768,-32768,   100,    49,-32768,    77,-32768,    75,
-32768,-32768,-32768,    76,-32768,-32768,-32768,-32768,   100,    82,
    86,   103,    38,-32768,     0,-32768,    70,-32768,    94,-32768,
-32768,-32768,-32768,    88,    89,-32768,   104,-32768,-32768,     0,
    98,-32768,-32768,   133,   135,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,   112,-32768,-32768,-32768,-32768,-32768,
-32768,   105,   107,-32768,   113,-32768,    45,-32768,-32768,    22,
-32768,-32768,-32768,-32768,   -32,   -31
};


#define	YYLAST		144


static const short yytable[] = {    60,
    67,   -22,    61,    62,    49,    41,    72,    73,    63,    64,
    42,    74,    85,    76,    46,    47,    43,    77,    78,    65,
    79,    80,    48,   110,    44,    66,    50,    61,    62,    54,
    86,    45,    51,    63,    64,   111,    52,   112,    89,   -17,
   -22,    90,    53,   113,    65,    87,    55,    94,    88,     8,
     9,    56,    96,    12,   -21,    13,    14,    15,    16,    -4,
     2,   100,     3,     4,     5,     6,     7,     8,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    71,    18,   108,
   109,    19,    75,    70,    20,    81,    82,    21,    83,    84,
    97,    22,    91,    95,    99,   117,    -8,     2,    23,     3,
     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
    14,    15,    16,    17,    98,    18,   101,   118,    19,    61,
    62,    20,   105,   107,    21,    63,    64,   106,    22,   119,
   121,   120,   125,   123,   126,    -8,    65,    57,    58,    69,
    68,   122,     0,   104
};

static const short yycheck[] = {    31,
    33,     0,    20,    21,    18,    40,    39,    40,    26,    27,
    40,    43,    23,    45,     8,     9,    40,    50,    51,    37,
    52,    53,     8,    24,    40,    43,    40,    20,    21,    29,
    41,    40,    40,    26,    27,    36,    40,    38,    71,    35,
    39,    74,    40,    44,    37,    38,    33,    80,    41,     8,
     9,    39,    85,    12,    35,    13,    14,    15,    16,     0,
     1,    93,     3,     4,     5,     6,     7,     8,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    21,    19,    42,
    43,    22,    35,    40,    25,    30,    34,    28,    35,    35,
    42,    32,    35,    31,    20,    26,     0,     1,    39,     3,
     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
    14,    15,    16,    17,    38,    19,    41,    24,    22,    20,
    21,    25,    41,    21,    28,    26,    27,    42,    32,    42,
    27,    43,     0,    36,     0,    39,    37,    26,    26,    35,
    34,   120,    -1,    99
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/local/store/share/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "/usr/local/store/share/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 104 "direc.y"
{ settheflag(); ;
    break;}
case 3:
#line 107 "direc.y"
{ TheCmxResult = CmxResult(); ;
    break;}
case 4:
#line 108 "direc.y"
{ TheCmxResult = CmxResult(); ;
    break;}
case 5:
#line 109 "direc.y"
{ TheCmxResult = yyvsp[0].annotation; ;
    break;}
case 12:
#line 125 "direc.y"
{ lift_uchar_as_char = true; ;
    break;}
case 13:
#line 127 "direc.y"
{ lift_long_double = true; ;
    break;}
case 15:
#line 135 "direc.y"
{
		directives.add_cpparg('D',yyvsp[0].str);
                free(yyvsp[0].str);
	  ;
    break;}
case 19:
#line 150 "direc.y"
{ directives.add_source(yyvsp[0].str,yyvsp[-1].pos); ;
    break;}
case 22:
#line 159 "direc.y"
{;
    break;}
case 23:
#line 162 "direc.y"
{
		directives.add_globalanno(yyvsp[-1].annotation,yyvsp[0].str) ;
		yyval.annotation = yyvsp[-1].annotation ;
	  ;
    break;}
case 24:
#line 166 "direc.y"
{
		directives.add_localanno(yyvsp[-3].annotation,yyvsp[-2].str,yyvsp[0].str) ;
		yyval.annotation = yyvsp[-3].annotation ;
	  ;
    break;}
case 25:
#line 170 "direc.y"
{
		directives.add_funanno(yyvsp[-3].annotation,yyvsp[-2].str);
		yyval.annotation = yyvsp[-3].annotation ;
	  ;
    break;}
case 26:
#line 174 "direc.y"
{ yyval.annotation = yyvsp[-1].annotation ;
    break;}
case 27:
#line 177 "direc.y"
{ yyval.annotation = yyvsp[-1].annotation ; ;
    break;}
case 28:
#line 178 "direc.y"
{ yyval.annotation = yyvsp[0].annotation ; ;
    break;}
case 29:
#line 181 "direc.y"
{ yyval.annotation=new StateAnnoAtom(cmxhere,SEPure); ;
    break;}
case 30:
#line 182 "direc.y"
{ yyval.annotation=new StateAnnoAtom(cmxhere,SEStateless); ;
    break;}
case 31:
#line 183 "direc.y"
{ yyval.annotation=new StateAnnoAtom(cmxhere,SEROState); ;
    break;}
case 32:
#line 184 "direc.y"
{ yyval.annotation=new StateAnnoAtom(cmxhere,SERWState); ;
    break;}
case 33:
#line 187 "direc.y"
{ yyval.annotation=new TimeAnnoAtom(cmxhere,VarIntSpectime,CTSpectime); ;
    break;}
case 34:
#line 188 "direc.y"
{ yyval.annotation=new TimeAnnoAtom(cmxhere,VarIntResidual,CTResidual); ;
    break;}
case 35:
#line 189 "direc.y"
{ yyval.annotation=new CallAnnoAtom(cmxhere, CTNoAnno); ;
    break;}
case 36:
#line 192 "direc.y"
{ yyval.annotation=new VarAnnoAtom(cmxhere,VarVisSpectime); ;
    break;}
case 37:
#line 193 "direc.y"
{ yyval.annotation=new VarAnnoAtom(cmxhere,VarVisResidual); ;
    break;}
case 38:
#line 194 "direc.y"
{ yyval.annotation=new VarAnnoAtom(cmxhere,VarExtSpectime); ;
    break;}
case 39:
#line 195 "direc.y"
{ yyval.annotation=new VarAnnoAtom(cmxhere,VarConstant); ;
    break;}
case 43:
#line 201 "direc.y"
{ yyval.annotation=new DualAnnoAtom(yyvsp[-1].annotation,yyvsp[0].annotation); ;
    break;}
case 44:
#line 202 "direc.y"
{ yyval.annotation=new DualAnnoAtom(yyvsp[-1].annotation,yyvsp[0].annotation); ;
    break;}
case 45:
#line 207 "direc.y"
{
              GeneratorDirective *gd = yyvsp[0].generator ;
              gd->generator_name = yyvsp[-2].str ;
	      gd->pos = yyvsp[-3].pos ;
              yyval.generator = gd ;
	  ;
    break;}
case 46:
#line 215 "direc.y"
{
	      GeneratorDirective *gd = yyvsp[0].generator ;
	      gd->generator_name = "main" ;
              gd->pos = yyvsp[-1].pos ;
              yyval.generator = gd ;
          ;
    break;}
case 47:
#line 222 "direc.y"
{
              GeneratorDirective *gd = new GeneratorDirective(yyvsp[-4].str,yyvsp[-3].pos) ;
              gd->params = yyvsp[-1].params ;
              yyval.generator = gd ;
          ;
    break;}
case 48:
#line 229 "direc.y"
{ yyval.params = new Plist<GeneratorDirective::Param>(); ;
    break;}
case 49:
#line 230 "direc.y"
{ yyval.params = yyvsp[0].params; ;
    break;}
case 50:
#line 233 "direc.y"
{ yyval.params = new Plist<GeneratorDirective::Param>(yyvsp[0].param); ;
    break;}
case 51:
#line 234 "direc.y"
{
	     Plist<GeneratorDirective::Param>* ps = yyvsp[-2].params ;
	     ps->push_back(yyvsp[0].param);
             yyval.params = ps ;
          ;
    break;}
case 52:
#line 241 "direc.y"
{ yyval.param = new GeneratorDirective::Param
                               (GeneratorDirective::Residual); ;
    break;}
case 53:
#line 243 "direc.y"
{ yyval.param = new GeneratorDirective::Param
                               (GeneratorDirective::SpectimeArg,yyvsp[0].numeric); ;
    break;}
case 54:
#line 245 "direc.y"
{ yyval.param = new GeneratorDirective::Param
                               (GeneratorDirective::ConstInteger,yyvsp[0].str); ;
    break;}
case 55:
#line 247 "direc.y"
{ yyval.param = new GeneratorDirective::Param
			       (GeneratorDirective::ConstString,yyvsp[0].str); ;
    break;}
case 56:
#line 251 "direc.y"
{
              GeneratorDirective *gd = yyvsp[-5].generator ;
	      gd->residual_name_base = yyvsp[-2].str ;
              gd->residual_name_parts = yyvsp[-1].params ;
              directives.add_generator(gd);
          ;
    break;}
case 57:
#line 257 "direc.y"
{
              GeneratorDirective *gd = yyvsp[-2].generator ;
	      gd->residual_name_base = yyvsp[0].str ;
              gd->residual_name_parts = new Plist<GeneratorDirective::Param>();
              directives.add_generator(gd);
          ;
    break;}
case 58:
#line 263 "direc.y"
{
              GeneratorDirective *gd = yyvsp[-2].generator ;
	      char *cbuf = new char[strlen(yyvsp[0].str)+3] ;
	      strcpy(cbuf,"\"");
	      strcat(cbuf,yyvsp[0].str);
	      strcat(cbuf,"\"");
	      gd->residual_name_base = cbuf ;
              gd->residual_name_parts = new Plist<GeneratorDirective::Param>();
              directives.add_generator(gd);
          ;
    break;}
case 59:
#line 273 "direc.y"
{
              GeneratorDirective *gd = yyvsp[0].generator ;
	      char *cbuf = new char[strlen(gd->subject_name)+3];
	      strcpy(cbuf,"\"");
	      strcat(cbuf,gd->subject_name);
	      strcat(cbuf,"\"");
	      gd->residual_name_base = cbuf ;
              gd->residual_name_parts = new Plist<GeneratorDirective::Param>();
              directives.add_generator(gd);
          ;
    break;}
case 60:
#line 286 "direc.y"
{ yyval.params = new Plist<GeneratorDirective::Param>(); ;
    break;}
case 61:
#line 287 "direc.y"
{
	     Plist<GeneratorDirective::Param>* ps = yyvsp[-2].params ;
	     ps->push_back(new GeneratorDirective::Param
                                  (GeneratorDirective::SpectimeArg,yyvsp[0].numeric));
             yyval.params = ps ;
          ;
    break;}
case 62:
#line 297 "direc.y"
{
              static char const *const ignorenothing[] = { "", NULL };
              CmixOutput::ProposeBase(yyvsp[0].str,900,ignorenothing);
              delete[] yyvsp[0].str ;
          ;
    break;}
case 63:
#line 306 "direc.y"
{
	      char *cp = yyvsp[0].str + 1 ;
              while ( isalnum(*cp) ||
                      ispunct(*cp) && *cp != '"' && *cp != '\\'
                                   && *cp != '<' && *cp != '>' )
                  cp ++ ;
              if ( cp[1] )
                  cp = NULL ;
              else
                  switch (*(yyvsp[0].str)) {
                  case '<':
                      if ( *cp != '>' )
                          cp = NULL ;
                      break ;
                  case '"':
                      if ( *cp != '"' )
                          cp = NULL ;
                      break ;
                  default:
                      cp = NULL ;
                  }
              if ( !cp )
                  Diagnostic(WARNING,yyvsp[-1].pos)
                      << "suspicious header specification `" << yyvsp[0].str << "'" ;
	      directives.headers.push_back(yyvsp[0].str);
          ;
    break;}
case 65:
#line 339 "direc.y"
{ directives.add_wellknown(yyvsp[0].str) ;
    break;}
case 66:
#line 340 "direc.y"
{ directives.add_wellknown(yyvsp[0].str) ;
    break;}
case 68:
#line 347 "direc.y"
{ directives.add_taboo(yyvsp[0].str) ;
    break;}
case 69:
#line 348 "direc.y"
{ directives.add_taboo(yyvsp[0].str) ;
    break;}
case 70:
#line 352 "direc.y"
{
	      directives.add_debug(new DebugDirective(yyvsp[-5].str,yyvsp[0].str,yyvsp[-6].pos));
	   ;
    break;}
case 73:
#line 361 "direc.y"
{ yyval.str = yyvsp[0].str; ;
    break;}
case 74:
#line 362 "direc.y"
{ yyval.str = stringDup("specializes"); ;
    break;}
case 75:
#line 363 "direc.y"
{ yyval.str = stringDup("producing"); ;
    break;}
case 76:
#line 364 "direc.y"
{ yyval.str = stringDup("at"); ;
    break;}
case 77:
#line 365 "direc.y"
{ yyval.str = stringDup("most"); ;
    break;}
case 78:
#line 367 "direc.y"
{ yyval.pos.set(cmxhere) ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/local/store/share/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 370 "direc.y"

/* ----end of grammar----*/

#ifdef YYDEBUG
        static void settheflag() {
            if (trace_cmx_parser)
                yydebug=1 ;
        }

#endif
    
