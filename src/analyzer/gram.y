/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: C Parser
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

/* Copyright (C) 1989,1990 James A. Roskind, All rights reserved.
    This grammar was developed  and  written  by  James  A.  Roskind.
    ...
    later the C-Mix folks FUBARed it
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

#include "Plist.h"
#include "symboltable.h"
#include "syntax.h"
#include "cpgm.h"
#include "liststack.h"
#include "directives.h"
#include "auxilary.h"
#include "options.h"
#include "parser.h"

    static void yyerror(const char*message) {
        Diagnostic(ERROR,ccchere) << message;
    }

#define YYERROR_VERBOSE

#define YYDEBUG 1

#ifdef YYDEBUG
    static void settheflag();
#else
    #define settheflag()
#endif

// trick bison into producing a member function rather
// than a freestanding C function.
#define cccparse(x) cccParser::ParseNow(x)

extern Scope<ObjectDecl> names;                 // Names
extern ListStack<VarDecl> objects;             // List of real objects.

%}

%expect 1

%union{
    // Temporaries and switchboards (syntax.h)
    PositionInUnion pos ;
    Parse_GeneralType* gtype;
    Parse_Type* pt;
    Parse_UserType* utype;
    UserTag uttag;
    Parse_TypedefType* tdtype;
    Plist<Parse_Typemod>* typemods;
    Parse_Declarator* idmod;
    Parse_MemberId* mempost;
    char* str;
    bool boolean;
    // Permanent structures/classes (cpgm.h)
    Type* type;
    Expr* expr;
    ConstExpr* cexpr;
    Stmt* stmt;
    Init* init;
    Plist<VarDecl>* vars;
    Plist<Expr>* exprs;
    Plist<MemberDecl>* membs;
    Plist<Stmt>* stmts;
    CProgram* cpgm;
    UnOp unop;
    AssignOp asgnop;
}

/* Define terminal tokens */

/* New Lexical element, whereas ANSI suggested non-terminal */

%token AUTO             DOUBLE          INT             STRUCT
%token BREAK            ELSE            LONG            SWITCH
%token CASE             ENUM            REGISTER        TYPEDEF
%token CHAR             EXTERN          RETURN          UNION
%token CONST            FLOAT           SHORT           UNSIGNED
%token CONTINUE         FOR             SIGNED          VOID
%token DEFAULT          GOTO            SIZEOF          VOLATILE
%token DO               IF              STATIC          WHILE

%token FLOATINGconstant         INTEGERconstant         CHARACTERconstant
%token LONGconstant             UINTEGERconstant        STRINGliteral
%token ULONGconstant            DOUBLEconstant

/* Multi-Character operators */
%token  ARROW                 /*    ->                              */
%token  ICR DECR              /*    ++      --                      */
%token  LEFTSHIFT RIGHTSHIFT  /*    <<      >>                      */
%token  LESSEQUAL GREATEQUAL  /*    <=      >=                      */
%token  EQUAL NOTEQUAL        /*    <=      >=      ==      !=      */
%token  ANDAND OROR           /*    &&      ||                      */
%token  ELLIPSIS              /*    ...                             */

/* modifying assignment operators */
%token MULTassign  DIVassign    MODassign   /*   *=      /=      %=      */
%token PLUSassign  MINUSassign              /*   +=      -=              */
%token LSassign    RSassign                 /*   <<=     >>=             */
%token ANDassign   ERassign     ORassign    /*   &=      ^=      |=      */

%token <str> IDENTIFIER
%token <str> TYPEDEFname /* Lexer will tell the difference between this and
    an  identifier!   An  identifier  that is CURRENTLY in scope as a
    typedef name is provided to the parser as a TYPEDEFname.*/

%token <str> CMIXTAG

/*************************************************************************/

%type <expr> constant_expression assignment_expression

%type <uttag> aggregate_key

%type <pt> declaration_qualifier_list type_qualifier_list
%type <pt> declaration_qualifier type_qualifier basic_declaration_specifier
%type <pt> basic_type_name storage_class basic_type_specifier

%type <gtype> declaration_specifier type_specifier
%type <utype> sue_type_specifier sue_declaration_specifier
%type <utype> elaborated_type_name aggregate_name enum_name
%type <vars> enumerator_list
%type <expr> enumerator_value_opt
%type <type> type_name
%type <gtype> member_declaring_list member_default_declaring_list
%type <gtype> declaring_list default_declaring_list

%type <tdtype> typedef_type_specifier typedef_declaration_specifier

/*%type <membs> member_declaration_list member_declaration*/
%type <expr> bit_field_size_opt bit_field_size
%type <str> identifier_or_typedef_name member_name
%type <init> initializer initializer_list initializer_opt
%type <stmt> statement, jump_statement, compound_statement, compound_statement1
%type <stmt> labeled_statement
%type <stmt> expression_statement, selection_statement, iteration_statement
%type <stmts> statement_list statement_list_opt
%type <pos> compound_stmt_marker

%type <typemods> type_postfix array_postfix type_prefix
%type <boolean> parameter_type_list

