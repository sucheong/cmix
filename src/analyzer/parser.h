/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __PARSER__
#define __PARSER__

#include <stdio.h>
#include "diagnostic.h"
#include "syntax.h"
#include "cpgm.h"

//--------------------------------------
// Flex-bison interface
//
// apart from what bison writes to the header file
//

extern LexerTracker ccchere;
int ccclex();

struct cccLexWrapper {
    bool ok ;
    FILE *f ;
    static int InUse ;
    static void Entry(Position&);
public:
    cccLexWrapper(const char *filename,const char* cppargs,Position ref);
    void parse();
    ~cccLexWrapper();
};

//----------------------------------
//
// The parser class itself

class cccParser {
 private:
    char const* fresh_anonymous_name();
    VarExpr* add_var_ref (char*,Position);
    void add_enum_decl(char const* name);
    void find_enum_decl(char const* name);
    void introduce_struct_decl(UserTag type,char const* name);
    void find_struct_decl(UserTag type,char const* name);
    void update_struct_decl(char const* name);
    void anonymous_struct_decl(UserTag type,char const* name);
    VarDecl* add_enumconst (char* n, Position pos, Expr* value);
    void lone_sue(Parse_UserType*);
    void enter_scope (void);
    void leave_scope (void);
    Plist<Parse_Typemod>* leave_parameter_scope(bool varargs);
    void add_declaration (Parse_GeneralType *,Parse_Declarator *);
    void enter_old_parameter_scope(void);
    void leave_old_parameter_scope(Parse_Typemod*);
    void add_member (Parse_GeneralType *,Parse_MemberId *);
    Parse_TypedefType* make_typedef (char const* name);
    void members_enter_scope (void);
    void members_leave_scope (void);
    void begin_FunDef (Parse_GeneralType *, Parse_Declarator *);
    void end_FunDef (Stmt *);
    void add_Label (LabelStmt* stmt);
    Indirection* add_goto (char* label, const Position);
    void add_initializer (Init* init);
    void clean_up_between_files();
 public:
    CProgram* get_program();
    int ParseNow(); // this is actually yyparse() from bison .. hack, hack..
};

extern cccParser parser ;

#endif
