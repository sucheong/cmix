/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Core C to output format
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdio.h>
#include "outcore.h"
#include "diagnostic.h"
#include "auxilary.h"
#include "options.h"
#include "commonout.h"
#include "strings.h"
#include "directives.h"
#include "renamer.h"

#define debug debugstream_outcore

////////////////////////////////////////////////////////////
// BEFORE MAKING CHANGES TO THIS MODULE, LOOK AT THE
// DEFINITIONS IN commonout.h (and strings.h).
////////////////////////////////////////////////////////////

/************************************************************************
 ************************************************************************

                   LOGISTICS AND INFRASTRUCTURE

 ************************************************************************
 ************************************************************************/

// Mark the given tree as a definition of the given Numbered term.
// Anchors for this is stored in the `anchors' array - they may
// have been created in advance if something needed to refer to
// the program element before it was rendered itself.
//   The `orphans' set consists of those elements that have been
// referred to but not yed defined. (The intention was that if
// that set was nonempty after the whole program had been output
// something is wrong. XXX this check is not yet implemented).
Output *
Outcore::define(Numbered *n,Output* o)
{
  Anchor *a ;
  Pset<Numbered>::iterator I = orphans.find(n) ;
  if ( I ) {
    orphans.erase(I);
    a = anchors[n] ;
    assert(a != NULL);
  } else {
    a = new Anchor(our_type) ;
    assert(anchors[n] == NULL);
    anchors[n] = a ;
  }
  assert(o!=NULL);
  return new Output(a,o);
}

// Decorate a tree with a set of annotations.
Output *
Outcore::decorate(Plist<Anchor> &targets,Output* o)
{
  // No annotations for this tree: return unchanged.
  if ( targets.empty() )
    return o;
  // Make an annotation node.
  if ( o->mark != Output::Anno ) {
    Plist<Anchor> *annos = new Plist<Anchor> ;
    o = new Output(o,annos);
  }
  // Apply the annotations.
  foreach(i,targets,Plist<Anchor>)
    o->anno_anchors()->push_back(*i);
  return o ;
}

// no_names is a placeholder that is used until we learn about the real
// name map (which only happens at the same time we're given a program).
static NameMap no_names;

Outcore::Outcore(OutputContainer &oc, CoreAnnotator &ann,class OType *ot)
  : our_type(ot), anchors(NULL), annotate(ann),
    the_names(&no_names), container(oc)
{
  anchors.accept(Numbered::C_STMT);
  anchors.accept(Numbered::C_DECL);
  anchors.accept(Numbered::C_BASICBLOCK);
  anchors.accept(Numbered::C_JUMP);
  anchors.accept(Numbered::C_USERDEF);
  anchors.accept(Numbered::C_ENUMDEF);
  anchors.accept(Numbered::C_USERMEMB);
}

// Annotate a tree with a link.
Output *
Outcore::crosslink(Output* o,Numbered *n)
{
  Anchor *a ;
  a = anchors[n] ;
  // If no anchor for n has been made, make one.
  if ( a == NULL ) {
    a = new Anchor(our_type) ;
    anchors[n] = a ;
    orphans.insert(n) ;
  }
  // Attach the link
  Plist<Anchor> annos ;
  annos.push_back(a) ;
  return decorate(annos,o);
}

// This routine annotates a tree with the address of the node it represents.
// Enabled by the -dcore-pointers option this is nice for debugging: in
// in combination with -ddump-corec the analyzer outputs an address annotated
// Core C program early enough that one can show it in one window while
// debugging the (still running) program in another.
static void
AddrDecorate(Output *&o, char const *type, void* addr) {
  if ( core_addresses ) {
    Plist<Output> *os = new Plist<Output> ;
    char buffer[40] ;
    sprintf(buffer,"{%s}0x%p ",type,addr);
    os->push_back(new Output(stringDup(buffer),OComment));
    os->push_back(o);
    o = new Output(Output::Consistent,os);
  }
}

#define NAMES (*the_names)

/************************************************************************
 ************************************************************************

                   RENDERING TYPES

 ************************************************************************
 ************************************************************************/