%type <idmod> abstract_declarator abstract_declarator_opt
%type <idmod> declarator
%type <idmod> identifier_declarator mod_identifier_declarator
%type <idmod> typedef_declarator bare_typedef_declarator
%type <idmod> paren_typedef_declarator inner_paren_typedef_declarator
%type <idmod> clean_typedef_declarator parameter_typedef_declarator
%type <idmod> paren_identifier_declarator

%type <mempost> member_identifier_declarator member_declarator
%type <idmod> old_function_declarator
%type <idmod> function_head

%type <str> FLOATINGconstant INTEGERconstant CHARACTERconstant
%type <str> DOUBLEconstant UINTEGERconstant STRINGliteral
%type <str> LONGconstant ULONGconstant

%type <cexpr> constant string_literal_list
%type <expr> primary_expression postfix_expression comma_expression_opt
%type <expr> unary_expression comma_expression
%type <unop> unary_operator
%type <asgnop> assignment_operator
%type <expr> conditional_expression logical_OR_expression
%type <expr> logical_AND_expression inclusive_OR_expression
%type <expr> exclusive_OR_expression AND_expression equality_expression
%type <expr> relational_expression shift_expression additive_expression
%type <expr> multiplicative_expression cast_expression
%type <exprs> argument_expression_list argument_list
/*************************************************************************/

%start start_symbol

/*************************************************************************/

%%

/************************** CONSTANTS *********************************/

constant:
        INTEGERconstant
          {
              $$=new ConstExpr($1,Integer,ccchere);
          }
        | UINTEGERconstant
          {
              $$=new ConstExpr($1,UInteger,ccchere);
          }
        | LONGconstant
          {
              $$=new ConstExpr($1,Long,ccchere);
          }
        | ULONGconstant
          {
              $$=new ConstExpr($1,ULong,ccchere);
          }
        | FLOATINGconstant
          {
              $$=new ConstExpr($1,Floating,ccchere);
          }
        | DOUBLEconstant
          {
              $$=new ConstExpr($1,DoubleF,ccchere);
          }
        | CHARACTERconstant
          {
              // Remember that the pings are still present.
              $$=new ConstExpr($1,Character,ccchere);
          }
        ;

string_literal_list:
	  STRINGliteral
          {
              // Remember that the quotes are still present.
              $$=new ConstExpr($1,StrConst,ccchere);
          }
        | string_literal_list STRINGliteral
          {
              // Concatenate the two strings by removing the " at
              // the end of the first string and the beginning of
              // the second one.
              int s1 = strlen($1->literal);
              int s2 = strlen($2);
              char* str = new char[s1+s2-1];
              strcpy(str,$1->literal);
              strcpy(str+s1-1,$2+1);
              $1->literal = str;
              $$ = $1;
          }
        ;

/************************* EXPRESSIONS ********************************/

primary_expression:
          IDENTIFIER
          {
              // Find the declaration for this identifier.
              $$=add_var_ref($1,ccchere);
          }
        | CMIXTAG ')' IDENTIFIER
          {
            VarExpr *e = add_var_ref($3,ccchere);
            e->annos = directives.AnnoFromString($1,ccchere);
            $$ = e ;
          }
        | constant			{ $$=$1; }
        | string_literal_list		{ $$=$1; }
        | '(' comma_expression ')'	{ $$=$2; }
        ;

argument_list:
        '(' ')'  { $$ = new Plist<Expr>(); }
        | '(' argument_expression_list ')' { $$ = $2 ; }
        ;

postfix_expression:
        primary_expression { $$=$1; }
        | postfix_expression '[' comma_expression ']'
          {
              $$=new ArrayExpr($1,$3,$1->pos);
          }
        | postfix_expression argument_list
          {
              $$=new CallExpr($1,$1->pos,$2);
          }
        | postfix_expression {} '.'   member_name
          {
              $$=new DotExpr($1,$4,$1->pos);
          }
        | postfix_expression {} ARROW member_name
          {
              // Convert "a->m" into "(*a).m"
              UnaryExpr* p = new UnaryExpr($1);
              $$=new DotExpr(p,$4,p->pos);
          }
        | postfix_expression ICR
          {
              $$=new PostExpr($1,Inc,$1->pos);
          }
        | postfix_expression DECR
          {
              $$=new PostExpr($1,Decr,$1->pos);
          }
        ;

member_name:
          IDENTIFIER
        | TYPEDEFname
        ;

argument_expression_list:
          assignment_expression
          {
              $$=new Plist<Expr>($1);
          }
        | argument_expression_list ',' assignment_expression
          {
              $1->push_back($3); $$=$1;
          }
        ;

unary_expression:
          postfix_expression { $$=$1; }
        | ICR unary_expression { $$=new PreExpr($2,Inc,$2->pos); }
        | DECR unary_expression { $$=new PreExpr($2,Decr,$2->pos); }
        | unary_operator cast_expression
          {
              $$=new UnaryExpr($2,$1,$2->pos);
          }
        | SIZEOF unary_expression
          {
              $$=new ExprSize($2,$2->pos)
          }
        | SIZEOF '(' type_name ')'
          {
              $$=new TypeSize($3,ccchere)
          }
        ;

