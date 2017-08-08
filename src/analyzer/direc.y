/* -*-Fundamental-*-
 * Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Grammar for specializer directives
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

%{

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

%}
    
%union{
    char* str;
    int numeric;
    AnnoAtom *annotation ;
    GeneratorDirective* generator ;
    Plist<GeneratorDirective::Param>* params ;
    GeneratorDirective::Param* param ;
    PositionInUnion pos;
}

/* Define terminal tokens */

%token DEFINE SOURCE GENERATOR OUTPUTBASE HEADER
%token SPECTIME RESIDUAL VISIBLE DANGEROUS ANYTIME
%token PURE STATELESS ROSTATE RWSTATE
%token WELLKNOWN CONSTANT TABOO
%token SPECIALIZES PRODUCING GOAL
%token COLONCOLON DOLLAR
%token DEBUG AT MOST
%token UNSIGNED CHARS ARE GLYPHS
%token LIFT LONG DOUBLE

%token WORD INT IDENT STRING

/*************************************************************************/

%type <str> WORD IDENT STRING identifier INT
%type <numeric> DOLLAR
%type <annotation> user_annotation anno_start anno_directive
%type <annotation> uanno_state uanno_funtime uanno_vartime
%type <pos> marker
%type <generator> goal generator_directive
%type <params> goal_parameters goal_plist name_plist
%type <param> goal_parm

/*************************************************************************/

%start flag_setter

/*************************************************************************/

%%

flag_setter
	: { settheflag(); } grammar_dispatch

grammar_dispatch
	: specification_file		{ TheCmxResult = CmxResult(); }
	| /* empty */			{ TheCmxResult = CmxResult(); }
	| user_annotation		{ TheCmxResult = $1; }

specification_file
	: directive
	| specification_semi directive
	| specification_semi

specification_semi
	: specification_file ';'
	| ';'

directive
        : error
        
/* * * * * * * * * * * * */

directive : UNSIGNED CHARS ARE GLYPHS { lift_uchar_as_char = true; }

directive : LIFT LONG DOUBLE { lift_long_double = true; }

/* * * * * * * * * * * * */

directive
	: define_directive

define_directive
	: define_start marker WORD {
		directives.add_cpparg('D',$3);
                free($3);
	  }

define_start
	: DEFINE ':'
	| define_directive

/* * * * * * * * * * * * */

directive
	: source_directive

source_directive
	: source_start marker WORD { directives.add_source($3,$2); }

source_start
	: SOURCE ':'
	| source_directive

/* * * * * * * * * * * * */

directive
	: anno_directive {}

anno_directive
	: anno_start identifier {
		directives.add_globalanno($1,$2) ;
		$$ = $1 ;
	  }
	| anno_start identifier COLONCOLON identifier {
		directives.add_localanno($1,$2,$4) ;
		$$ = $1 ;
	  }
	| anno_start identifier '(' ')' {
		directives.add_funanno($1,$2);
		$$ = $1 ;
	  }
	| anno_start ',' { $$ = $1 }

anno_start
	: user_annotation ':' { $$ = $1 ; }
	| anno_directive { $$ = $1 ; }

uanno_state
	: PURE      { $$=new StateAnnoAtom(cmxhere,SEPure); }
	| STATELESS { $$=new StateAnnoAtom(cmxhere,SEStateless); }
	| ROSTATE   { $$=new StateAnnoAtom(cmxhere,SEROState); }
	| RWSTATE   { $$=new StateAnnoAtom(cmxhere,SERWState); }

uanno_funtime
	: SPECTIME { $$=new TimeAnnoAtom(cmxhere,VarIntSpectime,CTSpectime); }
	| RESIDUAL { $$=new TimeAnnoAtom(cmxhere,VarIntResidual,CTResidual); }
	| ANYTIME  { $$=new CallAnnoAtom(cmxhere, CTNoAnno); }

uanno_vartime
	: VISIBLE SPECTIME   { $$=new VarAnnoAtom(cmxhere,VarVisSpectime); }
	| VISIBLE RESIDUAL   { $$=new VarAnnoAtom(cmxhere,VarVisResidual); }
	| DANGEROUS SPECTIME { $$=new VarAnnoAtom(cmxhere,VarExtSpectime); }
	| WELLKNOWN CONSTANT { $$=new VarAnnoAtom(cmxhere,VarConstant); }

user_annotation
	: uanno_funtime
	| uanno_state
	| uanno_vartime
	| uanno_funtime uanno_state { $$=new DualAnnoAtom($1,$2); }
	| uanno_state uanno_funtime { $$=new DualAnnoAtom($1,$2); }

/* * * * * * * * * * * * */

generator_directive
	: GENERATOR ':' marker identifier SPECIALIZES goal {
              GeneratorDirective *gd = $6 ;
              gd->generator_name = $4 ;
	      gd->pos = $3 ;
              $$ = gd ;
	  }

generator_directive
	: GOAL ':' marker goal {
	      GeneratorDirective *gd = $4 ;
	      gd->generator_name = "main" ;
              gd->pos = $3 ;
              $$ = gd ;
          }

