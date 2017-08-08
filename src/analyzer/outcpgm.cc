/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Outputting BT-annotated ISO C
 * History:  Derived from code by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "analyses.h" // we have to know about BTresults.
#include "cpgm.h"
#include "commonout.h"
#include "diagnostic.h"
#include "directives.h"
#include "options.h"

#define debug debugstream_outcpgm

////////////////////////////////////////////////////////////////
// BEFORE MAKING CHANGES TO THIS MODULE, LOOK AT THE
// DEFINITIONS IN commonout.h AND strings.h
////////////////////////////////////////////////////////////

/************************************************************************
 ************************************************************************

                   COLOR ANNOTATIONS FOR BINDING TIMES

 ************************************************************************
 ************************************************************************/

enum MixedReason { USERTYPE, BOOLEAN, NOMIX, MixedReasonEnd };

struct OutCpgm {
    OutputContainer &container ;
    BTresult &bt ;

    MixedReason mixreason ;
    
    Plist<Anchor> *TheStaticAnchor ;
    Plist<Anchor> *TheDynamicAnchor ;
    Plist<Anchor> *TheMixedAnchors[MixedReasonEnd] ;

    Output *decorate(Plist<C_Type>&,Output *) ;
    void decorate(Plist<C_Type>&,Plist<Output> *&) ;

    OutCpgm(OutputContainer &,BTresult &);
};

Output *
OutCpgm::decorate(Plist<C_Type> &btannos,Output *o) {
    bool metStatic = false ;
    bool metDynamic = false ;
    foreach(i,btannos,Plist<C_Type>) {
        if ( !bt.hasBindingTime(*i) )
            continue ;
        if ( bt.Static(*i) ) {
            if ( metDynamic )
                goto mixed_bt ;
            metStatic = true ;
        } else {
            if ( metStatic )
                goto mixed_bt ;
            metDynamic = true ;
        }
    }
    if ( metStatic )
        return new Output(o,TheStaticAnchor);
    else if ( metDynamic )
        return new Output(o,TheDynamicAnchor);
    else
        return o ;
 mixed_bt:
    return new Output(o,TheMixedAnchors[mixreason]);
}

void
OutCpgm::decorate(Plist<C_Type> &btannos,Plist<Output>* &os) {
    if ( os->size() == 1 )
        os->begin() << decorate(btannos,os->front()) ;
    else {
        Output *o = new Output(Output::Consistent,os) ;
        os = new Plist<Output>(decorate(btannos,o));
    }
}

OutCpgm::OutCpgm(OutputContainer &oc,BTresult &btr)
    : container(oc), bt(btr)
{
    mixreason = NOMIX ;
    
    Output *o ;
    
    TheStaticAnchor = new Plist<Anchor>(new Anchor(OStatic));
    container.add(new Output(TheStaticAnchor->front(),o_Null),OStatic);

    TheDynamicAnchor = new Plist<Anchor>(new Anchor(ODynamic));
    container.add(new Output(TheDynamicAnchor->front(),o_Null),ODynamic);

    TheMixedAnchors[USERTYPE] = new Plist<Anchor>(new Anchor(OStatDyn));
    if(1){
        Plist<Output> *os = new Plist<Output> ;
        os->push_back(new Output("This type is has different binding times",
                                 OAnnoText));
        os->push_back(newline);
        os->push_back(new Output("in different instances of the struct/union",
                                 OAnnoText));
        o = new Output(Output::Consistent,os);
    }
    container.add(new Output(TheMixedAnchors[USERTYPE]->front(),o),OStatDyn);

    TheMixedAnchors[BOOLEAN] = new Plist<Anchor>(new Anchor(OStatDyn));
    if(1){
        Plist<Output> *os = new Plist<Output> ;
        os->push_back(new Output("This boolean expression has complex "
                                 "control flow with",OAnnoText));
        os->push_back(newline);
        os->push_back(new Output("multiple binding times. Use -s switch "
                                 "to C-Mix for details",OAnnoText));
        o = new Output(Output::Consistent,os);
    }
    container.add(new Output(TheMixedAnchors[BOOLEAN]->front(),o),OStatDyn);

    TheMixedAnchors[NOMIX] = new Plist<Anchor>(new Anchor(OStatDyn));
    if(1){
        Plist<Output> *os = new Plist<Output> ;
        os->push_back(new Output("There should only be a single binding "
                                 "time here.",OAnnoText));
        os->push_back(newline);
        os->push_back(new Output("This may be an internal error in C-Mix!",
                                 OAnnoText));
        o = new Output(Output::Consistent,os);
    }
    container.add(new Output(TheMixedAnchors[NOMIX]->front(),o),OStatDyn);
}