unary_operator:
        '&' { $$=Addr; }
        | '*' { $$=DeRef; }
        | '+' { $$=Pos; }
        | '-' { $$=Neg; }
        | '~' { $$=Not; }
        | '!' { $$=Bang; }
        ;

cast_expression:
        unary_expression
        | '(' type_name ')' cast_expression
        {
            $$=new CastExpr($2,$4,ccchere);
        }
        ;

multiplicative_expression:
          cast_expression
        | multiplicative_expression '*' cast_expression
          {
              $$=new BinaryExpr($1,$3,Mul,$1->pos);
          }
        | multiplicative_expression '/' cast_expression
          {
              $$=new BinaryExpr($1,$3,Div,$1->pos);
          }
        | multiplicative_expression '%' cast_expression
          {
              $$=new BinaryExpr($1,$3,Mod,$1->pos);
          }
        ;

additive_expression:
          multiplicative_expression
        | additive_expression '+' multiplicative_expression
          {
              $$=new BinaryExpr($1,$3,Add,$1->pos);
          }
        | additive_expression '-' multiplicative_expression
          {
              $$=new BinaryExpr($1,$3,Sub,$1->pos);
          }
        ;

shift_expression:
          additive_expression
        | shift_expression LEFTSHIFT additive_expression
          {
              $$=new BinaryExpr($1,$3,LShift,$1->pos);
          }
        | shift_expression RIGHTSHIFT additive_expression
          {
              $$=new BinaryExpr($1,$3,RShift,$1->pos);
          }
        ;

relational_expression:
          shift_expression
        | relational_expression '<' shift_expression
          {
              $$=new BinaryExpr($1,$3,LT,$1->pos);
          }
        | relational_expression '>' shift_expression
          {
              $$=new BinaryExpr($1,$3,GT,$1->pos);
          }
        | relational_expression LESSEQUAL shift_expression
          {
              $$=new BinaryExpr($1,$3,LEq,$1->pos);
          }
        | relational_expression GREATEQUAL shift_expression
          {
              $$=new BinaryExpr($1,$3,GEq,$1->pos);
          }
        ;

equality_expression:
          relational_expression
        | equality_expression EQUAL relational_expression
          {
              $$=new BinaryExpr($1,$3,Eq,$1->pos);
          }
        | equality_expression NOTEQUAL relational_expression
          {
              $$=new BinaryExpr($1,$3,NEq,$1->pos);
          }
        ;

AND_expression:
          equality_expression
        | AND_expression '&' equality_expression
          {
              $$=new BinaryExpr($1,$3,BAnd,$1->pos);
          }
        ;

exclusive_OR_expression:
          AND_expression
        | exclusive_OR_expression '^' AND_expression
          {
              $$=new BinaryExpr($1,$3,BEOr,$1->pos);
          }
        ;

inclusive_OR_expression:
          exclusive_OR_expression
        | inclusive_OR_expression '|' exclusive_OR_expression
          {
              $$=new BinaryExpr($1,$3,BOr,$1->pos);
          }
        ;

logical_AND_expression:
          inclusive_OR_expression
        | logical_AND_expression ANDAND inclusive_OR_expression
          {
              $$=new BinaryExpr($1,$3,And,$1->pos);
          }
        ;

logical_OR_expression:
          logical_AND_expression
        | logical_OR_expression OROR logical_AND_expression
          {
              $$=new BinaryExpr($1,$3,Or,$1->pos);
          }
        ;

conditional_expression:
          logical_OR_expression
        | logical_OR_expression '?' comma_expression ':' conditional_expression
          {
              $$=new CondExpr($1,$3,$5,$1->pos);
          }
        ;

assignment_expression:
          conditional_expression
        | unary_expression assignment_operator assignment_expression
          {
              $$=new AssignExpr($1,$3,$2,$1->pos);
          }
        ;

assignment_operator:
          '=' { $$=Asgn; }
        | MULTassign { $$=MulAsgn; }
        | DIVassign { $$=DivAsgn; }
        | MODassign { $$=ModAsgn; }
        | PLUSassign { $$=AddAsgn; }
        | MINUSassign { $$=SubAsgn; }
        | LSassign { $$=LSAsgn; }
        | RSassign { $$=RSAsgn; }
        | ANDassign { $$=AndAsgn; }
        | ERassign { $$=EOrAsgn; }
        | ORassign { $$=OrAsgn; }
        ;

comma_expression:
          assignment_expression
        | comma_expression ',' assignment_expression
          { $$=new CommaExpr($1,$3,$1->pos); }
        ;

constant_expression:
          conditional_expression
        ;

comma_expression_opt:
          /* nothing */
          { $$=NULL; }
        | comma_expression
        ;


