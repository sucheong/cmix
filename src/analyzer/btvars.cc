/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Generating raw binding-time variables
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "bta.h"
#include "outcore.h"
#include "commonout.h"
#include "renamer.h"

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  S P E C I F I C    B I N D I N G - T Y P E    V A R I A B L E S
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

class CommonTbtv : public BTvariable {
  const char *text ;
public:
  Position pos ;
  CommonTbtv(const char *t,Position p) : text(t), pos(p) {}
  virtual void name(Diagnostic &) =0 ;
  virtual Output *name(Outcore *) =0 ;
  virtual void show(Diagnostic &d)
    {
      d.addline(pos) ;
      name(d);
      d << ' ' << text ;
    }
  virtual Output *show(Outcore *oc)
    {
      Plist<Output>* os = new Plist<Output> ;
      os->push_back(name(oc));
      os->push_back(INDENT);
      os->push_back(new Output(text,OAnnoText));
      return new Output(Output::Consistent,os);
    }
};

class TopTbtv : public CommonTbtv {
  C_Decl &owner ;
public:
  TopTbtv(C_Decl *d,const char *t)
    : CommonTbtv(t,d->pos), owner(*d) {}
  virtual bool hasOrgName()
    { return owner.hasName(); }
  virtual void name(Diagnostic &d)
    { d << owner.get_name() ; }
  virtual Output *name(Outcore *oc)
    {
      return oc->crosslink(new Output((*oc->the_names)[owner],OVarname),
                           owner);
    }
};

class ExprTbtv : public CommonTbtv {
  const char *kind ;
public:
  ExprTbtv(const char *k,Position p,const char *t)
    : CommonTbtv(t,p), kind(k) {}
  virtual void name(Diagnostic &d)
    { d << kind; }
  virtual Output *name(Outcore *)
    { return new Output(kind,OKeyword); }
};

class OwnedTbtv : public CommonTbtv {
  CommonTbtv *owner ;
public:
  OwnedTbtv(CommonTbtv *o, const char *t)
    : CommonTbtv(t,o->pos), owner(o) {}
  virtual void extra(Diagnostic &) =0 ;
  virtual Output *extra(Outcore *) =0 ;
  virtual bool hasOrgName()
    { return owner->hasOrgName(); }
  virtual void name(Diagnostic &d)
    { owner->name(d); extra(d); }
  virtual Output *name(Outcore *oc)
    { return oconcat(owner->name(oc),extra(oc)); }
};

class NextTbtv : public OwnedTbtv {
  const char *link ;
public:
  NextTbtv(CommonTbtv *o,const char *l, const char *t)
    : OwnedTbtv(o,t), link(l) {}
  virtual void extra(Diagnostic &d)
    { d << link; }
  virtual Output *extra(Outcore *oc)
    { return new Output(link,OSymbol); }
};

class MemberTbtv : public OwnedTbtv {
  C_UserMemb *memb ;
public:
  MemberTbtv(CommonTbtv *o,C_UserMemb *m, const char *t)
    : OwnedTbtv(o,t), memb(m) {}
  virtual void extra(Diagnostic &d)
    { d << '.' << memb->get_name() ; }
  virtual Output *extra(Outcore *oc)
    {
      return oconcat(dotsign,
                     new Output((*oc->the_names)[memb],OVarname));
    }
};

class AllocSplitBtv : public BTvariable {
  C_Stmt *owner ;
public:
  AllocSplitBtv(C_Stmt *o) : owner(o) {}
  virtual void show(Diagnostic &d)
    {
      d.addline(owner->pos)
        << "this heap allocation is residual" ;
    }
  virtual Output *show(Outcore *oc)
    {
      Plist<Output> *os = new Plist<Output> ;
      os->push_back(
                    oc->crosslink(new Output("this heap allocation",OAnnoText),
                                  owner));
      os->push_back(BREAK);
      os->push_back(new Output("is residual",OAnnoText));
      return new Output(Output::Consistent,os);
    }
};