/************************************************************************
 ************************************************************************

                   RENDERING TYPES

 ************************************************************************
 ************************************************************************/

// Insert a const/volatile (if any) in the beginning a list
static void
Constvol(Plist<Output>* os, Type* t)
{
    assert(os != NULL);
    if (t->cv.cst) {
        os->push_front(blank);
        os->push_front(new Output("const",OKeyword));
    }
    if (t->cv.vol) {
        os->push_front(blank);
        os->push_front(new Output("volatile",OKeyword));
    }
}

Output* AbsType::output(OutCpgm &env, Output* middle, bool) {
    Plist<Output>* os = new Plist<Output>(new Output(name,OTypename));
    Constvol(os,this);
    env.decorate(btannos,os);
    if (middle!=NULL) {
        os->push_back(blank);
        os->push_back(middle);
    }
    return new Output(Output::Consistent,os);
}

Output*
BaseType::output(OutCpgm &env, Output* middle, bool /*precedence*/)
{
    Plist<Output>* os = new Plist<Output>(basetype2output(tag));
    Constvol(os,this);
    env.decorate(btannos,os);
    if (middle!=NULL) {
        os->push_back(blank);
        os->push_back(middle);
    }
    return new Output(Output::Consistent,os);
}

Output*
PtrType::output(OutCpgm &env, Output* middle, bool /*precedence*/)
{
    Plist<Output>* os = new Plist<Output>(star);
    Constvol(os,this);
    env.decorate(btannos,os);
    if (middle!=NULL) {
        os->push_back(middle);
    }
    return next->output(env,new Output(Output::Consistent,os),true);
}

Output*
ArrayType::output(OutCpgm &env, Output* middle, bool precedence)
{
    Plist<Output>* os = new Plist<Output>();
    assert(cv.subsetof(constvol()));
    // Put parentheses around the middle part.
    if (middle!=NULL) {
        if (precedence) 
            os->push_back(lparen);
        os->push_back(middle);
        if (precedence) 
            os->push_back(rparen);
    }
    // Append brackets.
    os->push_back(env.decorate(btannos,lbracket));
    if (size!=NULL) os->push_back(size->output(env));
    os->push_back(env.decorate(btannos,rbracket));
    return next->output(env,new Output(Output::Consistent,os),false);
}

Output*
FunType::output(OutCpgm &env, Output* middle, bool precedence)
{
    Plist<Output>* os = new Plist<Output>();
    assert(cv.subsetof(constvol()));
    // The funtion name
    if (middle!=NULL || precedence) {
        if (precedence) 
            os->push_back(lparen);
        if (middle!=NULL)
            os->push_back(middle);
        if (precedence) 
            os->push_back(rparen);
    }
    // The parameter list.
    // output each parameter w.o. declaration (only the type).
    os->push_back(env.decorate(btannos,lparen));
    OutputList parlist(0,Output::Inconsistent) ;
    parlist.sep.push_back(env.decorate(btannos,comma));
    foreach(i,*params,Plist<Type>)
        parlist += i->output(env);
    if ( varargs )
      parlist += ellipsis ;
    os->push_back(parlist.inter());
    os->push_back(rparen);
    return ret->output(env, new Output(Output::Consistent,os), false);
}

Output*
StructType::output(OutCpgm &env, Output* middle, bool /*precedence*/)
{
    Plist<Output>* os = new Plist<Output>();
    Constvol(os,this);
    os->push_back(o_struct);
    os->push_back(blank);
    os->push_back(new Output(def->name,OVarname));
    env.decorate(btannos,os);
    if (middle!=NULL) {
        os->push_back(blank);
        os->push_back(middle);
    }
    return new Output(Output::Consistent,os);
}

Output*
UnionType::output(OutCpgm &env, Output* middle, bool /*precedence*/)
{
    Plist<Output>* os = new Plist<Output>();
    Constvol(os,this);
    os->push_back(o_union);
    os->push_back(blank);
    os->push_back(new Output(def->name,OVarname));
    env.decorate(btannos,os);
    if (middle!=NULL) {
        os->push_back(blank);
        os->push_back(middle);
    }
    return new Output(Output::Consistent,os);
}