/******************************* DECLARATIONS *********************************/

    /* The following is different from the ANSI C specified  grammar.
    The  changes  were  made  to  disambiguate  typedef's presence in
    declaration_specifiers (vs.  in the declarator for redefinition);
    to allow struct/union/enum tag declarations without  declarators,
    and  to  better  reflect the parsing of declarations (declarators
    must be combined with declaration_specifiers ASAP  so  that  they
    are visible in scope).

    Example  of  typedef  use  as either a declaration_specifier or a
    declarator:

      typedef int T;
      struct S { T T;}; /* redefinition of T as member name * /

    Example of legal and illegal statements detected by this grammar:

      int; /* syntax error: vacuous declaration * /
      struct S;  /* no error: tag is defined or elaborated * /

    Example of result of proper declaration binding:

        int a=sizeof(a); /* note that "a" is declared with a type  in
            the name space BEFORE parsing the initializer * /

        int b, c[sizeof(b)]; /* Note that the first declarator "b" is
             declared  with  a  type  BEFORE the second declarator is
             parsed * /

    */

declaration:
        sue_declaration_specifier ';'
          {
	    // A lone structure declaration. It has to be inserted if
	    // it is not in local scope.
	    lone_sue($1);
	  }
        | sue_type_specifier ';'
          {
	    lone_sue($1);
	  }
        | declaring_list ';' { delete $1; }
        | default_declaring_list ';' { delete $1; }
        ;

    /* Note that if a typedef were  redeclared,  then  a  declaration
    specifier must be supplied */

default_declaring_list: /* decl */ /* Can't redeclare typedef names */
        declaration_qualifier_list identifier_declarator
          { add_declaration($1,$2); $<gtype>$ = $1; }
          initializer_opt
          { add_initializer($4); $$=$<gtype>3; }
        | type_qualifier_list identifier_declarator
          { add_declaration($1,$2); $<gtype>$=$1; }
          initializer_opt
          { add_initializer($4); $$=$<gtype>3; }
        | default_declaring_list ',' identifier_declarator
          { add_declaration($1,$3); }
          initializer_opt
          { add_initializer($5); $$=$1; }
        ;

declaring_list: /* tdecl */
        declaration_specifier declarator
          { add_declaration($1,$2); $<gtype>$ = $1; }
          initializer_opt
          { add_initializer($4); $$=$<gtype>3; }
        | type_specifier declarator
          { add_declaration($1,$2); $<gtype>$ = $1; }
          initializer_opt
          { add_initializer($4); $$=$<gtype>3; }
        | declaring_list ',' declarator
          { add_declaration($1,$3); }
          initializer_opt
          { add_initializer($5); $$=$1; }
        ;

declaration_specifier:  /* Parse_Decl*  */
        basic_declaration_specifier          /* Arithmetic or void */
          { $$=$1; }
        | sue_declaration_specifier          /* struct/union/enum */
          { $$=$1; }
        | typedef_declaration_specifier      /* typedef*/
          { $$=$1; }
        ;

type_specifier:  /* Type*  */
        basic_type_specifier                 /* Arithmetic or void */
          { $$=$1; }
        | sue_type_specifier                 /* Struct/Union/Enum */
          { $$=$1; }
        | typedef_type_specifier             /* Typedef */
          { $$=$1; }
        ;


declaration_qualifier_list:  /* Parse_Type */ /* const/volatile, AND storage class */
        storage_class
        | type_qualifier_list storage_class
          { $1->combine(*$2); $$=$1 }
        | declaration_qualifier_list declaration_qualifier
          { $1->combine(*$2); $$=$1 }
        ;

type_qualifier_list:  /* Parse_Type*  */
        type_qualifier
        | type_qualifier_list type_qualifier
          { $1->combine(*$2); $$=$1 }
        ;

declaration_qualifier:  /*  Parse_Type*  */
        storage_class
        | type_qualifier                  /* const or volatile */
        ;

type_qualifier:  /*  Parse_Type*  */
        CONST { ($$=new Parse_Type())->qualifiers=constvol(true,false); }
        | VOLATILE { ($$=new Parse_Type())->qualifiers=constvol(false,true); }
        ;

basic_declaration_specifier: /* Parse_Type*  */
                             /*Storage Class+Arithmetic or void*/
        declaration_qualifier_list basic_type_name
          { $1->combine(*$2); $$=$1; }
        | basic_type_specifier  storage_class
          { $1->combine(*$2); $$=$1; }
        | basic_declaration_specifier declaration_qualifier
          { $1->combine(*$2); $$=$1; }
        | basic_declaration_specifier basic_type_name
          { $1->combine(*$2); $$=$1; }
        ;

basic_type_specifier:
        basic_type_name            /* Arithmetic or void */
        | type_qualifier_list  basic_type_name
          { $1->combine(*$2); $$=$1; }
        | basic_type_specifier type_qualifier
          { $1->combine(*$2); $$=$1; }
        | basic_type_specifier basic_type_name
          { $1->combine(*$2); $$=$1; }
        ;