class UnderDynamicControl : public BTvariable {
  C_BasicBlock &owner ;
public:
  UnderDynamicControl(C_BasicBlock &o) : owner(o) {}
  virtual bool hasOrgName() { return false; }
  virtual Output *show(Outcore *oc)
    {
      Output *o = new Output(" is under dynamic control",OAnnoText);
      return oconcat(oc->xref(owner),o);
    }
};

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  SHARING BINDING-TIME VARIABLES FOR FORCED-FAITHFUL EXPRESSIONS
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

static void setbtvar(BTmap &bt,BTvariable *var,C_Expr *e);
static void mkbtvars(BTmap &bt,C_Expr *e);

static void
setbtvar(BTmap &bt,BTvariable *var,C_Type *t)
{
  if ( bt[t] != NULL ) {
    assert( bt[t] == var );
    return ;
  }
  bt[t] = var ;
  switch(t->tag) {
  case Primitive:
  case Abstract:
  case EnumType:
    break ;
  case FunPtr:
  case Pointer:
    setbtvar(bt,var,t->ptr_next());
    break;
  case Array:
    if ( t->hasSize() )
      mkbtvars(bt,t->array_size());
    setbtvar(bt,var,t->array_next());
    break;
  case Function: {
    foreach(arg,t->fun_params(),Plist<C_Type>)
      setbtvar(bt,var,*arg);
    setbtvar(bt,var,t->fun_ret());
    break; }
  case StructUnion: {
    foreach(member,t->user_types(),Plist<C_Type>)
      setbtvar(bt,var,*member);
    break; }
  }
}

static void
setbtvar(BTmap &bt,BTvariable *var,C_Expr *e)
{
  switch (e->tag) {
  case C_EnumCnst: case C_Null: case C_Cnst: case C_Unary:
  case C_Binary: case C_PtrCmp: case C_SizeofT: case C_SizeofE:
  case C_Cast:
    setbtvar(bt,var,e->type);
    break ;
  case C_Member: case C_Var: case C_FunAdr: case C_Array: case C_PtrArith:
    assert( bt[e->type] == NULL );
    bt[e->type] = var ;
    break;
  case C_DeRef: case C_ExtFun:
    break ;
  }
  switch (e->tag) {
  case C_EnumCnst: case C_Cnst: case C_Null: case C_Var:
  case C_FunAdr: case C_ExtFun:
    break ;
  case C_Unary: case C_DeRef: case C_Array:
  case C_Member: case C_Cast: case C_SizeofE:
    setbtvar(bt,var,e->subexpr());
    break;
  case C_Binary: case C_PtrCmp: case C_PtrArith:
    setbtvar(bt,var,e->binary_expr1());
    setbtvar(bt,var,e->binary_expr2());
    break;
  case C_SizeofT:
    setbtvar(bt,var,e->sizeof_type());
    break;
  }
}

// ===========================================================================
// '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
//  GENERATING INDIVIDUAL, NICELY NAMED BINDING-TIME VARIABLES
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,
// ===========================================================================

////// Types //////

static char const*
type_purpose(C_Type *t) {
  switch(t->tag) {
  case FunPtr:
    return "must point to an external residual function";
  case Primitive: case Abstract: case EnumType: case Pointer:
    return "is residual";
  case Array:
    return "is residually indexed";
  case Function:
    return "is a residual function";
  case StructUnion:
    return "has residual members";
  }
  return "has the strangest type...";
}      