// Put a "volatile" and/or "const" in front of the type.
static void
apply(constvol cv,Output* &o)
{
  if (cv.vol) {
    o = oconcat(BREAK,o);
    o = oconcat(new Output(str_vol,OTypename),o);
  }
  if (cv.cst) {
    o = oconcat(BREAK,o);
    o = oconcat(new Output(str_const,OTypename),o);
  }
}

// Construct a legal C type from internal type representation. The "middle" bit
// is a possibly empty identifier name. This function calls itself recursively,
// accumulating the output in "middle". The precedence parameter is used to
// decide whether parentheses are needed.
Output*
Outcore::render(C_Type &t, Output* middle, bool precedence)
{
  Output* o = NULL;
  Plist<Anchor> annos ;
  annotate(t,annos,this);
  
  switch(t.tag) {
  case Primitive:
  case Abstract:
    // A leaf. The rest of the output has been accumulated in "middle".
    o = new Output(t.primitive(),OTypename);
    o = decorate(annos,o);
    if (middle!=NULL) {
      o = oconcat(o,BREAK);
      o = oconcat(o,middle);
    }
    break;
  case StructUnion:
    // A leaf. The rest of the output has been accumulated in "middle".
    // Make the tag (struct/union/enum).
    o = t.user_def()->isUnion ? o_union : o_struct ;
    o = oconcat(o,blank);
    o = oconcat(o,new Output(NAMES[t.user_def()],OTypename));
    o = crosslink(o,t.user_def());
    o = decorate(annos,o);
    if (middle!=NULL) {
      o = oconcat(o,BREAK);
      o = oconcat(o,middle);
    }
    break;
  case EnumType:
    o = oconcat(o_enum,blank);
    o = oconcat(o,new Output(NAMES[t.enum_def()],OTypename));
    o = crosslink(o,t.enum_def());
    o = decorate(annos,o);
    if (middle!=NULL) {
      o = oconcat(o,BREAK);
      o = oconcat(o,middle);
    }
    break;
  case FunPtr:
  case Pointer:
    // Append a star to the "middle" and descend.
    o = decorate(annos,star);
    if (middle!=NULL) {
      o = oconcat(o,middle);
    }
    apply(t.qualifiers(),o);
    o = render(*t.ptr_next(),o,true);
    break;
  case Array:
    o = decorate(annos,lbracket);
    if ( t.hasSize() )
      o = oconcat(o,render(*t.array_size())) ;
    o = oconcat(o,decorate(annos,rbracket));
    if (middle!=NULL) {
      // Put parentheses if necessary.
      if (precedence) {
        middle = oconcat(lparen,middle);
        middle = oconcat(middle,rparen);
      }
      o = oconcat(middle,o);
    }
    apply(t.qualifiers(),o);
    // Descend.
    o = render(*t.array_next(),o);
    break;
  case Function: {
    // The parameter list. The separator is ",<break(0)>".
    OutputList parlist(0,Output::Consistent);
    parlist.sep.push_back(decorate(annos,comma));
    parlist.sep.push_back(BREAK);
    foreach(p,t.fun_params(),Plist<C_Type>)
      parlist += render(**p);
    o = decorate(annos,lparen);
    o = oconcat(o,parlist.inter());
    o = oconcat(o,decorate(annos,rparen));
    // The funtion name.
    if (middle!=NULL) {
      if (precedence) {
        middle = oconcat(lparen,middle);
        middle = oconcat(middle,rparen);
      }
      o = oconcat(middle,o);
    }
    // Descend to the return type.
    o = render(*t.fun_ret(),o);
    break; }
  }
  return o;
}

/************************************************************************
 ************************************************************************

                   RENDERING DECLARATIONS

 ************************************************************************
 ************************************************************************/