Output*
EnumType::output(OutCpgm &env, Output* middle, bool /*precedence*/)
{
    Plist<Output>* os = new Plist<Output>();
    Constvol(os,this);
    os->push_back(o_enum);
    os->push_back(blank);
    os->push_back(new Output(def->name,OVarname));
    env.decorate(btannos,os);
    if (middle!=NULL) {
        os->push_back(blank);
        os->push_back(middle);
    }
    return new Output(Output::Consistent,os);
}

/************************************************************************
 ************************************************************************

                   RENDERING DECLARATIONS

 ************************************************************************
 ************************************************************************/

static Output*
outputObjDecl(ObjectDecl* d, OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    Output* o = env.decorate(d->type->btannos,new Output(d->name,OVarname));
    os->push_back(d->type->output(env,o));
    // Put modifiers in front.
    switch (d->linkage) {
    case Internal:
        os->push_front(blank);
        os->push_front(o_static);
        break;
    case External:
        os->push_front(blank);
        os->push_front(o_extern);
        break;
    case AbstractType:
        os->push_back(blank);
        os->push_back(new Output("/* abstract type */",OComment));
        // fall through
    case Typedef: 
        os->push_front(blank);
        os->push_front(o_typedef);
        break;
    case NoLinkage:
        break;
    }
    return new Output(Output::Consistent,os);
}

Output*
VarDecl::output(OutCpgm &env)
{
  if (debug^2) { assert(name!=NULL); debug << "[" << name; }
    if ( linkage == AbstractType )
        return o_Null ;
    switch ( varmode ) {
    case VarExtSpectime:
    case VarExtResidual:
    case VarExtDefault:
        if ( directives.is_wellknown(name) )
             return o_Null ;
        break ;
    case VarConstant:
        return o_Null ;
    default:
        break;
    }
    Output* o = outputObjDecl(this, env);
    // Initializers.
    if (init) {
        o = oconcat(o,eqsign);
        o = oconcat(o,init->output(env));
    }
    if (debug^2) debug << "]";
    return o;
}

Output*
FunDef::output(OutCpgm &env)
{
    if ( stmts == NULL && directives.is_wellknown(name) )
        return o_Null ;
  if (debug^1) debug << "[" << name;
    Plist<Output>* os = new Plist<Output>();
    // The return type.
    os->push_back(((FunType*)type)->ret->output(env));
    os->push_back(blank);
    // The function head.
    os->push_back(new Output(name,OVarname));
    os->push_back(lparen);

    // Make the list of parameters.    
    if (decls!=NULL) {
        OutputList parms(0,Output::Inconsistent);
        parms.sep.push_back(comma);
        parms.sep.push_back(BREAK);
        foreach(d,*decls,Plist<VarDecl>)
            parms += d->output(env);
        os->push_back(parms.inter());
    }

    os->push_back(rparen);
    // The function body. An external function has no statement.
    if (stmts != NULL) {
      os->push_back(newline);
      // The body is just a compound statement.
      MixedReason saved = env.mixreason ;
      env.mixreason = BOOLEAN ;
      os->push_back(stmts->output(env));
      env.mixreason = saved ;
    } else
        os->push_back(semi);
    if (debug^1) debug << "]";
    return new Output(Output::Consistent,os);
}

/************************************************************************
 ************************************************************************

                   RENDERING USERTYPE DEFINITIONS

 ************************************************************************
 ************************************************************************/

Output*
StructDecl::output(OutCpgm &env)
{
    assert(name!=NULL);
    // The struct name.
    Plist<Output>* os = new Plist<Output>();
    os->push_back(usertype2output(tag));
    os->push_back(blank);
    os->push_back(new Output(name,OVarname));
    // The members.
    if ( !member_list->empty() ) {
        MixedReason save = env.mixreason ;
        env.mixreason = USERTYPE ;
        os->push_back(blank);
        os->push_back(lbrace);
        os->push_back(INDENT);
        OutputList memblist(0,Output::Consistent);
        memblist.sep.push_back(semi);
        memblist.sep.push_back(BREAK);
        foreach(i,*member_list,Plist<MemberDecl>)
            memblist += i->output(env);
        os->push_back(memblist);
        os->push_back(BREAK);
        os->push_back(rbrace);
        env.mixreason = save ;
    }
    else
        os->push_back(new Output(" /*incomplete*/",OComment));
    os->push_back(semi);
    return new Output(Output::Consistent,os) ;
}