static void
assign_Tbtv(BTmap &bt,C_Type *t,CommonTbtv *var)
{
  assert(bt[t]==NULL);
  bt[t] = var ;
  switch(t->tag) {
  case Primitive: case Abstract: case EnumType:
    break;
  case FunPtr:
  case Pointer: {
    C_Type *next = t->ptr_next() ;
    if ( bt[next] == NULL )
      assign_Tbtv(bt,next,new NextTbtv(var,"*",type_purpose(next)));
    break; }
  case Array: {
    C_Type *next = t->array_next() ;
    if ( bt[next] == NULL )
      assign_Tbtv(bt,next,new NextTbtv(var,"[]",type_purpose(next)));
    if ( t->hasSize() )
      mkbtvars(bt,t->array_size());
    break; }
  case Function: {
    C_Type *ret = t->fun_ret() ;
    if ( bt[ret] == NULL )
      assign_Tbtv(bt,ret,new NextTbtv(var,"()",type_purpose(ret)));
    unsigned pcount = 0 ;
    foreach(arg,t->fun_params(),Plist<C_Type>) {
      if ( bt[*arg] == NULL ) {
        static char const *nrs[10]
          = { "(#1)", "(#2)", "(#3)", "(#4)", "(#5)",
              "(#6)", "(#7)", "(#8)", "(#9)", "(#*)" };
        assign_Tbtv(bt,*arg,
                    new NextTbtv(var,nrs[pcount],type_purpose(*arg)));
      }
      if ( pcount < 9 ) pcount++;
    }
    break; }
  case StructUnion: {
    Plist<C_UserMemb>::iterator n = t->user_def()->names.begin() ;
    foreach(member,t->user_types(),Plist<C_Type>) {
      if ( bt[*member] == NULL )
        assign_Tbtv(bt,*member,
                    new MemberTbtv(var,*n,type_purpose(*member)));
      n++ ;
    }
    break; }
  }
}

////// CoreC Expressions //////

static const char *
expr2cookie(C_Expr *e)
{
  switch(e->tag) {
  case C_Null:
    return "(null pointer)";
  case C_Cnst:
    return e->cnst();
  case C_SizeofT: case C_SizeofE:
    return "(sizeof)" ;
  case C_Cast:
    return "(typecast)" ;
  default:
    return "(strange thing!)" ;
  }
}

static void
mkbtvars(BTmap &bt,C_Expr *e)
{
  switch (e->tag) {
  case C_Cnst:
    assert( bt[e->type] == NULL );
    if (e->type->tag != Primitive) {
      setbtvar(bt,new ExprTbtv(e->cnst(),e->pos,"is a residual constant"),
               e->type);
      break ;
    }
  case C_EnumCnst: case C_Null: case C_Unary:
  case C_Binary: case C_PtrCmp: case C_SizeofT: case C_SizeofE:
  case C_Cast:
    assert( bt[e->type] == NULL );
    switch (e->type->tag) {
    case Primitive: case Abstract: case EnumType:
      bt[e->type] = new BTanonymousVar ;
      break ;
    default:
      assign_Tbtv(bt,e->type,new ExprTbtv(expr2cookie(e),e->pos,
                                          type_purpose(e->type)));
    }
    break ;
  case C_Member: case C_Var: case C_FunAdr: case C_Array: case C_PtrArith:
    assert( bt[e->type] == NULL );
    bt[e->type] = new BTanonymousVar ;
    break;
  case C_ExtFun:
  case C_DeRef:
    break ;
  }
  switch (e->tag) {
  case C_EnumCnst: case C_Cnst: case C_Null: case C_Var:
  case C_FunAdr: case C_ExtFun:
    break ;
  case C_Unary: case C_DeRef: case C_Array: case C_Member: case C_Cast:
  case C_SizeofE:
    mkbtvars(bt,e->subexpr());
    break;
  case C_Binary: case C_PtrCmp: case C_PtrArith:
    mkbtvars(bt,e->binary_expr1());
    mkbtvars(bt,e->binary_expr2());
    break;
  case C_SizeofT:
    setbtvar(bt,bt[e->type],e->sizeof_type());
    break;
  }
}

////// Control statements //////

static void
mkbtvars(BTmap &bt,C_Jump *j)
{
  switch (j->tag) {
  case C_If:
    mkbtvars(bt,j->cond_expr());
    break;
  case C_Goto:
    break;
  case C_Return:
    if ( j->hasExpr() )
      mkbtvars(bt,j->return_expr());
    break;
  }
}

////// Statements //////