sue_declaration_specifier: /* Parse_UserType  */
                           /* Storage Class + struct/union/enum */
        declaration_qualifier_list  elaborated_type_name
	  { $2->combine(*$1); $$=$2; }
        | sue_type_specifier        storage_class
	  { $1->combine(*$2); $$=$1; }
        | sue_declaration_specifier declaration_qualifier
	  { $1->combine(*$2); $$=$1; }
        ;

sue_type_specifier:  /* Parse_UserType*  */
        elaborated_type_name              /* struct/union/enum */
        | type_qualifier_list elaborated_type_name
	  { $2->combine(*$1); $$=$2; }
        | sue_type_specifier  type_qualifier
	  { $1->combine(*$2); $$=$1; }
        ;


typedef_declaration_specifier: /* Parse_TypedefType* */ /*Storage Class + typedef types */
        typedef_type_specifier          storage_class
          { $1->combine(*$2); $$=$1; }
        | declaration_qualifier_list    TYPEDEFname
          { Parse_TypedefType *r = make_typedef($2);
            r->combine(*$1);
            $$ = r;
          }
        | typedef_declaration_specifier declaration_qualifier
          { $1->combine(*$2); $$=$1; }
        ;

typedef_type_specifier:  /* Parse_TypedefType* */  /* typedef types */
        TYPEDEFname
          { $$=make_typedef($1); }
        | type_qualifier_list    TYPEDEFname
          { Parse_TypedefType *r = make_typedef($2);
            r->combine(*$1);
            $$ = r;
          }
        | typedef_type_specifier type_qualifier
          { $1->combine(*$2); $$=$1; }
        ;

storage_class:
        TYPEDEF         { $$=new Parse_Type(P_Typedef); }
        | EXTERN        { $$=new Parse_Type(P_Extern); }
        | STATIC        { $$=new Parse_Type(P_Static); }
        | AUTO          { $$=new Parse_Type(P_Auto); }
        | REGISTER      { $$=new Parse_Type(P_Register); }
        ;

basic_type_name:
        INT             { $$=new Parse_Type(P_Int); }
        | CHAR          { $$=new Parse_Type(P_Char); }
        | SHORT         { $$=new Parse_Type(P_Short); }
        | LONG          { $$=new Parse_Type(P_Long); }
        | FLOAT         { $$=new Parse_Type(P_Float); }
        | DOUBLE        { $$=new Parse_Type(P_Double); }
        | SIGNED        { $$=new Parse_Type(P_Signed); }
        | UNSIGNED      { $$=new Parse_Type(P_Unsigned); }
        | VOID          { $$=new Parse_Type(P_Void); }
        ;

elaborated_type_name:  /* Parse_UserType */
        aggregate_name
        | enum_name
        ;

aggregate_name:  /* Parse_UserType */
        aggregate_key '{' { members_enter_scope(); } member_declaration_list
        '}'
          {
	    /* Insert into the right symboltable. */
	    char const *aname = fresh_anonymous_name() ;
            anonymous_struct_decl($1,aname);
	    /* Make a temporary type for it. */
            $$ = new Parse_UserType(aname,$1);
            members_leave_scope();
          }
        | aggregate_key identifier_or_typedef_name
          {
              members_enter_scope();
	      // Reserve name (incomplete declaration).
              introduce_struct_decl($1, $2);
          }
          '{'  member_declaration_list '}'
          {
	      // Complete declaration.
              update_struct_decl($2);
              $$ = new Parse_UserType($2,$1);
              members_leave_scope();
          }
        | aggregate_key identifier_or_typedef_name
          {
              find_struct_decl($1, $2);
              $$ = new Parse_UserType($2,$1);
          }
        ;

aggregate_key:
        STRUCT          { $$=Struct; }
        | UNION         { $$=Union; }
        ;

/* Returns the list of member declations */
member_declaration_list: /* Plist<MemberDecl>* */
        member_declaration {}
        | member_declaration_list member_declaration
          { }
        ;

member_declaration: /* (nothing) */
          member_declaring_list ';' { delete $1; }
        | member_default_declaring_list ';' { delete $1; }
        ;

member_default_declaring_list: /* decl */ /* doesn't redeclare typedef */
        type_qualifier_list member_identifier_declarator
          { add_member($1,$2); $$=$1; }
        | member_default_declaring_list ',' member_identifier_declarator
          { add_member($1,$3); $$=$1; }
        ;

member_declaring_list:  /* decl  */
        type_specifier member_declarator
          { add_member($1,$2); $$=$1; }
        | member_declaring_list ',' member_declarator
          { add_member($1,$3); $$=$1; }
        ;

member_declarator:  /* Parse_MemberId*  */
        declarator bit_field_size_opt
            { $$=new Parse_MemberId($1,$2); }
        | bit_field_size
            { $$=new Parse_MemberId(NULL,$1); }
        ;

member_identifier_declarator:  /* Parse_MemberId*  */
        identifier_declarator bit_field_size_opt
            { $$=new Parse_MemberId($1,$2); }
        | bit_field_size
            { $$=new Parse_MemberId(NULL,$1); }
        ;

bit_field_size_opt:  /* Expr* */
        /* nothing */
          { $$ = NULL; }
        | bit_field_size
        ;