Output*
Outcore::render(C_Decl &d)
{
  Output* o = NULL;
  // Get the annotations.
  Plist<Anchor> annos ;
  annotate(d,annos,this);

  Plist<Output>* os = new Plist<Output>();
  switch (d.tag) {
  case VarDcl:
    // Link the annotations to the name of the variable.
    o = new Output(NAMES[d], OVarname);
    o = decorate(annos,o);
    apply(d.qualifiers,o);
    // Wrap the type around.
    os->push_back(render(*d.type,o));
    // Initializers
    if ( d.hasInit() ) {
      os->push_back(blank);
      os->push_back(eqsign);
      os->push_back(INDENT);
      os->push_back(render(*d.init()));
    }
    break;
  case FunDf: {
    // Create the name followed by parameters. Append the "under dynamic
    // control" to the name.
    o = new Output(NAMES[d], OFunname) ;
    o = decorate(annos,o);
    os->push_back(o);
    os->push_back(lparen);
    // The separator is ",<break(0)>".
    OutputList parlist(0,Output::Consistent);
    parlist.sep.push_back(comma);
    parlist.sep.push_back(BREAK);
    foreach(i,d.fun_params(),Plist<C_Decl>)
      parlist += render(**i);
    if (d.type->fun_varargs())
      parlist += new Output("...", OSymbol) ;
    os->push_back(parlist.inter()); 
    os->push_back(rparen);
    o = new Output(Output::Consistent,os);
    // Then wrap the type around.
    os = new Plist<Output>();
    os->push_back(render(*d.type->fun_ret(),o));
    // Prepare for the body.
    os->push_back(BREAK);

    Plist<Output>* os1 = new Plist<Output>();
    os1->push_back(lbrace);
    os1->push_back(INDENT);
    // The body.
    // Do the local declarations: separator is ";\n". The whole block is
    // indented by BlockIndentLevel.
    OutputList locallist(0,Output::Consistent);
    locallist.sep.push_back(semi);
    locallist.sep.push_back(newline);
    foreach(l,d.fun_locals(),Plist<C_Decl>)
      locallist += render(**l);
    os1->push_back(locallist);
    os->push_back(new Output(Output::Consistent, os1));
    if (d.fun_locals().empty()) os->push_back(BREAK);
    // Do the statemens.
    foreach(bb, d.blocks(), Plist<C_BasicBlock>)
      os->push_back(render(**bb));
    // End the body.
    os->push_back(new Output((unsigned)0, (unsigned)0));
    os->push_back(rbrace);
    os->push_back(BREAK);
    break; }
  case ExtFun:
  case ExtState:
    assert(0);
  }
  o = new Output(Output::Consistent,os) ;
  AddrDecorate(o,"C_Decl",(void*)&d);
  return define(&d,o);
}

/************************************************************************
 ************************************************************************

                   RENDERING TYPE DEFINITIONS

 ************************************************************************
 ************************************************************************/

Output*
Outcore::render(C_EnumDef &ed)
{
  Plist<Anchor> annos ;
  annotate(ed,annos,this);

  Output *o ;
  o = oconcat(o_enum,blank);
  o = oconcat(o,new Output(NAMES[ed],OTypename));
  o = decorate(annos,o);
  o = oconcat(o,blank) ;
  o = oconcat(o,lbrace) ;

  OutputList memblist(0,Output::Inconsistent) ;
  memblist.sep.push_back(comma);
  memblist.sep.push_back(BREAK);
  foreach(m,ed.members(),Plist<C_UserMemb>) {
    Plist<Anchor> mannos ;
    annotate(**m,mannos,this);
    Output *oo = new Output(NAMES[*m],OVarname);
    oo = decorate(mannos,oo);
    if ( m->hasValue() ) {
      oo = oconcat(oo,eqsign);
      oo = oconcat(oo,render(*m->value()));
    }
    memblist += define(*m,oo) ;
  }

  o = oconcat(o,memblist.inter()) ;
  o = oconcat(o,rbrace) ;
  AddrDecorate(o,"C_EnumDef",(void*)&ed);
  return define(&ed,o) ;
}

Output*
Outcore::render(C_UserDef &ud)
{
  Plist<Anchor> annos ;
  annotate(ud,annos,this);

  Output *o ;
  Plist<Output>* os = new Plist<Output>();
  o = oconcat(ud.isUnion ? o_union : o_struct,blank);
  o = oconcat(o,new Output(NAMES[ud],OTypename));
  o = decorate(annos,o);
  os->push_back(o);
  os->push_back(blank);
  os->push_back(lbrace);
  os->push_back(INDENT);

  OutputList memblist(0,Output::Consistent) ;
  // separator = semicolon + break(offset 0).
  memblist.sep.push_back(semi);
  memblist.sep.push_back(BREAK);
  foreach(i,ud,C_UserDef) {
    Plist<Anchor> mannos ;
    annotate(*i.name(),*i.type(),mannos,this);
    o = new Output(NAMES[i.name()],OVarname);
    o = decorate(mannos,o);
    apply(i.name()->qualifiers,o);
    o = render(*i.type(),o);
    memblist += o;
  }
  os->push_back(memblist);
  os->push_back(BREAK);
  os->push_back(rbrace);
  o = new Output(Output::Inconsistent,os);
  AddrDecorate(o,"C_UserDef",(void*)&ud);
  return define(&ud,o);
}

