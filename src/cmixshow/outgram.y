/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: annotation file grammar.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

%{
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
%}

%union{
    char* str;
    unsigned long num;
    int integer;
    struct AnnoTree *tree ;
    struct BlockElement *element ;
}

/* Define terminal tokens */

%token PRODUCER TYPES ATTRIBUTES OUTPUT
%token 'C' 'I'
%token ',' '@' '[' ']' '{' '}'
%token '(' ')' '?' '<' '>' '!' '=' '#'
%token ':'
%token <str> STRING 
%token <num> NUMBER

%type <num> output
%type <str> introduction
/* introduction's semantic value is the producer ID */
%type <integer> block_type
%type <tree> tree group realgroup contents linked target block text linklist
%type <element> blocklist separator

%start file

%%

file:
       introduction { free($1); }
       attr_section
       output_section

introduction:
       PRODUCER ':' STRING TYPES ':' { $$ = $3; }
       ;

introduction:
        introduction STRING '=' NUMBER '(' NUMBER ')' {
            struct TypeRec *type = typerec($4) ;
            if ( type->defined )
                die("Two definitions of type number " PCT_MAGIC,$4);
            type->defined = 1 ;
            type->Name = $2 ;
            if ( $4 == $6 ) {
                type->NextSibling = RootType ;
                RootType = $4 ;
            } else {
                struct TypeRec *parent = typerec($6) ;
                type->NextSibling = parent->FirstChild ;
                parent->FirstChild = $4 ;
            }
            FinishTypeRec($1,type);
        } ;

attr_section:
        ATTRIBUTES ':' attrlist
        ;

attrlist:
        attr
      | attr attrlist
        ;

attr:
        STRING '=' NUMBER
          {}
        ;

output_section:
        OUTPUT ':' outputlist
        ;

outputlist:
        output {
        } |
        output outputlist {
        } ;

output:
        '#' NUMBER {
            $$ = $2;
        } |
        output tree {
            AddAnnoTree($1,$2);
            $$ = $1 ;
        } ;

tree:
        block { $$ = $1 } |
        linked { $$ = $1 } |
        target { $$ = $1 } |
        text { $$ = $1 } ;

block:
        block_type NUMBER '{' blocklist '}' {
            struct AnnoTree *tree = alloctree();
            tree->TreeTag = TREE_LIST ;
            tree->ConsistentBlock = $1 ;
            tree->Indentation = (int)$2 ;
            tree->BlockContent = $4 ;
            $$ = tree ;
        } |
        block_type NUMBER '{' group '}' {
            /* shortcut: single-group blocks are just interpolated */
            $$ = $4 ;
        } ;
        
block_type:
        'C' { $$ = 1 } |
        'I' { $$ = 0 } ;

blocklist:
        group {
            struct BlockElement *elt = alloc(struct BlockElement);
            elt->Contents = $1 ;
            elt->SeparatorTag = SEP_NEVER ;
            elt->Spaces = 0 ;
            elt->NextOffset = 0 ;
            elt->next = NULL ;
            $$ = elt ;
        } |
        group separator blocklist {
            struct BlockElement *elt = $2 ;
            elt->Contents = $1 ;
            elt->next = $3 ;
            $$ = elt ;
        } ;

group:
        /* empty */ {
            $$ = alloctree() ;
        } |
        realgroup {
            $$ = $1 ;
        } ;

realgroup:
        tree {
            $$ = $1 ;
        } |
        tree realgroup {
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_CONCAT ;
            tree->TreeContent1 = $1 ;
            tree->TreeContent2 = $2 ;
            $$ = tree ;
        } ;

separator:
        '?' {
            struct BlockElement *elt = alloc(struct BlockElement) ;
            elt->SeparatorTag = SEP_BREAK ;
            elt->Spaces = 1 ;
            elt->NextOffset = 0 ;
            $$ = elt ;
        } |
        '?' '<' NUMBER ',' NUMBER '>' {
            struct BlockElement *elt = alloc(struct BlockElement);
            elt->SeparatorTag = SEP_BREAK ;
            elt->Spaces = (int)$3 ;
            elt->NextOffset = (int)$5 ;
            $$ = elt ;
        } |
        '!' {
            struct BlockElement *elt = alloc(struct BlockElement);
            elt->SeparatorTag = SEP_NEWLINE ;
            elt->Spaces = 1 ;
            elt->NextOffset = 0 ;
            $$ = elt ;
        } ;

text:
        STRING NUMBER {
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_STRING ;
            tree->TextContent = $1 ;
            tree->Type = $2 ;
            $$ = tree ;
        } ;

linked:
        '@' '[' linklist { $$ = $3; }
target:
        NUMBER ':' contents {
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_LABEL ;
            tree->TreeContent1 = $3 ;
            tree->Label = $1 ;
            $$ = tree ;
        } ;

linklist:
        ']' contents { $$ = $2; } |
        '(' NUMBER ',' NUMBER ')' linklist {
            struct AnnoTree *tree = alloctree() ;
            tree->TreeTag = TREE_LINK ;
            tree->TreeContent1 = $6 ;
            tree->Type = $4 ;
            tree->Label = $2 ;
            $$ = tree ;
        } ;

contents:
        tree { $$ = $1; } |
        separator {
            struct AnnoTree *tree = alloctree() ;
            free($1);
            tree->TreeTag = TREE_STRING ;
            tree->TextContent = "" ;
            tree->Attribute = 0 ;
            $$ = tree;
        }

%%
             
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