bit_field_size:  /* Expr* */
        ':' constant_expression
          { $$ = $2; }
        ;

enum_name:  /* Parse_UserType  */
        ENUM '{' enumerator_list '}'
          {
	    // Insert it into the right symboltable.
            char const *aname = fresh_anonymous_name() ;
	    add_enum_decl(aname);
	    // Make a temporary type for it.
            $$ = new Parse_UserType(aname,Enum);
          }
        | ENUM identifier_or_typedef_name '{' enumerator_list '}'
          {
	    add_enum_decl($2);
            $$ = new Parse_UserType($2,Enum);
          }
        | ENUM identifier_or_typedef_name
          {
	    find_enum_decl($2);
            $$ = new Parse_UserType($2,Enum);
          }
        ;

enumerator_list:  /* (nothing) */
        identifier_or_typedef_name enumerator_value_opt
          {
            add_enumconst($1,ccchere,$2);
          }
        | enumerator_list ',' identifier_or_typedef_name enumerator_value_opt
          {
            add_enumconst($3,ccchere,$4);
          }
        ;

enumerator_value_opt:   /* Init* */
        /* Nothing */
          { $$ = NULL; }
        | '=' constant_expression
          { $$ = $2; }
        ;

// This is the full parameter list. Types are extracted later.
parameter_type_list:
          parameter_list { $$ = false; }
        | parameter_list ',' ELLIPSIS { $$ = true; }
        ;

parameter_list: /* types  */
        parameter_declaration
        | parameter_list ',' parameter_declaration
        ;

parameter_declaration:  /* Type*  */
        declaration_specifier		abstract_declarator_opt
          { add_declaration($1,$2); delete $1; }
        | declaration_specifier		identifier_declarator
          { add_declaration($1,$2); delete $1; }
        | declaration_specifier		parameter_typedef_declarator
          { add_declaration($1,$2); delete $1; }
        | type_specifier		abstract_declarator_opt
          { add_declaration($1,$2); delete $1; }
        | type_specifier		identifier_declarator
          { add_declaration($1,$2); delete $1; }
        | type_specifier		parameter_typedef_declarator
          { add_declaration($1,$2); delete $1; }
        | declaration_qualifier_list	abstract_declarator_opt
          { add_declaration($1,$2); delete $1; }
        | declaration_qualifier_list    identifier_declarator
          { add_declaration($1,$2); delete $1; }
        | type_qualifier_list 		abstract_declarator_opt
          { add_declaration($1,$2); delete $1; }
        | type_qualifier_list		identifier_declarator
          { add_declaration($1,$2); delete $1; }
        ;

    /*  ANSI  C  section  3.7.1  states  "An identifier declared as a
    typedef name shall not be redeclared as a parameter".  Hence  the
    following is based only on IDENTIFIERs */

identifier_list: /* (only used for old style functions) */
        IDENTIFIER
          { // Make a plain int declaration. Later on we decide
            // wheter to use this or an explicit parameter declaration.
            Parse_Type pt ;
            add_declaration(&pt,new Parse_Declarator($1,ccchere));
          }
        | identifier_list ',' IDENTIFIER
          { Parse_Type pt ;
            add_declaration(&pt,new Parse_Declarator($3,ccchere));
          }
        ;

identifier_or_typedef_name:   /* str */
        IDENTIFIER
        | TYPEDEFname
        ;

type_name:  /* Type*  */
        type_specifier abstract_declarator_opt
	  { Type* t = $2->maketype($1) ;
            delete $1 ;
            delete $2 ;
            $$ = t ;
          }
        | type_qualifier_list abstract_declarator_opt
	  { Type* t = $2->maketype($1) ;
            delete $1 ;
            delete $2 ;
            $$ = t ;
          }
        ;

initializer_opt:  /* Init */
        /* nothing */
          { $$=NULL; }
        | '=' initializer
          { $$=$2; }
        ;

initializer: /* Init */
        '{' initializer_list '}'
          { $$=$2; }
        | '{' initializer_list ',' '}'
          { $$=$2; }
        | assignment_expression
          { $$=new Init($1); }
        ;

initializer_list: /* Init */
        initializer
          { $$=new Init($1);}
        | initializer_list ',' initializer
            { $$=$1->append($3); }
        ;


/*************************** STATEMENTS *******************************/

statement:
          labeled_statement
        | compound_statement
        | expression_statement
        | selection_statement
        | iteration_statement
        | jump_statement
        ;

labeled_statement:
          identifier_or_typedef_name ':' statement
          {
	      LabelStmt* s = new LabelStmt($1,$3,ccchere);
              add_Label(s);
	      $$=s;
          }
        | CASE constant_expression ':' statement
          {
              $$=new CaseStmt($2->pos,$2,$4);
          }
        | DEFAULT ':' statement
          {
              $$=new DefaultStmt($3->pos,$3);
          }
        ;

compound_statement:
          '{'
          {
              enter_scope();
	  }
          compound_statement1
          '}'
          {
	      $$ = $3;
	      leave_scope();
	  }
        ;