/************************************************************************
 ************************************************************************

                   RENDERING JUMPS, STATEMENTS, and BASIC BLOCKS

 ************************************************************************
 ************************************************************************/

static Output*
makeLabel(C_BasicBlock &bb)
{
  return oconcat(new Output("L",OLabel),
                 new Output(long2a(bb.Numbered_ID),OLabel));
}

Output *
Outcore::xref(C_BasicBlock &bb)
{
  return crosslink(makeLabel(bb),&bb);
}

Output*
Outcore::render(C_Jump &j)
{
  Plist<Output>* os = new Plist<Output>();
  Plist<Anchor> annos ;
  annotate(j,annos,this);
  switch (j.tag) {
  case C_If:	  // if (e) lab1 lab2;
    os->push_back(decorate(annos,o_if));
    os->push_back(blank);
    os->push_back(lparen);
    os->push_back(render(*j.cond_expr()));
    os->push_back(rparen);
    os->push_back(INDENT);
        
    os->push_back(o_goto);
    os->push_back(blank);
    os->push_back(xref(*j.cond_then()));
    os->push_back(semi);
    os->push_back(BREAK);

    os->push_back(decorate(annos,o_else));
    os->push_back(INDENT);
    os->push_back(o_goto);
    os->push_back(blank);
    os->push_back(xref(*j.cond_else()));
    break;
  case C_Goto: {    // goto lab;
    os->push_back(decorate(annos,o_goto));
    os->push_back(blank);
    os->push_back(xref(*j.goto_target()));
    break; }
  case C_Return: { // return e;
    os->push_back(decorate(annos,o_return));
    if ( j.hasExpr() ) {
      os->push_back(INDENT);
      os->push_back(render(*j.return_expr()));
    }
    break; }
  }
  Output* the_statement = new Output(Output::Consistent,os);
  AddrDecorate(the_statement,"C_Jump",(void*)&j);
  return define(&j,the_statement);
}

Output*
Outcore::render(C_Stmt &s)
{
  Plist<Output>* os = new Plist<Output>();
  Plist<Anchor> annos ;
  annotate(s,annos,this);
  if ( s.hasTarget() ) {
    Plist<Anchor> target_annos ;
    annotate(*s.target(),true,target_annos,this);
    os->push_back(deref(*s.target(),false,target_annos,4));
    os->push_back(blank);
    os->push_back(decorate(annos,eqsign));
    os->push_back(INDENT);
    annos.clear() ;
  }
  switch (s.tag) {
  case C_Assign: // target = e;
    os->push_back(render(*s.assign_expr(),4));
    break;
  case C_Call: {   // target = e0(e1,...eN);
    if ( !s.hasTarget() && !annos.empty() ) {
      os->push_back(decorate(annos,call));
      os->push_back(blank);
    }
    os->push_back(render(*s.call_expr(),30));
    os->push_back(lparen);
    OutputList args(0,Output::Inconsistent);
    args.sep.push_back(comma);
    args.sep.push_back(BREAK);
    foreach(i,s.call_args(),Plist<C_Expr>)
      args += render(**i,2) ;
    os->push_back(args.inter());
    os->push_back(rparen);
    break;
  }
  case C_Alloc: { // x = calloc(n,T);
    Plist<Output> *os2 = new Plist<Output> ;
    os2->push_back(decorate(annos, s.isMalloc() ? o_malloc : o_calloc ));
    os2->push_back(blank);
    os2->push_back(render(*s.alloc_objects()->type));
    os->push_back(new Output(Output::Consistent,os2));
    break; }
  case C_Free:   // free(e);
    os->push_back(decorate(annos,o_free));
    os->push_back(lparen);
    os->push_back(render(*s.free_expr()));
    os->push_back(rparen);
    break;
  case C_Sequence:
    os->push_back(decorate(annos,seqpoint));
    break;
  }
  os->push_back(semi);
  Output* the_statement = new Output(Output::Consistent,os);
  AddrDecorate(the_statement,"C_Stmt",(void*)&s);
  return define(&s,the_statement);
}