Output*
EnumDecl::output(OutCpgm &env)
{
    assert(name!=NULL);
    // The enum name.
    Plist<Output>* os = new Plist<Output>();
    os->push_back(usertype2output(Enum));
    os->push_back(blank);
    os->push_back(new Output(name,OVarname));
    // The constants.
    Plist<Output>* body = new Plist<Output>();
    if ( !member_list->empty() ) {
        os->push_back(blank);
        os->push_back(lbrace);
	os->push_back(INDENT);
        bool first = true ;
        foreach(mb, *member_list, Plist<VarDecl>) {
	  if (!first) {
	    body->push_back(comma);
	    body->push_back(BREAK);
          }
          first = false ;
          body->push_back(new Output((*mb)->name,OConstant));
          if ( mb->init ) {
            body->push_back(eqsign);
            body->push_back((*mb)->init->output(env));
          }
        }
	os->push_back(new Output(Output::Inconsistent,body,0));
	os->push_back(BREAK);
        os->push_back(rbrace);
    }
    else
        os->push_back(new Output(" /*incomplete*/",OComment));
    os->push_back(semi);
    return new Output(Output::Consistent,os);
}

Output*
MemberDecl::output(OutCpgm &env)
{
    Output* o = var->output(env);
    if (bitfield) {
        o = oconcat(o,colon);
        o = oconcat(o,bitfield->output(env));
    }
    return o;
}

/************************************************************************
 ************************************************************************

                   RENDERING EXPRESSIONS

 ************************************************************************
 ************************************************************************/

// Put parentheses around expression when a sub-expression has lower
// precedence.
static void
par(Plist<Output>* os, int pre, Expr* e, OutCpgm &env)
{
    if (e->precedence() < pre)
        os->push_back(lparen);
    os->push_back(e->output(env));
    if (e->precedence() < pre)
        os->push_back(rparen);
}

Output*
Init::output(OutCpgm &env)
{
    if (tag==InitElem) {
        return exp->output(env);
    }
    else {
        Plist<Output>* os = new Plist<Output>();
        os->push_back(lbrace);
        OutputList contents(0,Output::Inconsistent) ;
        contents.sep.push_back(comma);
        contents.sep.push_back(BREAK);
        foreach(i, *inits, Plist<Init>)
            contents += i->output(env);
        os->push_back(contents.inter());
        os->push_back(rbrace);
        return new Output(Output::Consistent,os);
    }
}

Output*
ConstExpr::output(OutCpgm &env)
{
    return env.decorate(btannos,new Output(literal, OConstant));
}

Output*
NullExpr::output(OutCpgm &env)
{
    return env.decorate(btannos,nullptr);
}

Output*
VarExpr::output(OutCpgm &env)
{
    return env.decorate(btannos,new Output(decl->name,OVarname));
}

Output*
ArrayExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    par(os, precedence(), left, env);
    os->push_back(env.decorate(btannos,lbracket));
    os->push_back(right->output(env));
    os->push_back(env.decorate(btannos,rbracket));
    return new Output(Output::Consistent,os);
}

Output*
UnaryExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,unary2output(op)));
    par(os, precedence(), exp, env);
    return new Output(Output::Consistent,os);
}


static Output*
incr2output(Incrementer i)
{
    switch (i) {
    case Decr:
        return new Output("--",OSymbol);
        break;
    case Inc:
        return new Output("++",OSymbol);
        break;
    }
    return NULL;
}

Output*
PostExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
     par(os, precedence(), exp, env);
    os->push_back(env.decorate(btannos,incr2output(op)));
    return new Output(Output::Consistent,os);
}

Output*
PreExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,incr2output(op)));
    par(os, precedence(), exp, env);
    return new Output(Output::Consistent,os);
}

Output*
CondExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(cond->output(env));    
    os->push_back(blank);
    os->push_back(env.decorate(btannos,question));
    os->push_back(new Output(BlockIndentLevel,0));
    os->push_back(left->output(env));    
    os->push_back(blank);
    os->push_back(env.decorate(btannos,colon));
    os->push_back(new Output(BlockIndentLevel,0));
    os->push_back(right->output(env));
    return new Output(Output::Consistent,os);
}

