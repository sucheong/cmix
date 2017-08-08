
/*  A Bison parser, made from outgram.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	PRODUCER	258
#define	TYPES	259
#define	ATTRIBUTES	260
#define	OUTPUT	261
#define	STRING	262
#define	NUMBER	263

#line 9 "outgram.y"

    #include <cmixconf.h>
  /* magic incantation from the autoconf manual */
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
    #include "annofront.h"
    #include <sys/stat.h>
    #include <string.h>
    #define YYERROR_VERBOSE

    struct AnnoTree *alloctree(void) {
        struct AnnoTree *t = alloc(struct AnnoTree);
        t->TreeTag = TREE_STRING ;
        t->NativeWidth = -1 ;
        t->PrettyHeight = 0 ;
        t->Type = 0 ;
        t->Label = 0 ;
        t->Attribute = 0 ;
        t->ConsistentBlock = 0 ;
        t->Indentation = 0 ;
        t->TreeContent1 = NULL ;
        t->TreeContent2 = NULL ;
        t->TextContent = "" ;
        t->BlockContent = NULL ;
        return t ;
    }

    #define yyerror die

#line 52 "outgram.y"
typedef union{
    char* str;
    unsigned long num;
    int integer;
    struct AnnoTree *tree ;
    struct BlockElement *element ;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		78
#define	YYFLAG		-32768
#define	YYNTBASE	26

#define YYTRANSLATE(x) ((unsigned)(x) <= 263 ? yytranslate[x] : 47)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    20,     2,    22,     2,     2,     2,     2,    15,
    16,     2,     2,     9,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    23,     2,    18,
    21,    19,    17,    10,     2,     2,     7,     2,     2,     2,
     2,     2,     8,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    11,     2,    12,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    13,     2,    14,     2,     2,     2,     2,     2,
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
     6,    24,    25
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     6,    12,    20,    24,    26,    29,    33,    37,
    39,    42,    45,    48,    50,    52,    54,    56,    62,    68,
    70,    72,    74,    78,    79,    81,    83,    86,    88,    95,
    97,   100,   104,   108,   111,   118,   120
};

static const short yyrhs[] = {    -1,
    28,    27,    29,    32,     0,     3,    23,    24,     4,    23,
     0,    28,    24,    21,    25,    15,    25,    16,     0,     5,
    23,    30,     0,    31,     0,    31,    30,     0,    24,    21,
    25,     0,     6,    23,    33,     0,    34,     0,    34,    33,
     0,    22,    25,     0,    34,    35,     0,    36,     0,    43,
     0,    44,     0,    42,     0,    37,    25,    13,    38,    14,
     0,    37,    25,    13,    39,    14,     0,     7,     0,     8,
     0,    39,     0,    39,    41,    38,     0,     0,    40,     0,
    35,     0,    35,    40,     0,    17,     0,    17,    18,    25,
     9,    25,    19,     0,    20,     0,    24,    25,     0,    10,
    11,    45,     0,    25,    23,    46,     0,    12,    46,     0,
    15,    25,     9,    25,    16,    45,     0,    35,     0,    41,
     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    81,    84,    86,    90,   108,   112,   114,   117,   122,   126,
   128,   132,   135,   141,   142,   143,   144,   147,   155,   161,
   162,   165,   174,   182,   185,   190,   193,   202,   209,   216,
   225,   234,   236,   245,   246,   256,   257
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","PRODUCER",
"TYPES","ATTRIBUTES","OUTPUT","'C'","'I'","','","'@'","'['","']'","'{'","'}'",
"'('","')'","'?'","'<'","'>'","'!'","'='","'#'","':'","STRING","NUMBER","file",
"@1","introduction","attr_section","attrlist","attr","output_section","outputlist",
"output","tree","block","block_type","blocklist","group","realgroup","separator",
"text","linked","target","linklist","contents", NULL
};
#endif