static void
mkbtvars(BTmap &bt,C_Stmt *s)
{
  if ( s->hasTarget() )
    mkbtvars(bt,s->target());
  switch (s->tag) {
  case C_Assign:
    mkbtvars(bt,s->assign_expr());
    break;
  case C_Call: {
    mkbtvars(bt,s->call_expr());
    foreach(arg,s->call_args(),Plist<C_Expr>)
      mkbtvars(bt,*arg);
    break; }
  case C_Alloc: {
    C_Decl *pseudoarray = s->alloc_objects() ;
    if ( pseudoarray->type->hasSize() )
      mkbtvars(bt,pseudoarray->type->array_size());
    bt[pseudoarray->type] = new AllocSplitBtv(s);
    assign_Tbtv(bt,pseudoarray->type->array_next(),
                new ExprTbtv("(allocation)",s->pos,
                             type_purpose(pseudoarray->type->array_next())));
    break ; }
  case C_Free:
    mkbtvars(bt,s->free_expr());
    break;
  case C_Sequence:
    break;
  }
}

/////// Declarations ///////

static void
mkbtvars(BTmap &bt,C_Init *i)
{
  switch(i->tag) {
  case FullyBraced:
  case SloppyBraced: {
    foreach(j,i->braced_init(),Plist<C_Init>)
      mkbtvars(bt,*j);
    break ; }
  case StringInit:
    break ;
  case Simple:
    mkbtvars(bt,i->simple_init());
    break ;
  }
}

static void
mkbtvars(BTmap &bt,C_Decl *d)
{
  switch (d->tag) {
  case VarDcl:
    assign_Tbtv(bt,d->type,new TopTbtv(d,type_purpose(d->type)));
    if ( d->hasInit())
      mkbtvars(bt,d->init());
    break;
  case FunDf: {
    bt[d] = new TopTbtv(d,"may be called speculatively");
    CommonTbtv *tbtv = new TopTbtv(d,"cannot be specialized");
    bt[d->type] = tbtv ;
    assign_Tbtv(bt,d->type->fun_ret(),
                new NextTbtv(tbtv,"()",type_purpose(d->type->fun_ret())));
    foreach(par,d->fun_params(),Plist<C_Decl>)
      mkbtvars(bt,*par);
    foreach(local,d->fun_locals(),Plist<C_Decl>)
      mkbtvars(bt,*local);
    foreach(bb,d->blocks(),Plist<C_BasicBlock>) {
      assert(bt[*bb] == NULL);
      bt[*bb] = new UnderDynamicControl(**bb);
      foreach(stmt,bb->getBlock(),Plist<C_Stmt>)
        mkbtvars(bt,*stmt);
      mkbtvars(bt,bb->exit());
    }
    break; }
  case ExtState:
  case ExtFun:
    Diagnostic(INTERNAL,Position()) << "this never happens";
    break;
  }
}

////// CoreC program //////

void
create_btvars(C_Pgm const&pgm,BTmap &bt)
{
  foreach(exf,pgm.exfuns,Plist<C_Decl>)
    setbtvar(bt,new ExprTbtv(exf->get_name(),exf->pos,
                             "must be called residually"),
             exf->type);
  foreach(var,pgm.globals,Plist<C_Decl>)
    mkbtvars(bt,*var);
  foreach(gen,pgm.generators,Plist<C_Decl>)
    mkbtvars(bt,*gen);
  foreach(fun,pgm.functions,Plist<C_Decl>)
    mkbtvars(bt,*fun);
  foreach(ed,pgm.enumtypes,Plist<C_EnumDef>) {
    bt[*ed] = new BTanonymousVar ;
    foreach(m,ed->members(),Plist<C_UserMemb>) {
      bt[*m] = bt[*ed] ;
      if ( m->hasValue() )
        mkbtvars(bt,m->value());
    }
  }
  foreach(ud,pgm.usertypes,Plist<C_UserDef>) {
    foreach(m,ud->names,Plist<C_UserMemb>)
      if ( m->hasValue() )
        mkbtvars(bt,m->value());
    // some of the instances may not yet have received bt-variables
    // (because the c2core phase, eftar creating them, decideded they
    // were unnecessary). Give them some variables anyway, lest later
    // phases get confused by the lack of bt-attributes.
    foreach(i,ud->instances,Plist<C_Type>)
      if ( bt[*i] == NULL ) {
        static BTvariable *var = new ExprTbtv("This",Position(),
                                              "should never be hit!");
        setbtvar(bt,var,*i);
      }
  }
}
