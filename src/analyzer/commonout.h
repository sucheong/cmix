/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: 
 * History:  Derived from code by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __COMMONOUT__
#define __COMMONOUT__

#include "tags.h"
#include "output.h"

/*********************************************

  These Output Otypes are reserved:
   
*********************************************/

extern OType* OProgram;
  extern OType* OPA;
    extern OType* OAliased;
    extern OType* OExternRead;
    extern OType* OExternWrite;
  extern OType* OCG;
  extern OType* OBTA;
    extern OType* OStatic;
    extern OType* ODynamic;
    extern OType* OStatDyn;
  extern OType* ODataflow;
    extern OType* ORead;
      extern OType* OKill;
    extern OType* OWrite;
  extern OType* OSA;
    extern OType* OShareable ;
    extern OType* OUnshareable ;

extern TextAttribute* OKeyword;
extern TextAttribute* OFunname;
extern TextAttribute* OVarname;
extern TextAttribute* OTypename;
extern TextAttribute* OComment;
extern TextAttribute* OSymbol;
extern TextAttribute* OConstant; // including strings.
extern TextAttribute* OBlank;
extern TextAttribute* OLabel;
extern TextAttribute* OAnnoText;

Plist<TextAttribute>* allAttributes();

#define BlockIndentLevel 4

////////////////// output ////////////////////////

extern Output* newline;
extern Output* BREAK;
extern Output* INDENT;
extern Output* blank;
extern Output* eqsign;
extern Output* dotsign;
extern Output* lparen;
extern Output* rparen;
extern Output* comma;
extern Output* call;
extern Output* semi;
extern Output* lbrace;
extern Output* rbrace;
extern Output* lbracket;
extern Output* rbracket;
extern Output* colon;
extern Output* ampersand;
extern Output* star;
extern Output* rarrow;
extern Output* question;
extern Output* tilde;
extern Output* nullptr;
extern Output* ellipsis;

extern Output* seqpoint;

extern Output* o_Null ; // empty string, ignored in lists

extern Output* o_if;
extern Output* o_else;
extern Output* o_case;
extern Output* o_switch;
extern Output* o_goto;
extern Output* o_return;
extern Output* o_malloc;
extern Output* o_calloc;
extern Output* o_length;
extern Output* o_sizeof;
extern Output* o_free;
extern Output* o_default;
extern Output* o_while;
extern Output* o_do;
extern Output* o_for;
extern Output* o_break;
extern Output* o_default;
extern Output* o_case;
extern Output* o_continue;
extern Output* o_struct;
extern Output* o_union;
extern Output* o_enum;

extern Output* o_static;
extern Output* o_extern;
extern Output* o_typedef;

extern Output* o_anything_but;

/********************************************/

Output* basetype2output(BaseTypeTag);
Output* usertype2output(UserTag);
Output* unary2output(UnOp);
Output* binary2output(BinOp);

/********************************************/

class OutputList {
public:
    Plist<Output> sep ;
private:
    Plist<Output>* os ;
    unsigned ilevel ;
    Output::BlockType bt ;
public:
    OutputList(unsigned,Output::BlockType) ;
    void operator+=(Output *);

    operator Output *(); // i_1 sep .. sep i_n sep.
    Output *inter();     // i_1 sep .. sep i_n.
} ;

#endif