compound_stmt_marker: { $$.set(ccchere); } ;

compound_statement1:
          compound_stmt_marker declaration_list statement_list_opt
          {
              // Pick up the declarations.
              Plist<VarDecl>* dcls = objects.get_list();
	      $$=new CompoundStmt($1,dcls,$3);
          }
        ;

declaration_list:
          /* nothing */
        | declaration_list declaration
        ;

statement_list_opt:
          statement_list
        | /* nothing */
          { $$=new Plist<Stmt>; }
          ;

statement_list:
          statement
          {
              $$=new Plist<Stmt>($1);
          }
        | statement_list statement
          {
              $1->push_back($2);
              $$=$1;
          }
        ;

expression_statement:
          comma_expression_opt ';'
          {
              // The epression could be NULL (nothing).
              if ($1==NULL)
                  $$=new DummyStmt(ccchere);
              else
                  $$=new ExprStmt($1->pos,$1);
          }
        ;

selection_statement:
          IF '(' comma_expression ')' statement
          {
              $$=new IfStmt($3->pos,$3,$5,new DummyStmt(ccchere));
          }
        | IF '(' comma_expression ')' statement ELSE statement
          {
              $$=new IfStmt($3->pos,$3,$5,$7);
          }
        | SWITCH '(' comma_expression ')' statement
          {
              $$=new SwitchStmt($3->pos,$3,$5);
          }
        ;

iteration_statement:
          WHILE '(' comma_expression ')' statement
          {
              $$=new WhileStmt($3->pos,$3,$5);
          }
        | DO statement WHILE '(' comma_expression ')' ';'
          {
              $$=new DoStmt($2->pos,$5,$2);
         }
        | FOR '(' comma_expression_opt ';' comma_expression_opt ';'
                comma_expression_opt ')' statement
          {
              // Expressions can be NULL;
              Position p = $9->pos;
              if ($3 != NULL)
                  p = $3->pos;
              else if ($5 != NULL)
                  p = $5->pos;
              else if ($7 != NULL)
                  p = $7->pos;
              $$=new ForStmt(p,$3,$5,$7,$9);
          }
        ;

jump_statement:
          GOTO identifier_or_typedef_name ';'
          {
              $$=new GotoStmt(ccchere,add_goto($2,ccchere));
          }
        | CONTINUE ';'
          {
              $$=new ContStmt(ccchere);
          }
        | BREAK ';'
          {
              $$=new BreakStmt(ccchere);
          }
        | RETURN comma_expression_opt ';'
          {
              $$=new ReturnStmt(ccchere,$2);
          }
        ;

/***************************** EXTERNAL DEFINITIONS *****************************/

start_symbol:
        { settheflag(); } translation_unit { clean_up_between_files(); }
        ;

translation_unit:
        external_definition
        | translation_unit external_definition
        | IDENTIFIER ':'
          { Diagnostic(FATAL,ccchere)
              << "This is not a C source file" ;
          }
        | IDENTIFIER IDENTIFIER
          { Diagnostic(FATAL,ccchere)
              << "This is not a C source file" ;
          }
        ;

external_definition:
        // After every function definition we dispose all labels.
        function_definition
        | declaration
        | cmix_syntactic_ext ';'
        ;

cmix_syntactic_ext:
	  TYPEDEF CMIXTAG ')' TYPEDEFname { /* ignore attempt to redefine */ }
	| TYPEDEF CMIXTAG ')' IDENTIFIER {
	      Type *thetype = new AbsType($4,$2);
              VarDecl *decl
                  = new VarDecl($4,ccchere,thetype,AbstractType,VarMu);
              names.insert($4,decl);
              assert($2!=NULL);
              objects.push_back(decl);
              directives.add_taboo($4);
          }
        ;

function_head:
          identifier_declarator
        | old_function_declarator
          { enter_old_parameter_scope(); }
          declaration_list
          { leave_old_parameter_scope($1->typemods.back()); $$=$1; }
        ;

function_definition:
                                     function_head
          { begin_FunDef(new Parse_Type(),$1); }
          function_body
        | declaration_specifier      function_head
          { begin_FunDef($1,$2); }
          function_body
        | type_specifier             function_head
          { begin_FunDef($1,$2); }
          function_body
        | declaration_qualifier_list function_head
          { begin_FunDef($1,$2); }
          function_body
        | type_qualifier_list        function_head
          { begin_FunDef($1,$2); }
          function_body
        ;

function_body:
          '{' compound_statement1 '}'
        { end_FunDef($2); }
        ;

declarator:  /* Parse_Identifier*  */
        identifier_declarator
        | typedef_declarator
        ;

typedef_declarator:  /* Parse_Identifier*  */
        paren_typedef_declarator          /* would be ambiguous as parameter*/
        | parameter_typedef_declarator    /* not ambiguous as param*/
        ;

parameter_typedef_declarator:   /* Parse_Declarator*  */
        bare_typedef_declarator
        | clean_typedef_declarator
        ;

    /*  The  following have at least one '*'. There is no (redundant)
    '(' between the '*' and the TYPEDEFname. */