goal	: identifier marker '(' goal_parameters ')' {
              GeneratorDirective *gd = new GeneratorDirective($1,$2) ;
              gd->params = $4 ;
              $$ = gd ;
          }

goal_parameters
	: /* empty */ { $$ = new Plist<GeneratorDirective::Param>(); }
        | goal_plist  { $$ = $1; } 

goal_plist
	: goal_parm { $$ = new Plist<GeneratorDirective::Param>($1); }
	| goal_plist ',' goal_parm {
	     Plist<GeneratorDirective::Param>* ps = $1 ;
	     ps->push_back($3);
             $$ = ps ;
          }

goal_parm
	: '?'    { $$ = new GeneratorDirective::Param
                               (GeneratorDirective::Residual); }
	| DOLLAR { $$ = new GeneratorDirective::Param
                               (GeneratorDirective::SpectimeArg,$1); }
	| INT    { $$ = new GeneratorDirective::Param
                               (GeneratorDirective::ConstInteger,$1); }
	| STRING { $$ = new GeneratorDirective::Param
			       (GeneratorDirective::ConstString,$1); }

directive
	: generator_directive PRODUCING '(' STRING name_plist ')' {
              GeneratorDirective *gd = $1 ;
	      gd->residual_name_base = $4 ;
              gd->residual_name_parts = $5 ;
              directives.add_generator(gd);
          }
	| generator_directive PRODUCING STRING {
              GeneratorDirective *gd = $1 ;
	      gd->residual_name_base = $3 ;
              gd->residual_name_parts = new Plist<GeneratorDirective::Param>();
              directives.add_generator(gd);
          }
	| generator_directive PRODUCING identifier {
              GeneratorDirective *gd = $1 ;
	      char *cbuf = new char[strlen($3)+3] ;
	      strcpy(cbuf,"\"");
	      strcat(cbuf,$3);
	      strcat(cbuf,"\"");
	      gd->residual_name_base = cbuf ;
              gd->residual_name_parts = new Plist<GeneratorDirective::Param>();
              directives.add_generator(gd);
          }
	| generator_directive {
              GeneratorDirective *gd = $1 ;
	      char *cbuf = new char[strlen(gd->subject_name)+3];
	      strcpy(cbuf,"\"");
	      strcat(cbuf,gd->subject_name);
	      strcat(cbuf,"\"");
	      gd->residual_name_base = cbuf ;
              gd->residual_name_parts = new Plist<GeneratorDirective::Param>();
              directives.add_generator(gd);
          }


name_plist
	: /* empty */ { $$ = new Plist<GeneratorDirective::Param>(); }
	| name_plist ',' DOLLAR {
	     Plist<GeneratorDirective::Param>* ps = $1 ;
	     ps->push_back(new GeneratorDirective::Param
                                  (GeneratorDirective::SpectimeArg,$3));
             $$ = ps ;
          }

/* * * * * * * * * * * * */

directive
	: OUTPUTBASE ':' WORD {
              static char const *const ignorenothing[] = { "", NULL };
              CmixOutput::ProposeBase($3,900,ignorenothing);
              delete[] $3 ;
          }

/* * * * * * * * * * * * */

directive
	: HEADER ':' marker WORD {
	      char *cp = $4 + 1 ;
              while ( isalnum(*cp) ||
                      ispunct(*cp) && *cp != '"' && *cp != '\\'
                                   && *cp != '<' && *cp != '>' )
                  cp ++ ;
              if ( cp[1] )
                  cp = NULL ;
              else
                  switch (*($4)) {
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
                  Diagnostic(WARNING,$3)
                      << "suspicious header specification `" << $4 << "'" ;
	      directives.headers.push_back($4);
          }

/* * * * * * * * * * * * */

directive
	: wellknown_directive

wellknown_directive
	: WELLKNOWN ':' identifier	 { directives.add_wellknown($3) }
	| wellknown_directive identifier { directives.add_wellknown($2) }

/* * * * * * * * * * * * */

directive: taboo_directive

taboo_directive
	: TABOO ':' identifier		{ directives.add_taboo($3) }
	| taboo_directive identifier	{ directives.add_taboo($2) }

/* * * * * * * * * * * * */

directive: DEBUG ':' marker identifier opt_parens PRODUCING AT MOST INT {
	      directives.add_debug(new DebugDirective($4,$9,$3));
	   }

opt_parens: '(' ')' | ;

/* * * * * * * * * * * * */

identifier
	: IDENT { $$ = $1; }
	| SPECIALIZES { $$ = stringDup("specializes"); }
	| PRODUCING { $$ = stringDup("producing"); }
	| AT { $$ = stringDup("at"); }
	| MOST { $$ = stringDup("most"); }

marker: { $$.set(cmxhere) }


%%
/* ----end of grammar----*/

#ifdef YYDEBUG
        static void settheflag() {
            if (trace_cmx_parser)
                yydebug=1 ;
        }

#endif
    