static const short yyr1[] = {     0,
    27,    26,    28,    28,    29,    30,    30,    31,    32,    33,
    33,    34,    34,    35,    35,    35,    35,    36,    36,    37,
    37,    38,    38,    39,    39,    40,    40,    41,    41,    41,
    42,    43,    44,    45,    45,    46,    46
};

static const short yyr2[] = {     0,
     0,     4,     5,     7,     3,     1,     2,     3,     3,     1,
     2,     2,     2,     1,     1,     1,     1,     5,     5,     1,
     1,     1,     3,     0,     1,     1,     2,     1,     6,     1,
     2,     3,     3,     2,     6,     1,     1
};

static const short yydefact[] = {     0,
     0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     2,     3,     0,     0,     5,     6,     0,
     0,     0,     7,     0,     9,    10,     4,     8,    12,    20,
    21,     0,     0,     0,    11,    13,    14,     0,    17,    15,
    16,     0,    31,     0,     0,     0,     0,    32,    28,    30,
    36,    37,    33,    24,    34,     0,     0,    26,     0,     0,
    25,     0,     0,    27,    18,    19,    24,     0,     0,    23,
    22,     0,     0,    35,    29,     0,     0,     0
};

static const short yydefgoto[] = {    76,
     5,     2,     9,    18,    19,    14,    25,    26,    58,    37,
    38,    59,    60,    61,    52,    39,    40,    41,    48,    53
};

static const short yypact[] = {    -2,
   -21,   -14,    -5,     3,    28,    32,    12,    16,    34,    18,
    23,    19,    21,-32768,-32768,    17,    24,-32768,    19,    25,
    30,    26,-32768,    27,-32768,     1,-32768,-32768,-32768,-32768,
-32768,    37,    29,    33,-32768,-32768,-32768,    35,-32768,-32768,
-32768,    -9,-32768,    -3,    36,    -3,    38,-32768,    39,-32768,
-32768,-32768,-32768,     5,-32768,    41,    40,     5,    44,    14,
-32768,    42,    46,-32768,-32768,-32768,     5,    43,    45,-32768,
    15,    -9,    47,-32768,-32768,    53,    61,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,    49,-32768,-32768,    48,-32768,   -26,-32768,
-32768,     2,     4,     6,   -44,-32768,-32768,-32768,   -10,    31
};


#define	YYLAST		77


static const short yytable[] = {    36,
     1,     3,    46,    30,    31,    47,    32,    30,    31,     4,
    32,    30,    31,    49,    32,    67,    50,    51,     6,    51,
    33,    34,    24,     7,    33,    34,    67,    66,    33,    34,
    49,    49,     8,    50,    50,    10,    11,    16,    12,    13,
    15,    21,    17,    20,    22,    27,    24,    42,    54,    62,
    28,    29,    77,    43,    69,    44,    57,    65,    72,    45,
    78,    74,    56,    64,    63,    75,    68,    23,    70,    73,
    71,     0,     0,    35,     0,     0,    55
};