Output*
CallExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    par(os, precedence(), fun, env);
    os->push_back(env.decorate(btannos,lparen));
    OutputList arglist(0,Output::Consistent);
    arglist.sep.push_back(comma);
    arglist.sep.push_back(BREAK);
    foreach(a,*args,Plist<Expr>)
        arglist += a->output(env);
    os->push_back(arglist.inter());
    os->push_back(env.decorate(btannos,rparen));
    return new Output(Output::Consistent,os);
}

Output*
DotExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    par(os, precedence(), left, env);
    os->push_back(env.decorate(btannos,oconcat(dotsign,
                                               new Output(member,OVarname))));
    return new Output(Output::Consistent,os);
}

Output*
TypeSize::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_sizeof));
    os->push_back(lparen);
    os->push_back(typesz->output(env));
    os->push_back(rparen);
    return new Output(Output::Consistent,os);
}

Output*
CastExpr::output(OutCpgm &env)
{
  if (silent)
    return exp->output(env);
  else {
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,lparen));
    os->push_back(exprType->output(env));
    os->push_back(env.decorate(btannos,rparen));
    par(os, precedence(), exp, env);
    return new Output(Output::Consistent,os);
  }
}

Output*
CommaExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    par(os, precedence(), left, env);
    os->push_back(comma);
    os->push_back(BREAK);
    par(os, precedence(), right, env);
    return new Output(Output::Consistent,os);
}

Output*
AssignExpr::output(OutCpgm &env)
{ 
    Plist<Output>* os = new Plist<Output>();
    par(os, precedence(), left, env);
    Output* o = NULL;
    switch (op) {
    case MulAsgn:
        o = new Output("*=", OSymbol);
        break;
    case DivAsgn:
        o = new Output("/=", OSymbol);
        break;
    case ModAsgn:
        o = new Output("%=", OSymbol);
        break;
    case AddAsgn:
        o = new Output("+=", OSymbol);
        break;
    case SubAsgn:
        o = new Output("-=", OSymbol);
        break;
    case LSAsgn:
        o = new Output("<<=", OSymbol);
        break;
    case RSAsgn:
        o = new Output(">>=", OSymbol);
        break;
    case AndAsgn:
        o = new Output("&=", OSymbol);
        break;
    case OrAsgn:
        o = new Output("|=", OSymbol);
        break;
    case EOrAsgn:
        o = new Output("^=", OSymbol);
        break;
    case Asgn:
        o = eqsign;
        break;
    }
    os->push_back(blank);
    os->push_back(env.decorate(btannos,o));
    os->push_back(INDENT);
    par(os, precedence(), right, env);
    return new Output(Output::Consistent,os);
}

Output*
ExprSize::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_sizeof));
    os->push_back(lparen);
    os->push_back(exp->output(env));
    os->push_back(rparen);
    return new Output(Output::Consistent,os);
}

Output*
BinaryExpr::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    par(os, precedence(), left, env);
    os->push_back(blank);
    os->push_back(env.decorate(btannos,binary2output(op)));
    os->push_back(BREAK);
    par(os, precedence(), right, env);
    return new Output(Output::Consistent,os);
}

/************************************************************************
 ************************************************************************

                   RENDERING STATEMENTS

 ************************************************************************
 ************************************************************************/

Output*
LabelStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(new Output(label,OLabel));
    os->push_back(colon);
    os->push_back(newline);
    os->push_back(stmt->output(env));
    return new Output(Output::Consistent,os);
}

Output*
CaseStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_case));
    os->push_back(blank);
    os->push_back(exp->output(env));
    os->push_back(colon);
    os->push_back(newline);
    os->push_back(stmt->output(env));
    return new Output(Output::Consistent,os);
}

Output*
DefaultStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_default));
    os->push_back(colon);
    os->push_back(newline);
    os->push_back(stmt->output(env));
    return new Output(Output::Consistent,os);
}

Output*
CompoundStmt::output(OutCpgm &env)
{
    // Make an indented block out of the statements.
    OutputList body(0,Output::Consistent);
    body.sep.push_back(newline);
    foreach(o,*objects,Plist<VarDecl>)
        body += oconcat(o->output(env),semi);
    foreach(s,*stmts,Plist<Stmt>)
        body += s->output(env);
    
    // Wrap it in braces.
    Plist<Output>*os = new Plist<Output>();
    os->push_back(lbrace);
    os->push_back(new Output(BlockIndentLevel, 1));
    os->push_back(body);
    os->push_back(BREAK);
    os->push_back(rbrace);

    return new Output(Output::Consistent,os);
}