Output*
Outcore::render(C_BasicBlock &bb)
{
  Plist<Output>* os = new Plist<Output>();
  Plist<Anchor> annos ;
  annotate(bb,annos,this);
  Output *label = decorate(annos,oconcat(makeLabel(bb),colon)) ;
  AddrDecorate(label,"C_BasicBlock",(void*)&bb);
  os->push_back(label);
  os->push_back(INDENT);
  // Print all statements in this block.
  Plist<Output>* os1 = new Plist<Output>();
  foreach(s, bb.getBlock(),Plist<C_Stmt>) {
    // Dont print sequence points.
    if ((*s)->tag != C_Sequence) {
      os1->push_back(render(**s));
      os1->push_back(newline);
    }
  }
  // Print the exit statement.
  os1->push_back(render(*bb.exit()));
  os1->push_back(semi);
  os1->push_back(newline);
  os->push_back(new Output(Output::Consistent, os1, 0));

  Output *the_block = new Output(Output::Consistent,os);
  return define(&bb,the_block);
}

/************************************************************************
 ************************************************************************

                   RENDERING INITIALIZERS

 ************************************************************************
 ************************************************************************/

Output *
Outcore::render(C_Init &i)
{
  switch(i.tag) {
  default:
    assert(0);
  case Simple:
    return render(*i.simple_init(),2);
  case StringInit:
    return new Output(i.string_init(),OConstant);
  case FullyBraced:
  case SloppyBraced:
    OutputList contents(0,i.tag == FullyBraced ?
                        Output::Consistent : Output::Inconsistent) ;
    contents.sep.push_back(comma);
    contents.sep.push_back(BREAK);
    foreach(j,i.braced_init(),Plist<C_Init>)
      contents += render(**j);
    Plist<Output> *os = new Plist<Output> ;
    os->push_back(lbrace);
    os->push_back(blank);
    os->push_back(contents.inter());
    os->push_back(rbrace);
    return new Output(Output::Consistent,os);
  }
}

/************************************************************************
 ************************************************************************

                   RENDERING EXPRESSIONS

 ************************************************************************
 ************************************************************************/

// Wrap an expression in parentheses if necessary and make it
// into a list.
static Output*
bracket(Plist<Output>* os, int innerprec, int outerprec) {
  if ( outerprec > innerprec ) {
    os->push_front(lparen);
    os->push_back(rparen);
  }
  return new Output(Output::Consistent,os,2);
}

// Outcore::deref can work in two different modes:
//  1) The /address/ of the lvalue (i.e. the nominal value
//     of the Core C expression) is interesting.
//  2) The value /stored/ in the pointed-to object (i.e. the
//     nominal value of the printed expression) is interesting.
// In both cases, the caller provides suitable annotations.
// The two modes differ in the member selection recursion, and
// also behave differently in hardcore_mode ;

Output *
Outcore::deref(C_Expr &expr,bool for_address,Plist<Anchor> &annos,int prec)
{
  Output *o ;
  if ( for_address || !hardcore_mode )
    switch(expr.tag) {
    case C_Var:
      o = new Output(NAMES[expr.var()], OVarname);
      o = crosslink(o,expr.var());
      o = decorate(annos,o);
      return o ;
    case C_PtrArith:
      if ( hardcore_mode || expr.binary_op() != Add )
        break;
      else {
        Plist<Output> *os = new Plist<Output> ;
        os->push_back(render(*expr.binary_expr1(),29));
        os->push_back(decorate(annos,lbracket));
        os->push_back(render(*expr.binary_expr2()));
        os->push_back(decorate(annos,rbracket));
        return bracket(os,29,prec);
      }
    case C_Member: {
      C_Expr* e = expr.subexpr() ;
      C_UserMemb* m = expr.struct_name() ;
      o = new Output(NAMES[m],OVarname);
      Plist<Output> *os = new Plist<Output> ;
      if ( hardcore_mode || e->tag == C_DeRef ) {
        o = oconcat(rarrow,o);
        os->push_back(render(*e,29));
      } else {
        o = oconcat(dotsign,o);
        Plist<Anchor> next ;
        // when printing the expression for its address we can
        // use the normal annotations (lval=false)
        // but when printing for its value there is no Core C
        // expression that has the right annotations, so we
        // cheat and set lval=true.
        annotate(*e,!for_address,next,this);
        os->push_back(deref(*e,for_address,next,29));
      }
      os->push_back(decorate(annos,o));
      return bracket(os,29,prec); }
    default:
      break ;
    }
    
  Plist<Output> *os = new Plist<Output> ;
  os->push_back(decorate(annos,star));
  os->push_back(render(expr,27));
  return bracket(os,27,prec);
}
   
