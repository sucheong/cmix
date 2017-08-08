/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Common definitions and constants.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "auxilary.h"
#include "output.h"
#include "commonout.h"
#include "strings.h"
#include "tags.h"

/*********************************************

  These Output Otypes are reserved:
   
*********************************************/

OType* OProgram = new OType("Source program");
OType* OPA  = new OType("Pointer analysis", OProgram);
OType* OAliased = new OType("Possible recursive alias", OPA);
OType* OExternWrite = new OType("Possible writes in external functions",OPA);
OType* OExternRead = new OType("Possible reads in external functions",OPA);
OType* OCG = new OType("Call graph", OProgram);
OType* OBTA = new OType("Binding times",OProgram);
OType* OStatic = new OType("Spectime",OBTA);
OType* ODynamic = new OType("Residual",OBTA);
OType* OStatDyn = new OType("Mixed binding time",OBTA);

OType* ODataflow = new OType("Dataflow analysis",OProgram);
OType* ORead = new OType("Read",ODataflow);
OType* OKill = new OType("Kill",ORead);
OType* OWrite = new OType("Write",ODataflow);

OType* OSA = new OType("Function sharing analysis",OProgram);
OType* OShareable = new OType("Shareable function",OSA);
OType* OUnshareable = new OType("Unshareable function",OSA);

TextAttribute* OKeyword = new TextAttribute("keyword");
TextAttribute* OFunname = new TextAttribute("function");
TextAttribute* OVarname = new TextAttribute("variable");
TextAttribute* OTypename = new TextAttribute("type");
TextAttribute* OComment = new TextAttribute("comment");
TextAttribute* OSymbol = new TextAttribute("symbol");
TextAttribute* OString = new TextAttribute("string");
TextAttribute* OBlank = new TextAttribute("whitespace");
TextAttribute* OLabel = new TextAttribute("label");
TextAttribute* OAnnoText = new TextAttribute("anno");
TextAttribute* OConstant = new TextAttribute("constant");


Plist<TextAttribute>*
allAttributes()
{
  Plist<TextAttribute>* OAttrList = new Plist<TextAttribute>();
  OAttrList->push_back(OKeyword);
  OAttrList->push_back(OFunname);
  OAttrList->push_back(OVarname);
  OAttrList->push_back(OTypename);
  OAttrList->push_back(OComment);
  OAttrList->push_back(OSymbol);
  OAttrList->push_back(OConstant);
  OAttrList->push_back(OBlank);
  OAttrList->push_back(OLabel);
  OAttrList->push_back(OAnnoText);
  return OAttrList;
}

////////////////////////// output //////////////////////////////

Output* newline = new Output();
Output* BREAK  = new Output((unsigned)0, (unsigned)1);
Output* INDENT  = new Output((unsigned)BlockIndentLevel, (unsigned)1);
Output* blank   = new Output(str_blank,OBlank);
Output* eqsign  = new Output(str_is, OSymbol);
Output* dotsign = new Output(str_dot, OSymbol);
Output* lparen  = new Output(str_lparen, OSymbol);
Output* rparen  = new Output(str_rparen, OSymbol);
Output* call    = new Output("call", OSymbol);
Output* comma   = new Output(str_comma, OSymbol);
Output* semi    = new Output(str_semi, OSymbol);
Output* lbrace  = new Output(str_lbrace, OSymbol);
Output* rbracket= new Output(str_rbracket, OSymbol);
Output* lbracket= new Output(str_lbracket, OSymbol);
Output* rbrace  = new Output(str_rbrace, OSymbol);
Output* colon   = new Output(str_colon, OSymbol);
Output* ampersand = new Output(str_addr, OSymbol);
Output* star    = new Output(str_mul, OSymbol);
Output* rarrow  = new Output(str_rarrow, OSymbol);
Output* question= new Output(str_question, OSymbol);
Output* tilde   = new Output(str_tilde, OSymbol);
Output* nullptr = new Output("NULL", OSymbol);
Output* ellipsis = new Output("...", OSymbol);

Output* o_Null  = new Output("",OBlank);

Output* o_if  = new Output(str_if, OKeyword);
Output* o_default  = new Output(str_default, OKeyword);
Output* o_while    = new Output(str_while, OKeyword);
Output* o_do       = new Output(str_do, OKeyword);
Output* o_for      = new Output(str_for, OKeyword);
Output* o_break    = new Output(str_break, OKeyword);
Output* o_case     = new Output(str_case, OKeyword);
Output* o_continue = new Output(str_continue, OKeyword);
Output* o_sizeof  = new Output(str_sizeof, OKeyword);
Output* o_goto  = new Output(str_goto, OKeyword);
Output* o_calloc  = new Output(str_calloc, OKeyword);
Output* o_malloc  = new Output(str_malloc, OKeyword);
Output* o_length  = new Output(str_length, OKeyword);
Output* o_return  = new Output(str_return, OKeyword);
Output* o_switch  = new Output(str_switch, OKeyword);
Output* o_else  = new Output(str_else, OKeyword);
Output* o_free  = new Output(str_free, OKeyword);
Output* o_struct = new Output(str_struct, OKeyword);
Output* o_union  = new Output(str_union, OKeyword);
Output* o_enum   = new Output(str_enum, OKeyword);

Output* o_static   = new Output(str_static, OKeyword);
Output* o_extern   = new Output(str_extern, OKeyword);
Output* o_typedef  = new Output(str_typedef, OKeyword);

Output* o_anything_but  = new Output(str_anything_but, OKeyword);

Output* seqpoint= new Output("/* sequence point */", OComment);

/********************************************/

OutputList::OutputList(unsigned il, Output::BlockType btyp)
{
    os = NULL ;
    ilevel = il ;
    bt = btyp ;
}

void
OutputList::operator+=(Output *o)
{
    if ( o == o_Null )
        return ;
    if ( os == NULL )
        os = new Plist<Output> ;
    else {
        foreach(i,sep,Plist<Output>)
            os->push_back(*i);
    }
    os->push_back(o);
}

OutputList::operator Output*()
{
    if ( os == NULL )
        return o_Null ;
    foreach(i,sep,Plist<Output>)
        os->push_back(*i);
    return inter() ;
}

Output *
OutputList::inter()
{
    if ( os == NULL )
        return o_Null ;
    Output *o = new Output(bt, os, ilevel);
    os = NULL ;
    return o ;
}

/********************************************/

Output*
basetype2output(BaseTypeTag bt)
{
    return new Output(basetype2str(bt), OTypename);
}


Output*
usertype2output(UserTag tag)
{
    return new Output(usertype2str(tag), OTypename);
}

Output*
unary2output(UnOp op)
{
    return new Output(unary2str(op), OSymbol);
}

Output*
binary2output(BinOp op)
{
    return new Output(binary2str(op), OSymbol);
}