static const short yycheck[] = {    26,
     3,    23,    12,     7,     8,    15,    10,     7,     8,    24,
    10,     7,     8,    17,    10,    60,    20,    44,    24,    46,
    24,    25,    22,    21,    24,    25,    71,    14,    24,    25,
    17,    17,     5,    20,    20,     4,    25,    15,    23,     6,
    23,    25,    24,    23,    21,    16,    22,    11,    13,     9,
    25,    25,     0,    25,     9,    23,    18,    14,    16,    25,
     0,    72,    25,    58,    25,    19,    25,    19,    67,    25,
    67,    -1,    -1,    26,    -1,    -1,    46
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
#line 82 "outgram.y"
{ free(yyvsp[0].str); ;
    break;}
case 3:
#line 87 "outgram.y"
{ yyval.str = yyvsp[-2].str; ;
    break;}
case 4:
#line 91 "outgram.y"
{
            struct TypeRec *type = typerec(yyvsp[-3].num) ;
            if ( type->defined )
                die("Two definitions of type number " PCT_MAGIC,yyvsp[-3].num);
            type->defined = 1 ;
            type->Name = yyvsp[-5].str ;
            if ( yyvsp[-3].num == yyvsp[-1].num ) {
                type->NextSibling = RootType ;
                RootType = yyvsp[-3].num ;
            } else {
                struct TypeRec *parent = typerec(yyvsp[-1].num) ;
                type->NextSibling = parent->FirstChild ;
                parent->FirstChild = yyvsp[-3].num ;
            }
            FinishTypeRec(yyvsp[-6].str,type);
        ;
    break;}
case 8:
#line 119 "outgram.y"
{;
    break;}
case 10:
#line 127 "outgram.y"
{
        ;
    break;}
case 11:
#line 129 "outgram.y"
{
        ;
    break;}
case 12:
#line 133 "outgram.y"
{
            yyval.num = yyvsp[0].num;
        ;
    break;}
case 13:
#line 136 "outgram.y"
{
            AddAnnoTree(yyvsp[-1].num,yyvsp[0].tree);
            yyval.num = yyvsp[-1].num ;
        ;
    break;}
case 14:
#line 142 "outgram.y"
{ yyval.tree = yyvsp[0].tree ;
    break;}
case 15:
#line 143 "outgram.y"
{ yyval.tree = yyvsp[0].tree ;
    break;}
case 16:
#line 144 "outgram.y"
{ yyval.tree = yyvsp[0].tree ;
    break;}
case 17:
#line 145 "outgram.y"
{ yyval.tree = yyvsp[0].tree ;
    break;}
case 18:
#line 148 "outgram.y"
{
            struct AnnoTree *tree = alloctree();
            tree->TreeTag = TREE_LIST ;
            tree->ConsistentBlock = yyvsp[-4].integer ;
            tree->Indentation = (int)yyvsp[-3].num ;
            tree->BlockContent = yyvsp[-1].element ;
            yyval.tree = tree ;
        ;
    break;}
case 19:
#line 156 "outgram.y"
{
            /* shortcut: single-group blocks are just interpolated */
            yyval.tree = yyvsp[-1].tree ;
        ;
    break;}
case 20:
#line 162 "outgram.y"
{ yyval.integer = 1 ;
    break;}
case 21:
#line 163 "outgram.y"
{ yyval.integer = 0 ;
    break;}
case 22:
#line 166 "outgram.y"
{
            struct BlockElement *elt = alloc(struct BlockElement);
            elt->Contents = yyvsp[0].tree ;
            elt->SeparatorTag = SEP_NEVER ;
            elt->Spaces = 0 ;
            elt->NextOffset = 0 ;
            elt->next = NULL ;
            yyval.element = elt ;
        ;
    break;}
case 23:
#line 175 "outgram.y"
{
            struct BlockElement *elt = yyvsp[-1].element ;
            elt->Contents = yyvsp[-2].tree ;
            elt->next = yyvsp[0].element ;
            yyval.element = elt ;
        ;
    break;}
case 24:
#line 183 "outgram.y"
{
            yyval.tree = alloctree() ;
        ;
    break;}
case 25:
#line 186 "outgram.y"
{
            yyval.tree = yyvsp[0].tree ;
        ;
    break;}
case 26:
#line 191 "outgram.y"
{
            yyval.tree = yyvsp[0].tree ;
        ;
    break;}
case 27:
#line 194 "outgram.y"
{
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_CONCAT ;
            tree->TreeContent1 = yyvsp[-1].tree ;
            tree->TreeContent2 = yyvsp[0].tree ;
            yyval.tree = tree ;
        ;
    break;}
case 28:
#line 203 "outgram.y"
{
            struct BlockElement *elt = alloc(struct BlockElement) ;
            elt->SeparatorTag = SEP_BREAK ;
            elt->Spaces = 1 ;
            elt->NextOffset = 0 ;
            yyval.element = elt ;
        ;
    break;}
case 29:
#line 210 "outgram.y"
{
            struct BlockElement *elt = alloc(struct BlockElement);
            elt->SeparatorTag = SEP_BREAK ;
            elt->Spaces = (int)yyvsp[-3].num ;
            elt->NextOffset = (int)yyvsp[-1].num ;
            yyval.element = elt ;
        ;
    break;}
case 30:
#line 217 "outgram.y"
{
            struct BlockElement *elt = alloc(struct BlockElement);
            elt->SeparatorTag = SEP_NEWLINE ;
            elt->Spaces = 1 ;
            elt->NextOffset = 0 ;
            yyval.element = elt ;
        ;
    break;}
case 31:
#line 226 "outgram.y"
{
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_STRING ;
            tree->TextContent = yyvsp[-1].str ;
            tree->Type = yyvsp[0].num ;
            yyval.tree = tree ;
        ;
    break;}
case 32:
#line 235 "outgram.y"
{ yyval.tree = yyvsp[0].tree; ;
    break;}
case 33:
#line 237 "outgram.y"
{
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_LABEL ;
            tree->TreeContent1 = yyvsp[0].tree ;
            tree->Label = yyvsp[-2].num ;
            yyval.tree = tree ;
        ;
    break;}
case 34:
#line 246 "outgram.y"
{ yyval.tree = yyvsp[0].tree; ;
    break;}
case 35:
#line 247 "outgram.y"
{
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_LINK ;
            tree->TreeContent1 = yyvsp[0].tree ;
            tree->Type = yyvsp[-2].num ;
            tree->Label = yyvsp[-4].num ;
            yyval.tree = tree ;
        ;
    break;}
case 36:
#line 257 "outgram.y"
{ yyval.tree = yyvsp[0].tree; ;
    break;}
case 37:
#line 258 "outgram.y"
{
            struct AnnoTree *tree = alloctree() ;
            free(yyvsp[0].element);
            tree->TreeTag = TREE_STRING ;
            tree->TextContent = "" ;
            tree->Attribute = 0 ;
            yyval.tree = tree;
        ;
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
#line 267 "outgram.y"

             
static const char* rereadfile = NULL ;
static time_t rereadfile_mtime = 0 ;

static time_t getmtime(void) {
    struct stat statbuf ;
    if ( stat(rereadfile,&statbuf) == 0 )
        return statbuf.st_mtime ;
    else
        return rereadfile_mtime ;
}

static void textreader(FILE *f) {
    extern void yyrestart(FILE *);
    yyrestart(f);
    yyparse();
    NormalizeCut();
}

void readthetext(const char *filename) {
    if ( strcmp(filename,"-") != 0 ) {
        FILE *f ;
        char *newbuffer = NULL ;
        rereadfile = filename ;
	f = fopen(filename,"rt");
        if ( f == NULL ) {
            newbuffer = malloc(strlen(filename)+4);
            if ( newbuffer != NULL ) {
                strcpy(newbuffer,filename);
                strcat(newbuffer,".ann");
                f = fopen(newbuffer,"rt");
            }
            rereadfile = newbuffer ;
        }
        if ( f == NULL )
            die("Couldn't open input file %s",
                newbuffer ? filename : " (out of memory)");
        rereadfile_mtime = getmtime();
        textreader(f);
        fclose(f);
    } else
        textreader(stdin);
}

void perhapsreread(void) {
    if ( rereadfile ) {
        time_t timenow = getmtime();
        if ( timenow != rereadfile_mtime ) {
            FILE *f = fopen(rereadfile,"rt");
            rereadfile_mtime = timenow ;
            if ( f != NULL ) {
                FreeEverything();
                textreader(f);
                fclose(f);
            }
        }
    }
}

const char *getfilename() {
    return rereadfile ? rereadfile : "(standard input)";
}