Output*
Outcore::render(C_Expr &expr, int prec)
{
  Plist<Anchor> annos ;
  annotate(expr,false,annos,this);

  switch (expr.tag) {
  case C_Cnst:
    return decorate(annos,new Output(expr.cnst(),OConstant));
  case C_EnumCnst:
    return decorate(annos,
                    crosslink(new Output(NAMES[expr.enum_cnst()],OConstant),
                              expr.enum_cnst()));
  case C_Null: {
    Output *o = decorate(annos,nullptr);
    if ( hardcore_mode ) {
      o = oconcat(o,lparen) ;
      o = oconcat(o,render(*expr.type)) ;
      o = oconcat(o,rparen) ;
    }
    return o ; }
  case C_ExtFun: {
    Output *o = new Output(expr.var()->get_name(), OVarname);
    o = decorate(annos,o);
    return define(expr.var(),o) ; }
  case C_FunAdr: {
    Output *o = new Output(NAMES[expr.var()], OVarname);
    o = crosslink(o,expr.var());
    o = decorate(annos,o);
    return o ; }
  case C_Var:
  case C_Member: {
    Plist<Output> *os = new Plist<Output> ;
    os->push_back(decorate(annos,ampersand));
    os->push_back(deref(expr,true,annos,27));
    return bracket(os,27,prec); }
  case C_DeRef:
    return deref(*expr.subexpr(),false,annos,prec);
  case C_Unary: {
    Plist<Output> *os = new Plist<Output> ;
    os->push_back(decorate(annos,unary2output(expr.unary_op())));
    os->push_back(render(*expr.subexpr(),27));
    return bracket(os,27,prec); }
  case C_PtrArith:
  case C_PtrCmp:
  case C_Binary: {
    Plist<Output> *os = new Plist<Output>;
    int myprec = binary2prec(expr.binary_op());
    os->push_back(render(*expr.binary_expr1(),myprec));
    os->push_back(blank);
    os->push_back(decorate(annos,binary2output(expr.binary_op())));
    os->push_back(BREAK);
    os->push_back(render(*expr.binary_expr2(),myprec+1));
    return bracket(os,myprec,prec); }
  case C_SizeofE: {
    Plist<Output> *os = new Plist<Output>;
    os->push_back(decorate(annos,o_sizeof));
    os->push_back(blank);
    os->push_back(render(*expr.subexpr(),27));
    return bracket(os,27,prec); }
  case C_SizeofT: {
    Plist<Output> *os = new Plist<Output>;
    os->push_back(decorate(annos,o_sizeof));
    os->push_back(lparen);
    os->push_back(render(*expr.sizeof_type()));
    os->push_back(rparen);
    return bracket(os,27,prec); }
  case C_Array:
    if ( hardcore_mode ) {
      Plist<Output> *os = new Plist<Output>;
      os->push_back(decorate(annos,new Output("<DECAY>",OSymbol)));
      os->push_back(render(*expr.subexpr(),27));
      return bracket(os,27,prec);
    } else {
      annos.clear();
      // Let the 'a' in 'a[42]' show non-decayed data
      annotate(*expr.subexpr(),true,annos,this);
      return deref(*expr.subexpr(),false,annos,prec);
    }
  case C_Cast:
    Plist<Output> *os = new Plist<Output>;
    os->push_back(decorate(annos,lparen));
    os->push_back(render(*expr.type));
    os->push_back(decorate(annos,rparen));
    os->push_back(render(*expr.subexpr(),27));
    return bracket(os,27,prec);
  }

  Diagnostic(INTERNAL,expr.pos)
    << "fell through switch at Outcore::render(C_Expr)" ;
  return NULL ;
}