Output*
ExprStmt::output(OutCpgm &env)
{
    return oconcat(exp->output(env),semi);
}

Output*
IfStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_if));
    os->push_back(env.decorate(btannos,lparen));
    os->push_back(exp->output(env));
    os->push_back(env.decorate(btannos,rparen));
    os->push_back(INDENT);
    os->push_back(thn->output(env));
    os->push_back(BREAK);
    os->push_back(env.decorate(btannos,o_else));
    os->push_back(INDENT);
    os->push_back(els->output(env));
    return new Output(Output::Consistent,os);
}

Output*
DummyStmt::output(OutCpgm &env)
{    
    return semi ;
}

Output*
SwitchStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_switch));
    os->push_back(env.decorate(btannos,lparen));
    os->push_back(exp->output(env));
    os->push_back(env.decorate(btannos,rparen));
    os->push_back(BREAK);
    os->push_back(stmt->output(env));
    return new Output(Output::Consistent,os);
}

Output*
WhileStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_while));
    os->push_back(env.decorate(btannos,lparen));
    os->push_back(exp->output(env));
    os->push_back(env.decorate(btannos,rparen));
    os->push_back(BREAK);
    os->push_back(stmt->output(env));
    return new Output(Output::Consistent,os);
}

Output*
DoStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_do));
    os->push_back(BREAK);
    os->push_back(stmt->output(env));
    os->push_back(env.decorate(btannos,o_while));
    os->push_back(env.decorate(btannos,lparen));
    os->push_back(exp->output(env));
    os->push_back(env.decorate(btannos,rparen));
    os->push_back(semi);
    return new Output(Output::Consistent,os);
}

Output*
ForStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    // First make the parts inside parentheses.
    if ( e1 )
        os->push_back(e1->output(env));
    os->push_back(env.decorate(btannos,semi));
    os->push_back(BREAK);
    if ( e2 )
        os->push_back(e2->output(env));
    os->push_back(env.decorate(btannos,semi));
    os->push_back(BREAK);
    if ( e3 )
        os->push_back(e3->output(env));
    Output* o = new Output(Output::Consistent,os);
    // Then wrap the keyword around
    os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_for));
    os->push_back(env.decorate(btannos,lparen));
    os->push_back(o);
    os->push_back(env.decorate(btannos,rparen));
    os->push_back(BREAK);
    // Do the statement.
    os->push_back(stmt->output(env));
    return new Output(Output::Consistent,os);
}


Output*
Indirection::output(OutCpgm&)
{
    if (stmt != NULL) { 
        return new Output(stmt->label,OLabel);
    }
    else {
        return new Output("<not resolved>",OLabel);
    }
}


Output*
GotoStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(o_goto);
    os->push_back(blank);
    os->push_back(ind->output(env));
    os->push_back(semi);
    return new Output(Output::Consistent,os);
}

Output*
BreakStmt::output(OutCpgm &env)
{
    return oconcat(o_break,semi);
}

Output*
ContStmt::output(OutCpgm &env)
{
    return oconcat(o_continue,semi);
}

Output*
ReturnStmt::output(OutCpgm &env)
{
    Plist<Output>* os = new Plist<Output>();
    os->push_back(env.decorate(btannos,o_return));
    if (exp != NULL) {
        os->push_back(blank);
        os->push_back(exp->output(env));
    }
    os->push_back(semi);
    return new Output(Output::Consistent,os);
}


void
CProgram::output(OutputContainer &container, BTresult &bt)
{
    OutCpgm env(container,bt);
    OutputList program(0,Output::Consistent);
    program.sep.push_back(newline);
    foreach(ut,*usertypes,Plist<UserDecl>)
        program += ut->output(env);
    foreach(d,*definitions,Plist<VarDecl>) {
        Output *elt = d->output(env);
        if ( elt != o_Null )
            program += oconcat(elt,semi);
    }
    foreach(f,*functions,Plist<FunDef>)
        program += f->output(env) ;
    container.add(program,OProgram);
}