clean_typedef_declarator:  /* Parse_Declarator*  */
        '(' clean_typedef_declarator ')'
          { $$=$2; }
        | '(' clean_typedef_declarator ')' type_postfix
          { $$=$2->prepend_typemods($4); }
        | type_prefix clean_typedef_declarator
          { $$=$2->prepend_typemods($1); }
        | type_prefix bare_typedef_declarator
          { $$=$2->prepend_typemods($1); }
        ;

    /* The following have a redundant '(' placed immediately  to  the
    left of the TYPEDEFname */

paren_typedef_declarator: /* Parse_Declarator */
        '(' inner_paren_typedef_declarator ')'
          { $$=$2; }
        | '(' inner_paren_typedef_declarator ')' type_postfix
          { $$=$2->prepend_typemods($4); }
        | type_prefix paren_typedef_declarator
          { $$=$2->prepend_typemods($1); }
        ;

inner_paren_typedef_declarator: /* Parse_Declarator */
        bare_typedef_declarator
        | paren_typedef_declarator
        ;

bare_typedef_declarator:
        TYPEDEFname
          { $$=new Parse_Declarator($1,ccchere); }
        | TYPEDEFname type_postfix
          { $$=new Parse_Declarator($1,ccchere,$2); }
        ;

identifier_declarator:   /* Parse_Declarator*  */
        mod_identifier_declarator
        | paren_identifier_declarator
        ;

mod_identifier_declarator:   /* Parse_Declarator*  */
        type_prefix paren_identifier_declarator
          { $$=$2->prepend_typemods($1); }
        | paren_identifier_declarator type_postfix
          { $$=$1->prepend_typemods($2); }
        | '(' mod_identifier_declarator ')'
          { $$=$2; }
        | '(' mod_identifier_declarator ')' type_postfix
          { $$=$2->prepend_typemods($4); }
        | type_prefix mod_identifier_declarator
          { $$=$2->prepend_typemods($1); }
        ;

paren_identifier_declarator:   /* Parse_Declarator*  */
        IDENTIFIER
          { $$=new Parse_Declarator($1,ccchere); }
        | '(' paren_identifier_declarator ')'
          { $$=$2; }
        ;

old_function_declarator: /* Parse_Declarator*  */
        paren_identifier_declarator '(' { enter_scope(); } identifier_list ')'
          { $$=$1->prepend_typemods(leave_parameter_scope(false)); }
        | '(' old_function_declarator ')'
          { $$=$2;}
        | '(' old_function_declarator ')' type_postfix
          { $$=$2->prepend_typemods($4); }
        | type_prefix old_function_declarator
          { $$=$2->prepend_typemods($1); }
        ;

abstract_declarator_opt: /* Parse_Declarator* */
        abstract_declarator
        | /* nothing */ { $$=new Parse_Declarator(NULL,ccchere); }

abstract_declarator:  /* Parse_Declarator* */
        type_prefix
          { $$=new Parse_Declarator(NULL,ccchere,$1); }
	| type_postfix
          { $$=new Parse_Declarator(NULL,ccchere,$1); }
        | type_prefix abstract_declarator
          { $$=$2->prepend_typemods($1); }
        | '(' abstract_declarator ')'
          { $$=$2; }
        | '(' abstract_declarator ')' type_postfix
          { $$=$2->prepend_typemods($4); }
	;

type_prefix:  /* Plist<Parse_Typemod>*  */
        '*'
	  { $$=new Plist<Parse_Typemod>(new Parse_Pointer(constvol())); }
	| '*' type_qualifier_list
          { Parse_Pointer *pp = new Parse_Pointer($2->qualifiers);
            delete $2 ;
            $$=new Plist<Parse_Typemod>(pp);
          }
	;

type_postfix:  /* Plist<Parse_Typemod>*  */
        array_postfix
        | '(' ')' // Function type (unknown parameters).
          { Parse_Typemod *tm = new Parse_Funtype(new Plist<VarDecl>(),true,
                                                  new Plist<UserDecl>,ccchere);
            $$ = new Plist<Parse_Typemod>(tm);
          }
        | '(' {enter_scope();} parameter_type_list ')'
	  { $$ = leave_parameter_scope($3); }
        ;

array_postfix:  /* list<Parse_Typemod>* */
        '[' ']'
          { $$=new Plist<Parse_Typemod>(new Parse_Array((Expr*)NULL)); }
        | '[' constant_expression ']'
          { $$=new Plist<Parse_Typemod>(new Parse_Array($2)); }
        | array_postfix '[' constant_expression ']'
	  // Insert sizes in reverse order so the type chain will
	  // get the right direction, ie. "int x[2] -> int x[2][3]"
	  // will be "array 2 of int -> array 2 of array 3 of int".
          { $1->push_front(new Parse_Array($3)); $$=$1; }
        ;

%%
/* ----end of grammar----*/

#ifdef YYDEBUG
        static void settheflag() {
            if (trace_c_parser)
                yydebug=1 ;
        }

#endif

cccParser parser ;