/************************************************************************
 ************************************************************************

                   RENDERING THE ENTIRE PROGRAM

 ************************************************************************
 ************************************************************************/

Output *
Outcore::operator()(C_Pgm &pgm,NameMap &nametable)
{
  the_names = &nametable ;
    
  Plist<Output>* os = new Plist<Output>();
  if (!pgm.enumtypes.empty()) {
    os->push_back(new Output("/* Enumerated types */",OComment));
    os->push_back(BREAK);
    foreach(ed, pgm.enumtypes, Plist<C_EnumDef>) {
      os->push_back(render(**ed));
      os->push_back(semi);
      os->push_back(BREAK);
    }
  }
  
  if (!pgm.usertypes.empty()) {
    os->push_back(new Output("/* Structs and unions */",OComment));
    os->push_back(BREAK);
    foreach(ud, pgm.usertypes, Plist<C_UserDef>) {
      os->push_back(render(**ud));
      os->push_back(semi);
      os->push_back(BREAK);
    }
  }
  
  if (!pgm.globals.empty()) {
    os->push_back(new Output("/* Global variables */",OComment));
    os->push_back(BREAK);
    foreach(var, pgm.globals, Plist<C_Decl>) {
      if (debug^1) debug << "[" << NAMES[*var];
      switch((*var)->varmode()) {
      case VarExtDefault:
      case VarExtResidual:
      case VarExtSpectime:
        os->push_back(o_extern);
        os->push_back(blank);
        break ;
      case VarVisResidual:
      case VarVisSpectime:
        break ;
      default:
        os->push_back(o_static);
        os->push_back(blank);
        break ;
      }
      os->push_back(render(**var));
      os->push_back(semi);
      os->push_back(BREAK);
      if (debug^1) debug << "]";
    }
  }
  os->push_back(BREAK);
  os->push_back(new Output("/* Functions */",OComment));
  os->push_back(BREAK);
  foreach(fun, pgm.functions, Plist<C_Decl>) {
    if (debug^1) debug << "[" << NAMES[*fun] ;
    os->push_back(render(**fun));
    os->push_back(BREAK);
    os->push_back(newline);
    if (debug^1) debug << "]";
  }
  os->push_back(BREAK);
  os->push_back(new Output("/* Entry point wrappers */",OComment));
  os->push_back(BREAK);
  foreach(fun2, pgm.generators, Plist<C_Decl>) {
    if (debug^1) debug << "[" << NAMES[*fun2] ;
    os->push_back(render(**fun2));
    os->push_back(BREAK);
    if (debug^1) debug << "]";
  }

  the_names = &no_names ;
  return new Output(Output::Consistent,os) ;
}

/************************************************************************
 ************************************************************************

                   ANNOTATOR BASE CLASSES

 ************************************************************************
 ************************************************************************/

#define SIMPLE(T) \
  void CoreAnnotator::operator()(T &t, Plist<Anchor> &a, Outcore *o) \
  { next(t,a,o); }

SIMPLE(C_Type)
SIMPLE(C_Init)
SIMPLE(C_Decl)
SIMPLE(C_EnumDef)
SIMPLE(C_UserMemb)
SIMPLE(C_UserDef)
SIMPLE(C_Stmt)
SIMPLE(C_Jump)
SIMPLE(C_BasicBlock)
    
void
CoreAnnotator::operator()(C_UserMemb &um,C_Type &t,Plist<Anchor> &a,Outcore *o)
{
  next(um,t,a,o);
}

void
CoreAnnotator::operator()(C_Expr &e,bool lval, Plist<Anchor> &a,Outcore *o)
{
  next(e,lval,a,o);
}

CoreAnnotator::CoreAnnotator(CoreAnnotator &nxt)
  : next(nxt)
{
}

CoreAnnotator::~CoreAnnotator()
{
}

NoCoreAnno::NoCoreAnno()
  : CoreAnnotator(*this)
{
}

NoCoreAnno::~NoCoreAnno()
{
}
