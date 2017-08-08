/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Transform ANSI C into CoreC
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <string.h>
#include "array.h"
#include "options.h"
#include "diagnostic.h"
#include "directives.h"
#include "c2core.h"

#define debug debugstream_c2core

/************************************************************************
 ************************************************************************
 ***								      ***
 ***   P R O T O - F L O W G R A P H S                                ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

/******************************************
 * class AnyAtom
 ******************************************/

class AnyAtom ;
static Plist<AnyAtom> AllAtoms ;

class AnyAtom
{
protected:
  AnyAtom() ;
  unsigned predecessor_count ;
  C_BasicBlock *collected ;
  static C_BasicBlock *get_bb(AnyAtom*) ;
public:
  static Plist<C_BasicBlock>* collectAll(AnyAtom*) ;
  virtual bool block_leader(C_BasicBlock *);
  virtual C_Jump *create_jump();
  virtual void add_predecessor(unsigned=1);
  virtual void emit_stmt(C_BasicBlock *);
  virtual AnyAtom *get_next() ;
  virtual void show(ostream &ost) {
    ost << "in-degree=" << predecessor_count << endl ;
  }
};

AnyAtom::AnyAtom(): predecessor_count(0), collected(NULL)
{
  AllAtoms.push_back(this);
}

bool
AnyAtom::block_leader(C_BasicBlock *)
{
  return predecessor_count > 1 ;
}

void
AnyAtom::add_predecessor(unsigned nr)
{
  predecessor_count += nr ;
}

void
AnyAtom::emit_stmt(C_BasicBlock *)
{
}

AnyAtom *
AnyAtom::get_next()
{
  return NULL ;
}
   
/******************************************
 * class SimpleAtom
 ******************************************/

class SimpleAtom: public AnyAtom
{
protected:
  AnyAtom *next ;
  SimpleAtom() ;
public:
  virtual void link(AnyAtom *);
  virtual AnyAtom *get_next() ;
  virtual void show(ostream &ost) {
    if ( next != NULL )
      ost << "next=" << (void*)next << ", " ;
    AnyAtom::show(ost);
  }
};

SimpleAtom::SimpleAtom(): next(NULL) {}

AnyAtom *
SimpleAtom::get_next()
{
  return next ;
}

void
SimpleAtom::link(AnyAtom *new_next)
{
  assert( new_next != NULL );
  assert( next == NULL );
  next = new_next ;
  next->add_predecessor();
}

/******************************************
 * class StmtAtom
 ******************************************/

class StmtAtom: public SimpleAtom
{
public:
  C_Stmt *stmt ;
  StmtAtom(C_Stmt *);
  virtual void emit_stmt(C_BasicBlock *) ;
  virtual void show(ostream &ost) {
    ost << "Statement: " ;
    SimpleAtom::show(ost);
  }
};

StmtAtom::StmtAtom(C_Stmt *s) {
  assert( s != NULL );
  stmt = s ;
}

void
StmtAtom::emit_stmt(C_BasicBlock *bb)
{
  switch ( stmt->tag ) {
  case C_Assign:
  case C_Alloc:
    if ( !stmt->hasTarget() )
      return ;
    /* else fall through */
  default:
    bb->getBlock().push_back(stmt);
  }
}

/******************************************
 * class GotoAtom
 ******************************************/

class GotoAtom: public SimpleAtom {
  enum { Unlinked, Linking, Linked, PerpetualLoop } status ;
public:
  GotoAtom() ;
  virtual void link(AnyAtom *);
  virtual void add_predecessor(unsigned=1);
  virtual void show(ostream &ost) {
    ost << "Goto (" ;
    switch(status) {
    case Unlinked: ost << "Unlinked): " ; break ;
    case Linking: ost << "Linking): " ; break ;
    case Linked: ost << "Linked): " ; break ;
    case PerpetualLoop: ost << "PerpetualLoop): " ; break ;
    default: ost << "Bad status): " ; break ;
    }
    SimpleAtom::show(ost);
  }
};

GotoAtom::GotoAtom(): status(Unlinked) {}

void
GotoAtom::link(AnyAtom *new_next)
{
  assert( new_next != NULL );
  assert( next == NULL );
  assert( status == Unlinked );
  status = Linking ;
  next = new_next ;
  new_next->add_predecessor(predecessor_count);
  if ( status != Linking ) {
    // this forms an infinite loop
    predecessor_count = 2 ; // enough to provoke a BB here
  } else {
    predecessor_count = 0 ;
    status = Linked ;
  }
}

void
GotoAtom::add_predecessor(unsigned nr)
{
  switch(status) {
  case Unlinked:
    assert( next == NULL );
    predecessor_count += nr ;
    break ;
  case Linking:
    status = PerpetualLoop ;
    break ;
  case Linked:
    assert( next != NULL );
    next->add_predecessor(nr);
    break ;
  case PerpetualLoop:
    break ;
  }
}

/******************************************
 * class InitAtom
 ******************************************/

class InitAtom: public SimpleAtom {
  C_Decl *const owner ;
public:
  InitAtom(C_Decl *o) : owner(o) {}
  virtual bool block_leader(C_BasicBlock *);
  virtual void emit_stmt(C_BasicBlock *);
  virtual void show(ostream &ost) {
    ost << "Init " << owner->get_name() << ": " ;
  }
};

bool
InitAtom::block_leader(C_BasicBlock *bb)
{
  // Start off a new basic block if a compound initializer appears
  // in the middle of the block. That guarantees that any initializing
  // statement will be inserted at the right place (the testsuite
  // contains pathological examples that malfunction if this is not
  // done).
  return !bb->getBlock().empty() || SimpleAtom::block_leader(bb) ;
}

void
InitAtom::emit_stmt(C_BasicBlock *bb) {
  owner->init_where(bb) ;
}

/******************************************
 * class IfAtom
 ******************************************/

class IfAtom : public AnyAtom {
  C_Expr *condition ;
  AnyAtom *Then, *Else ;
  Position pos ;
public:
  IfAtom(C_Expr *,AnyAtom *, AnyAtom *, Position);
  virtual C_Jump *create_jump();
  virtual void show(ostream &ost) {
    ost << "if (..) " << (void*) Then << " else " << (void*)Else << ':' ;
    AnyAtom::show(ost);
  }
};

IfAtom::IfAtom(C_Expr *e, AnyAtom *th, AnyAtom *el, Position p)
  : condition(e), Then(th), Else(el), pos(p)
{
  assert( e != NULL );
  assert( th != NULL );   assert( el != NULL );
  th->add_predecessor();  el->add_predecessor();
}

/******************************************
 * class ReturnAtom
 ******************************************/

class ReturnAtom : public AnyAtom {
  C_Jump *jump ;
public:
  ReturnAtom(C_Jump *);
  virtual C_Jump *create_jump();
  virtual void show(ostream &ost) {
    ost << "return:" ;
    AnyAtom::show(ost);
  }
};

ReturnAtom::ReturnAtom(C_Jump *j): jump(j) {}

/******************************************
 * class StopperAtom
 ******************************************/

class StopperAtom : public AnyAtom {
  C_Type *rettype ;
  CodeEnv &env ;
  Position pos ;
public:
  StopperAtom(C_Type *,CodeEnv &, Position);
  virtual C_Jump *create_jump();
  virtual void show(ostream &ost) {
    ost << "stopper:" ;
    AnyAtom::show(ost);
  }
};

StopperAtom::StopperAtom(C_Type *t,CodeEnv &e, Position p)
  : rettype(t), env(e), pos(p)
{
  assert( t != NULL );
}

/***********************************************************************
 * These are the functions that make a final proto-flowgraph into a
 * list of C_BasicBlocks. They work together to do a depth-first
 * search through the proto-flowgraph.
 */

static Plist<AnyAtom> *collect_stack ;

Plist<C_BasicBlock> *
AnyAtom::collectAll(AnyAtom *start)
{
  if ( debug^2 ) {
    debug << "List of atoms currently live:" << endl ;
    foreach(i,AllAtoms,Plist<AnyAtom>) {
      debug << (void*)(*i) << " = " ;
      (*i)->show(debug);
    }
    debug << "Starting atom = " << (void*)start << endl ;
  }
    
  Plist<AnyAtom> the_stack ;
  collect_stack = &the_stack ;
  Plist<C_BasicBlock> *result = new Plist<C_BasicBlock> ;

  // find the first BB
  start->add_predecessor();          
  get_bb(start); // this puts the first BB on the stack
  while ( !the_stack.empty() ) {
    AnyAtom *atom = the_stack.back() ;
    AnyAtom *next ;
    the_stack.pop_back() ;
    C_BasicBlock *bb = atom->collected ;
    assert( bb != NULL );

    while(1) {
      atom->emit_stmt(bb);
      next = atom->get_next() ;
      if ( next == NULL || next->block_leader(bb) )
        break ;
      atom = next ;
    }
    if ( next == NULL )
      bb->exit(atom->create_jump()) ;
    else
      bb->exit(new C_Jump(get_bb(next)));
    result->push_back(bb);
        
    // create_jump() or get_bb() may have added new atoms to
    // the stack.
  }

  // Now when everything has been collected we can delete all
  // the atoms.
  foreach(i,AllAtoms,Plist<AnyAtom>)
    delete (AnyAtom*)*i ;
  AllAtoms.clear() ;

  return result ;
}

C_BasicBlock*
AnyAtom::get_bb(AnyAtom *atom) {
  assert( atom != NULL );
  while ( atom->predecessor_count == 0 ) {
    // the predecessor count is 0 for GotoAtoms which have been linked
    // (unless there is an empty perpetual loop)
    atom = atom->get_next() ;
    assert ( atom != NULL );
  }
  if ( atom->collected == NULL ) {
    atom->collected = new C_BasicBlock ;
    collect_stack->push_back(atom);
  }
  return atom->collected ;
}

C_Jump*
AnyAtom::create_jump()
{
  DONT_CALL_THIS ;
  return NULL ;
}

C_Jump*
IfAtom::create_jump()
{
  C_BasicBlock *thenbranch = get_bb(Then) ;
  C_BasicBlock *elsebranch = get_bb(Else) ;
  return new C_Jump(condition,thenbranch,elsebranch,pos);
}

C_Jump*
ReturnAtom::create_jump()
{
  return jump ;
}

// StopperAtom::create_jump() is part of the function translation

/************************************************************************
 ************************************************************************
 ***								      ***
 ***   P R O T O - F L O W G R A P H   I N T E R V A L S              ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

/* An Interval with a begin of NULL is a trivial interval: control
 * passes right through it.
 * An Interval with a nonzero begin but NULL as end has no official
 * exit node (i.e., exits either through an "unauthorized" jump or
 * a return statement or both).
 */

class Interval
{
public:
  AnyAtom *begin ;
  SimpleAtom *end ;
  Interval(AnyAtom* =NULL,SimpleAtom* =NULL);
  Interval(C_Stmt*);
  Interval(Plist<C_Stmt>&);

  Interval operator+(const Interval &) const;
  void operator +=(const Interval &);
  void force();
  bool trivial();
};

Interval::Interval(AnyAtom *a1,SimpleAtom *a2): begin(a1), end(a2)
{
  assert( a1 != NULL || a2 == NULL );
}

Interval::Interval(C_Stmt *s)
{
  assert( s != NULL );
  begin = end = new StmtAtom(s);
}

Interval::Interval(Plist<C_Stmt> &ss)
{
  begin = end = NULL ;
  foreach(i,ss,Plist<C_Stmt>) {
    StmtAtom *a = new StmtAtom(*i) ;
    if ( begin == NULL )
      begin = end = a ;
    else {
      end->link(a) ;
      end = a ;
    }
  }
}

Interval
Interval::operator+(const Interval &o) const
{
  Interval r(*this);
  r += o ;
  return r ;
}

void
Interval::operator+=(const Interval &o)
{
  if ( begin == NULL )
    *this = o ;
  else if ( o.begin != NULL ) {
    if ( end )
      end->link(o.begin) ;
    end = o.end ;
  }
}

bool
Interval::trivial()
{
  return begin == NULL ;
}

void
Interval::force()
{
  if ( begin == NULL ) {
    end = new GotoAtom ;
    begin = end ;
  }
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***   T H E    T R A N S L A T I O N    O F    T Y P E S             ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

struct DeferredExpr {
  Expr *original ;
  DeferredExpr(Expr *o) : original(o) {}
  void translate(CodeEnv&) ; // defined after the expression translation
  virtual void target(C_Expr *) =0;
  virtual ~DeferredExpr() {}
} ;

struct TypeEnv {
  CodeEnv &code ;
  Plist<DeferredExpr> *defer ; // NULL for translating right away
  bool make_anno_links ;

  TypeEnv(CodeEnv &);
};

TypeEnv::TypeEnv(CodeEnv &c)
  : code(c)
{
  defer = NULL ;
  make_anno_links = true ;
}

C_Type* AbsType::trans(TypeEnv const &env) {
  BaseTypeTag mytag ;
  if ( strchr(properties,'i') )
    if ( strchr(properties,'u') )
      mytag = UAbsT ;
    else
      mytag = AbsT ;
  else
    if ( strchr(properties,'a') )
      mytag = FloatingAbsT ;
    else
      mytag = Void ;
  C_Type *r = new C_Type(name,mytag);
  if ( env.make_anno_links )
    btannos.push_back(r);
  return r;
}

C_Type*
BaseType::trans(TypeEnv const &env)
{
  C_Type *r = new C_Type(tag);
  if ( env.make_anno_links )
    btannos.push_back(r);
  return r;
}

C_Type*
PtrType::trans(TypeEnv const&env)
{
  C_Type *nxt = next->trans(env);
  C_Type *r = new C_Type(nxt->tag==Function ? FunPtr:Pointer, nxt );
  if ( nxt->tag != Function )
    r->qualifiers(next->cv) ;
  if ( env.make_anno_links )
    btannos.push_back(r);
  return r;
}

struct DeferredArraySize : DeferredExpr {
  C_Type *tgt ;
  DeferredArraySize(Expr *o,C_Type *t) : DeferredExpr(o), tgt(t) {}
  void target(C_Expr *e) { tgt->array_size(e); }
  ~DeferredArraySize() {}
};

C_Type*
ArrayType::trans(TypeEnv const&env)
{
  assert(cv.subsetof(constvol())) ; // arrays are not const or volatile
  C_Type *r = new C_Type(Array,next->trans(env));
  if ( size == NULL )
    /* nothing */ ;
  else if ( env.defer != NULL )
    env.defer->push_back(new DeferredArraySize(size,r));
  else
    DeferredArraySize(size,r).translate(env.code) ;
  if ( env.make_anno_links )
    btannos.push_back(r);
  r->qualifiers(next->cv);
  return r ;
}

C_Type*
FunType::trans(TypeEnv const&env)
{
  assert(cv.subsetof(constvol())); // functions are not const or volatile
  // Tanslate the parameter types
  Plist<C_Type>* pars = new Plist<C_Type>;
  foreach(t, *params, Plist<Type>)
    pars->push_back((*t)->trans(env));
  C_Type *r = new C_Type(pars, ret->trans(env), varargs);
  if ( env.make_anno_links )
    btannos.push_back(r);
  return r ;
}

C_Type*
UserType::trans(TypeEnv const&env)
{
  assert(def != NULL);
  UserDecl *rdef = def ;
  while ( rdef->refer_to != NULL ) rdef = rdef->refer_to ;
  C_Type *r = rdef->trans(env);
  if ( env.make_anno_links )
    btannos.push_back(r);
  return r ;
}

C_Type*
StructDecl::trans(TypeEnv const&env)
{
  if ( rectrans != NULL )
    return rectrans ;
  assert( translated != NULL );
  C_Type *r = new C_Type(translated);
  rectrans = r ;
  foreach(i,*member_list,Plist<MemberDecl>) {
    r->user_types().push_back(i->var->type->trans(env)) ;
  }
  rectrans = NULL ;
  return r ;
}

C_Type*
EnumDecl::trans(TypeEnv const&)
{
  assert( translated != NULL );
  return new C_Type(translated);
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***   T R A N S L A T I O N    E N V I R O N M E N T S               ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

struct SwitchEnv {
  C_Expr *expr ;            // set at switch
  bool do_copy_expr ;
  AnyAtom *Default ;        // set at default
  Interval TestChain ;      // set at switch, CHANGED at case: labels
} ;

struct CodeEnv {
  C_Type *returnType ;      // used for annotating the source
  AnyAtom *returnTarget ;   // set at top, used at return w/o expr
  AnyAtom *breakTarget ;    // set at switch/loop, used at break;
  AnyAtom *continueTarget ; // set at loop, used at continue;
  SwitchEnv &Switch ; 
  Plist<C_Decl> *locals ;   // destination for local temporaries
  C_Pgm &program ;          // the program we translate into

  CodeEnv(SwitchEnv &sw, C_Pgm &pgm) 
    : returnType(NULL), returnTarget(NULL), breakTarget(NULL),
      continueTarget(NULL), Switch(sw),
      locals(&pgm.globals), program(pgm) {}
  CodeEnv(const CodeEnv &o)
    : returnType(o.returnType), returnTarget(o.returnTarget),
      breakTarget(o.breakTarget), continueTarget(o.continueTarget),
      Switch(o.Switch),
      locals(o.locals), program(o.program) {} 
  CodeEnv(const CodeEnv &o, SwitchEnv &sw)
    : returnType(o.returnType), returnTarget(o.returnTarget),
      breakTarget(o.breakTarget), continueTarget(o.continueTarget),
      Switch(sw), locals(o.locals), program(o.program) {}
};

/************************************************************************
 ************************************************************************
 ***								      ***
 ***   T R A N S L A T I O N    O F    E X P R E S S I O N S          ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

enum ExprTransTag { PREVALUE, VALUE, PLACE, STATEMENTS, BOOL, IGNORED };
struct ExprTrans {
  ExprTransTag tag ;  //     Used   Used        Used  Used     Used
  Position pos ;      //     Used   Used        Used  Used     Used
  Interval I ;        //     Used   Used        Used  Used     Used
  C_Expr *e ;         //     Used   Used
  Plist<C_Stmt> s ;   //     Used   Used        Used
  Plist<C_Type> tlist; //                       Used  Used
  SimpleAtom *omega0; //                              Used
  // the entry and "true" exit for BOOL is shared with I internally

  ExprTrans()
    : tag(IGNORED), e(NULL), omega0(NULL), do_copy_e(false)
    {}
    
  bool do_copy_e ;
  void force_e() ; // forces e to be owned

  void add_pre(Interval);

  // coercions:
  void Dereference() ;
  void Tempvar(CodeEnv &) ;
  void ZeroOne() ;
  void AssgnGen() ;
  void IfGen() ;
  void Ignore() ;
  void coerce(CodeEnv &,ExprTransTag);
  
  void set_left(C_Expr *e,bool do_copy); // only for STATEMENTS
  void conds(Plist<C_Type>&); // only for BOOL
};

void
ExprTrans::force_e()
{
  assert( tag == VALUE || tag == PLACE );
  if ( do_copy_e ) {
    e = e->copy() ;
    do_copy_e = false ;
  }
}

void
ExprTrans::add_pre(Interval J)
{
  // By sheer chance, this combination works for every kind
  // of ExprTrans.
  Interval K = J + I ;
  I = K ;
}
    
void
ExprTrans::Dereference()
{
  assert( tag == PLACE );
  force_e() ;
  e = new C_Expr(C_DeRef,e,pos);
  tag = VALUE ;
}

void
ExprTrans::Tempvar(CodeEnv &env)
{
  assert( tag == STATEMENTS );
  C_Decl *tmpvar = new C_Decl(VarDcl,tlist.front()->copy(),NULL);
  tlist.clear();
  env.locals->push_back(tmpvar);
  e = new C_Expr(tmpvar,pos);
  set_left(e,true); // clears the s list.
  do_copy_e = false ;
  tag = PLACE ;
}
    
void
ExprTrans::ZeroOne()
{
  assert( tag == BOOL );
  assert( s.empty() );
  SimpleAtom *a[2] ;
  static const char *consts[2] = { "0", "1" } ;
  for ( int i = 0 ; i <= 1 ; i++ ) {
    tlist.clear();
    tlist.push_back(new C_Type(Int));
    C_Expr *ex = new C_Expr(tlist.front(),consts[i],pos) ;
    C_Stmt *st = new C_Stmt(C_Assign,ex,pos);
    a[i] = new StmtAtom(st);
    s.push_back(st);
  }
  omega0->link(a[0]);
  I.end->link(a[1]);
  I.end = new GotoAtom ;
  a[0]->link(I.end);
  a[1]->link(I.end);
  omega0 = NULL ;
  tag = STATEMENTS ;
}

void
ExprTrans::AssgnGen()
{
  assert( tag == VALUE );
  force_e() ;
  C_Stmt *st = new C_Stmt(C_Assign,e,pos);
  I += st ;
  I += s ;
  s.clear() ;
  s.push_back(st) ;
  tlist.push_back(e->type) ;
  e = NULL ;
  tag = STATEMENTS ;
}
    
void
ExprTrans::IfGen()
{
  assert( tag == VALUE );
  assert( tlist.empty() );
  force_e() ;
  Interval Itrue, Ifalse ;
  foreach(i,s,Plist<C_Stmt>) {
    Itrue += i->copy() ;
    Ifalse += (Interval)*i ;
  }
  s.clear() ;
  Itrue.force() ;
  Ifalse.force() ;
  tlist.push_back(e->type);
  I += new IfAtom(e,Itrue.begin,Ifalse.begin,pos) ;
  assert( I.end == NULL );
  I.end = Itrue.end ;
  omega0 = Ifalse.end ;
  tag = BOOL ;
  e = NULL ;
}

void
ExprTrans::Ignore()
{
  switch ( tag ) {
  case VALUE:
  case PLACE:
    I += s ;
    s.clear() ;
    break ;
  case STATEMENTS:
    // do not set_left()
    // that way statements can disappear in the collection phase
    break ;
  case IGNORED:
    break ;
  default:
    assert(0) ;
  case BOOL:
    tlist.clear() ;
    SimpleAtom *end = new GotoAtom ;
    I.end->link(end);
    omega0->link(end);
    I.end = end ;
    break ;
  }
  tag = IGNORED ;
}      
                
void
ExprTrans::coerce(CodeEnv &env,ExprTransTag goal) {
  if ( goal == IGNORED )
    Ignore() ;
  else
    while ( tag != goal )
      switch(tag) {
      case VALUE:
        if ( goal == PREVALUE && s.empty() )
          return ;
        if ( goal == BOOL )
          IfGen();
        else
          AssgnGen();
        break ;
      case PLACE:
        Dereference() ;
        break ;
      case STATEMENTS:
        Tempvar(env);
        break ;
      case BOOL:
        ZeroOne() ;
        break ;
      case IGNORED:
        assert(0);
      default:
        assert(0);
      }
}

void
ExprTrans::set_left(C_Expr *e,bool do_copy)
{
  assert( tag == STATEMENTS );
  foreach(i,s,Plist<C_Stmt>) {
    if ( do_copy )
      e = e->copy() ;
    i->target(e);
    do_copy = true ;
  }
  s.clear() ;
  tag = IGNORED ;
}

void
ExprTrans::conds(Plist<C_Type> &btannos)
{
  assert( tag == BOOL );
  foreach(i,tlist,Plist<C_Type>)
    btannos.push_back(*i);
}

void
Expr::ok(ExprTrans &r) {
  r.pos = pos ;
  switch(r.tag) {
  case VALUE:
    assert(r.omega0==NULL);
    btannos.push_back(r.e->type);
    return ;
  case PLACE:
    assert(r.omega0==NULL);
    btannos.push_back(r.e->type->ptr_next());
    return ;
  case STATEMENTS:
    assert(r.e==NULL);
    assert(r.omega0==NULL);
    assert(r.tlist.size()==1);
    btannos.push_back(r.tlist.front());
    return ;
  case BOOL:
    assert(r.I.begin != NULL);
    assert(r.I.end != NULL);
    assert(r.omega0 != NULL);
    assert(r.e==NULL);
    assert(r.s.empty());
    r.conds(btannos);
    return ;
  default:
    ; /* this happens for free() calls */
  }
}

/************************************************************************
 * Special processing for heap allocation functions                     *
 ************************************************************************/

typedef bool CallTranslator(Plist<C_Expr>&,CodeEnv&,Position,ExprTrans &) ;

// This function tries to break the expression e up into zero or
// more factors and a sizeof expression. If there are more than one
// suitable sizeof the leftmost one is used (thus calloc calls will
// be parsed correctly).
// It can return NULL and leave factors untouched, or a pointer
// to a type and insert the other factors in factors.
static C_Type *
extractSizeof(C_Expr* e, Plist<C_Expr> &factors)
{
  switch(e->tag) {
  case C_Binary:
    if ( e->binary_op() == Mul ) {
      C_Type *t = extractSizeof(e->binary_expr2(),factors);
      if ( t ) {
        factors.push_front(e->binary_expr1());
        return t ;
      }
      t = extractSizeof(e->binary_expr1(),factors) ;
      if ( t ) {
        factors.push_back(e->binary_expr2());
        return t ;
      }
      return NULL ;
    }
    // else fall through
  default:
    return NULL ;
  case C_SizeofE: case C_SizeofT:
    return e->sizeof_type() ;
  case C_Cast:
    if ( e->subexpr()->type->tag == Primitive )
      return extractSizeof(e->subexpr(),factors);
    else
      return NULL ;
  }
}

static bool
convertMalloc(Plist<C_Expr> &args, CodeEnv &env, Position pos, ExprTrans &r)
{
  if ( args.size() != 1 || args.front()->type->tag != Primitive )
    return false ;
  Plist<C_Expr> factors ;
  C_Type *alloctype = extractSizeof(args.front(),factors) ;
  if ( alloctype == NULL )
    return false ;

  C_Type *alloc_arr_type = new C_Type(Array,alloctype);
        
  // we have a type and a list of factors. Collect the factors into
  // a single expression:
  if ( !factors.empty() ) {
    C_Expr *length = factors.back() ;
    factors.pop_back() ;
    foreach(f,factors,Plist<C_Expr>)
      length = new C_Expr(Mul,length,*f,Position()) ;
    alloc_arr_type->array_size(length);
  }
                             
  C_Decl *allocd = new C_Decl(VarDcl,alloc_arr_type,NULL);
  env.program.heap.push_back(allocd);
  C_Stmt *s = new C_Stmt(allocd,pos) ;
  s->isMalloc(true);
    
  r.I = s ;
  r.s.push_back(s) ;
  r.tlist.push_back(new C_Type(Pointer,alloctype)) ;
  r.tag = STATEMENTS ;
  return true ;
}

static bool
convertCalloc(Plist<C_Expr> &args, CodeEnv &env, Position pos, ExprTrans &r)
{
  if( args.size() != 2 ||
      args.front()->type->tag != Primitive ||
      args.back()->type->tag != Primitive )
    return false ;
  // cheat by using the same detection code as for malloc
  C_Expr pseudoarg(Mul,args.front(),args.back(),pos);
  Plist<C_Expr> pseudoargs ;
  pseudoargs.push_back(&pseudoarg);
  if ( !convertMalloc(pseudoargs,env,pos,r) )
    return false ;
  r.s.front()->isMalloc(false);
  return true ;
}
    
static bool
convertFree(Plist<C_Expr> &args, CodeEnv&, Position pos, ExprTrans &r)
{
  if( args.size() != 1 )
    return false ;
  C_Expr *e = args.front() ;
  if( e->type->tag != Pointer )
    return false ;
  r.I = new C_Stmt(C_Free,e,pos) ;
  r.tag = IGNORED ;
  return true ;
}

static CallTranslator*
special_external(const char *funname)
{
  static struct {
    const char *name ;
    CallTranslator *fun ;
  } const lookup_table[] = {
    { "malloc", convertMalloc },
    { "calloc", convertCalloc },
    { "free", convertFree },
  };

  int i = sizeof lookup_table / sizeof lookup_table[0] ;
  while ( i-- )
    if ( strcmp(funname,lookup_table[i].name) == 0 )
      return lookup_table[i].fun ;
  return NULL ;
}

/************************************************************************
 * The expression translation proper                                    *
 ************************************************************************/

void
ConstExpr::trans(CodeEnv &env,ExprTrans &r,bool)
{
  C_Type* t = type()->trans(env);
  if (tag==StrConst) {
    // Strings get an explicit declaration to make them objects.
    C_Decl *myconst = new C_Decl(VarDcl,t,NULL);
    myconst->init(new C_Init(literal));
    env.program.globals.push_back(myconst);

    r.e = new C_Expr(myconst,pos);
    r.tag = PLACE ;
  } else {
    r.e = new C_Expr(t,literal,pos);
    r.tag = VALUE ;
  }
  ok(r);
}

void
NullExpr::trans(CodeEnv &env, ExprTrans &r,bool)
{
  r.e = new C_Expr(C_Null,type()->trans(env),pos);
  r.tag = VALUE ;
  ok(r);
}

void
VarExpr::trans(CodeEnv &env, ExprTrans &r,bool)
{
  assert( isChecked() );
  assert( *type() == *decl->type );
  ObjectDecl *rdecl = decl ;
  while ( rdecl->refer_to != NULL ) rdecl = rdecl->refer_to ;
  if ( rdecl->tag == Vardecl ) {
    VarDecl *vd = (VarDecl*)rdecl ;
    switch ( vd->varmode ) {
    case VarConstant:
      // a magic constant. It has its own copy of the type
      // and should be translated into a Const
      r.e = new C_Expr(type()->trans(env),vd->name,pos);
      r.tag = VALUE ;
      break ;
    case VarEnum:
      // an enumeration constant.
      assert(vd->enum_translated != NULL);
      r.e = new C_Expr(vd->enum_translated,pos);
      r.tag = VALUE ;
      break ;
    default:
      assert(vd->translated != NULL);
      r.e = new C_Expr(vd->translated,pos);
      r.tag = PLACE ;
      break ;
    }
  } else {
    assert( rdecl->tag == Fundef );
    FunDef *fd = (FunDef*)rdecl ;
    if ( fd->stmts == NULL ) {
      // mention of an external function. We make an entirely new
      // external declaration for it.
      C_Type *funtype = new C_Type(FunPtr,type()->trans(env));
      C_Decl *decl = new C_Decl(ExtFun,funtype,fd->name);
      if ( fd->effects  ) fd->effects ->ApplyCore(decl);
      if ( fd->calltime ) fd->calltime->ApplyCore(decl);
      if ( annos        ) annos       ->ApplyCore(decl);
      env.program.exfuns.push_back(decl);
      r.e = new C_Expr(decl,pos);
      r.tag = PLACE ;
      // the type check has created an explicit decay cast for us
    } else {
      // mention of an internal function
      assert(fd->translated!=NULL);
      r.e = new C_Expr(fd->translated,pos);
      r.tag = PLACE ;
    }
  }
  ok(r);
}

void
CallExpr::trans(CodeEnv &env,ExprTrans &r,bool)
{
  assert( isChecked() );
  // Transform the components of the call.
  Interval precode ;
  Plist<C_Expr> *arglist = new Plist<C_Expr>;
  foreach( i, *args, Plist<Expr> ) {
    ExprTrans ei ;
    i->trans(env,ei,false);
    ei.coerce(env,PREVALUE);
    ei.force_e() ;
    arglist->push_back(ei.e);
    precode += ei.I ;
  }
  ExprTrans e0 ;
  fun->trans(env,e0,false); e0.coerce(env,PREVALUE);
  e0.force_e(); // if borrowed, we're going to make plain call anyway

  CallTranslator *ct = NULL ;
  if ( e0.e->tag == C_ExtFun )
    ct = special_external(e0.e->var()->get_name()) ;

  if ( ct != NULL && ct(*arglist,env,pos,r) ) {
    // e0 is a special function.
    delete arglist ;
    C_Decl *thefun = e0.e->var() ;
    if ( thefun->calltime() != NULL ) {
      Diagnostic d(ERROR,pos);
      d  << "calls to special functions such as " << e0.e->cnst()
         << " cannot have binding time annotations" ;
      d.addline(thefun->calltime()->pos) << "annotation here";
    }
    if ( thefun->effects() != NULL ) {
      Diagnostic d(ERROR,pos);
      d  << "calls to special functions such as " << e0.e->cnst()
         << " cannot have side effect annotations" ;
      d.addline(thefun->effects()->pos) << "annotation here";
    }
    assert( env.program.exfuns.back() == thefun );
    env.program.exfuns.pop_back() ;
    delete thefun ;
    delete e0.e ;
  } else {
    // it's not recognised as a special function. Generate plain call.
    C_Stmt *s = new C_Stmt(e0.e,arglist,pos);
    r.I = s ;
    r.s.push_back(s) ;
    r.tlist.push_back(e0.e->type->ptr_next()->fun_ret()) ;
    r.tag = STATEMENTS ;
  }
  // now r contains the call itself, but we need to add the preconditions.
  r.add_pre(e0.I);
  r.add_pre(precode);
  ok(r);
}

void
ArrayExpr::trans(CodeEnv &env,ExprTrans &r,bool)
{
  ExprTrans r2 ;
  left ->trans(env,r ,false); r .coerce(env,VALUE); r .force_e();
  right->trans(env,r2,false); r2.coerce(env,VALUE); r2.force_e();
  r.I += r2.I ;
  r.e = new C_Expr(Add,r.e,r2.e,pos);
  r.s.splice(r.s.end(),r2.s);
  r.tag = PLACE ;
  ok(r);

  // annotate the expression with the bt of the indexing operation
  btannos.clear();
  btannos.push_back(r.e->type);
}

void
DotExpr::trans(CodeEnv &env,ExprTrans &r,bool)
{
  left->trans(env,r,false); r.coerce(env,PLACE); r.force_e();
  r.e = new C_Expr(r.e,memindex,pos);
  ok(r);
}

// This function converts a Core C expression e into an expression
// that is equivalent to !e but whose topmost operator is ! only if
// strictly necessary.
static C_Expr *
reduce_bang(C_Expr *e,bool boolean_context,Position pos)
{
  switch(e->tag) {
  case C_Unary:
    if ( e->unary_op() == Bang ) {
      C_Expr *r = e->subexpr() ;
      // !!!E ==> !E is always safe, but !!E ==> E is only safe in a boolean
      // context (i.e., in recursive calls due to de Morgan).
      if ( boolean_context || r->tag == C_Unary && r->unary_op() == Bang ) {
        delete e ;
        return r ;
      }
    }
    break ;
  case C_PtrCmp:
  case C_Binary:
    switch( e->binary_op() ) {
    case LT: e->binary_op(GEq); return e;
    case GT: e->binary_op(LEq); return e;
    case LEq: e->binary_op(GT); return e;
    case GEq: e->binary_op(LT); return e;
    case Eq: e->binary_op(NEq); return e;
    case NEq: e->binary_op(Eq); return e;
    case And:
      // Use de Morgan's rule
      e->binary_op(Or);
      e->binary_expr1(reduce_bang(e->binary_expr1(),true,pos));
      e->binary_expr2(reduce_bang(e->binary_expr2(),true,pos));
      return e ;
    case Or:
      // Use de Morgan's rule
      e->binary_op(And);
      e->binary_expr1(reduce_bang(e->binary_expr1(),true,pos));
      e->binary_expr2(reduce_bang(e->binary_expr2(),true,pos));
      return e ;
    default:
      break ;
    }
    break ;
  case C_Cast:
  case C_SizeofE:
  case C_SizeofT:
  case C_PtrArith:
  case C_EnumCnst:
  case C_Cnst:
  case C_DeRef:
    break ;
  case C_Array:
  case C_Null:
  case C_Var:
  case C_FunAdr:
  case C_ExtFun:
  case C_Member:
    Diagnostic(INTERNAL,pos) << "trying to negate a pointer" ;
    break ;
  }
  return new C_Expr(Bang,e,pos);
}

void
UnaryExpr::trans(CodeEnv &env,ExprTrans &r,bool)
{
  exp->trans(env,r,false);
  switch(op) {
  case Addr:
    assert(r.tag==PLACE);
    r.tag = VALUE ;
    break ;
  case DeRef:
    r.coerce(env,VALUE);
    r.tag = PLACE ;
    break ;
  case Pos:
    break ;
  case Bang:
    if ( r.tag == BOOL ) {
      SimpleAtom *temp ;
      temp = r.I.end ;
      r.I.end = r.omega0 ;
      r.omega0 = temp ;
      break ;
    }
    r.coerce(env,VALUE);
    r.force_e();
    r.e = reduce_bang(r.e,false,pos) ;
    break ;
  default:
    r.coerce(env,VALUE); r.force_e();
    r.e = new C_Expr(op,r.e,pos);
    break ;
  }
  ok(r);
}

static void
TransAssignment(CodeEnv &env,ExprTrans &target,ExprTrans &source,
                Position pos,bool ignore)
{
  assert(target.tag==PLACE);
  C_Expr *saved_e = NULL;
  source.coerce(env,STATEMENTS);
  if ( !ignore ) {
    // the assignment itself may invalidate the expression
    // we're using for the right-hand side, so we need to
    // save its value in a temporary
    source.Tempvar(env);
    source.Dereference();
    saved_e = source.e ;
    source.AssgnGen();
  }
  source.set_left(target.e,target.do_copy_e);
  target.I += source.I ;
  if ( !ignore ) {
    target.e = saved_e ;
    target.do_copy_e = true ;
    target.tag = VALUE ;
  } else {
    target.I += target.s ;
    target.s.clear();
    target.tag = IGNORED ;
  }
}

static void
TransPresideeffect(CodeEnv &env,ExprTrans &left,BinOp bop,ExprTrans &right,
                   Position pos, bool ignore)
{
  assert(left.tag==PLACE);
  right.coerce(env,VALUE); right.force_e();
  C_Expr *read = new C_Expr(C_DeRef,left.e->copy(),left.pos);
  right.e = new C_Expr(bop,read,right.e,pos);
  TransAssignment(env,left,right,pos,ignore);
}

void
PreExpr::trans(CodeEnv &env,ExprTrans &r,bool ignore)
{
  // Resolve operation.
  BinOp bop = Add;
  switch(op) {
  case Inc:  bop = Add; break;
  case Decr: bop = Sub; break;
  }

  ExprTrans r2 ;
  r2.e = new C_Expr(new C_Type(Int),"1",pos) ;
  r2.tag = VALUE ;

  exp->trans(env,r,false);
  TransPresideeffect(env,r,bop,r2,pos,ignore);
  ok(r) ;
}

void
AssignExpr::trans(CodeEnv &env, ExprTrans &r,bool ignore)
{
  left->trans(env,r,false);
  ExprTrans r2 ;
  right->trans(env,r2,false);

  BinOp bop = Add;
  switch (op) {
  case Asgn:
    TransAssignment(env,r,r2,pos,ignore);
    ok(r);
    return ;
  case MulAsgn: bop = Mul; break;
  case DivAsgn: bop = Div; break;
  case ModAsgn: bop = Mod; break;
  case AddAsgn: bop = Add; break;
  case SubAsgn: bop = Sub; break;
  case LSAsgn:  bop = LShift; break;
  case RSAsgn:  bop = RShift; break;
  case AndAsgn: bop = BAnd; break;
  case EOrAsgn: bop = BEOr; break;
  case OrAsgn:  bop = BOr; break;
  }
  TransPresideeffect(env,r,bop,r2,pos,ignore);
  ok(r) ;
}

void
PostExpr::trans(CodeEnv &env,ExprTrans &r,bool)
{
  // Resolve operation.
  BinOp bop = Add;
  switch(op) {
  case Inc:  bop = Add; break;
  case Decr: bop = Sub; break;
  }
  exp->trans(env,r,false); assert(r.tag==PLACE); r.force_e() ;

  C_Expr *one = new C_Expr(new C_Type(Int),"1",pos) ;
  C_Expr *oldval = new C_Expr(C_DeRef,r.e->copy(),pos);
  C_Expr *result = new C_Expr(bop,oldval,one,pos) ;
  C_Stmt *s = new C_Stmt(C_Assign,result,pos);
  s->target(r.e);
    
  r.s.push_back(s) ;
  r.do_copy_e = true ;
  // r.tag == PLACE already
  ok(r) ;
}

void
TypeSize::trans(CodeEnv &env,ExprTrans &r,bool)
{
  r.tag = VALUE ;
  r.e = new C_Expr(C_SizeofT,typesz->trans(env),pos);
  ok(r) ;
}

void
ExprSize::trans(CodeEnv &env,ExprTrans &r,bool)
{
  exp->trans(env,r,false) ; r.coerce(env,VALUE) ; r.force_e() ;
  r.I = Interval() ;
  r.e = new C_Expr(C_SizeofE,r.e,pos) ;
  r.s.clear() ;
  ok(r) ;
}

void
CastExpr::trans(CodeEnv &env,ExprTrans &r,bool)
{
  exp->trans(env,r,false);
  if ( exp->type()->realtype == FunT ) {
    // pointer decay for functions
    assert(r.tag == PLACE);
    assert( type()->isPointer() == exp->type() );
    r.tag = VALUE ;
    ok(r) ;
    return ;
  }
  if ( exp->type()->realtype == ArrayT ) {
    // pointer decay for arrays
    assert(r.tag == PLACE); r.force_e() ;
    assert(type()->realtype == PtrT );
    r.e = new C_Expr(C_Array,r.e,pos);
    r.tag = VALUE ;
    ok(r) ;
    return ;
  }
  if ( type()->isVoid ) {
    // casts to void result from expression statements
    // that are not assignments. The program can also
    // specify them difectly. In any case they have no
    // place in the Core C program.
    r.Ignore();
    ok(r);
    return ;
  }
  TypeEnv te(env);
  if ( silent )
    te.make_anno_links = false ;
  C_Type* t = type()->trans(te);
  switch(r.tag) {
  case BOOL:
    r.ZeroOne() ;
    // fall through
  case STATEMENTS:
    if ( t->equal(*r.tlist.front()) ) {
      // casts from a type to itself are ignored. This is useful for
      // constructions like
      //     (int*)malloc(42*sizeof(int)) ;
      // where the magic recognition of mallocs have already made an
      // alloc statement with pointer to int type
      ok(r) ;
      return ;
    }
    // else fall through once again
  default:
    r.coerce(env,VALUE);
    if ( t->equal(*r.e->type) ) {
      // once again try to ignore trivial casts
      ok(r) ;
      return ;
    }
    r.force_e() ;
    break ;
  }
  if ( ( t->tag==Pointer || t->tag==FunPtr ) && r.e->tag == C_Null ) {
    // a null pointer constant cast to another pointer type is
    // just another null pointer constant
    r.e->type = t ;
    ok(r) ;
    return ;
  }
  r.e = new C_Expr(t,r.e,pos);
  ok(r) ;
}

static bool
ConstantLogic(ExprTrans &r1,ExprTrans &r2)
{
  return ( r1.tag == PLACE || r1.tag == VALUE ) &&
    r1.s.empty() &&
    ( r2.tag == PLACE || r2.tag == VALUE ) &&
    r2.I.trivial() &&
    r2.s.empty() ;
}

void
BinaryExpr::trans(CodeEnv &env, ExprTrans &r,bool)
{
  ExprTrans r2 ;
  left->trans(env,r,false) ;
  right->trans(env,r2,false) ;
  switch(op) {
  case And:
    if ( ConstantLogic(r,r2) )
      break ;
    else {
      r.coerce(env,BOOL);
      r2.coerce(env,BOOL);
      r.I.end->link(r2.I.begin);
      SimpleAtom *o0 = new GotoAtom ;
      r.omega0->link(o0) ;
      r2.omega0->link(o0) ;
      r.I.end = r2.I.end ;
      r.omega0 = o0 ;
      r2.conds(r.tlist);
      ok(r);
      return ;
    }
  case Or:
    if ( ConstantLogic(r,r2) )
      break ;
    else {
      r.coerce(env,BOOL);
      r2.coerce(env,BOOL);
      r.omega0->link(r2.I.begin);
      SimpleAtom *o1 = new GotoAtom ;
      r.I.end->link(o1) ;
      r2.I.end->link(o1) ;
      r.omega0 = r2.omega0 ;
      r.I.end = o1 ;
      r2.conds(r.tlist);
      ok(r);
      return ;
    }
  case Div:
    // special handling for "sizeof a / sizeof a[0]"
    // and related idioms
    if ( r.tag == VALUE && r2.tag == VALUE ) {
      C_Type *t1 ;
      C_Type *t2 ;
      if ( r.e->tag == C_SizeofT || r.e->tag == C_SizeofE )
        t1 = r.e->sizeof_type() ;
      else break ;
      if ( r2.e->tag == C_SizeofT || r2.e->tag == C_SizeofE )
        t2 = r2.e->sizeof_type() ;
      else break ;
      if ( t1->tag == Array && t1->hasSize() &&
           t1->array_next()->equal(*t2) ) {
        r.I = Interval() ;
        r.s.clear() ;
        r.e = t1->array_size() ;
        r.do_copy_e = true ;
        ok(r);
        return ;
      }
    }
  default:
    break;
  }
  r.coerce(env,VALUE); r.force_e() ;
  r2.coerce(env,VALUE); r2.force_e() ;
  r.I += r2.I ;
  r.e = new C_Expr(op,r.e,r2.e,pos);
  r.s.splice(r.s.end(),r2.s);
  ok(r) ;
}

void
CommaExpr::trans(CodeEnv &env, ExprTrans &r,bool ignore)
{
  ExprTrans r0 ;
  left->trans(env,r0,true); r0.coerce(env,IGNORED);
  right->trans(env,r,ignore);
  r.add_pre(r0.I);
  ok(r) ;
}

void
CondExpr::trans(CodeEnv &env, ExprTrans &r1,bool)
{
  ExprTrans r0 ;
  ExprTrans r2 ;
  cond->trans(env,r0,false); r0.coerce(env,BOOL);
  left->trans(env,r1,false); r1.coerce(env,STATEMENTS); r1.I.force();
  right->trans(env,r2,false); r2.coerce(env,STATEMENTS); r2.I.force();
  SimpleAtom *end = new GotoAtom ;
  r0.I.end->link(r1.I.begin);
  r0.omega0->link(r2.I.begin);
  r1.I.end->link(end);
  r2.I.end->link(end);

  r1.I.begin = r0.I.begin ;
  r1.I.end = end ;
  r1.s.splice(r1.s.end(),r2.s);

  ok(r1);

  // annotate the expression with the condition's colours
  btannos.clear();
  r0.conds(btannos);
}

static C_Expr*
ConstantExpr(Expr *e,CodeEnv &env)
{
  ExprTrans r ;
  e->trans(env,r,false);
  r.coerce(env,PREVALUE);
  r.force_e() ;
  if ( !r.I.trivial() ) {
    Diagnostic d(ERROR,e->pos);
    d << "the expression " ;
    e->show(d) ;
    d.addline() << "must be constant but is not." ;
  }
  return r.e ;
}

void
DeferredExpr::translate(CodeEnv &env)
{
  target(ConstantExpr(original,env));
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***   T R A N S L A T I O N    O F    S T A T E M E N T S            ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

static void
FullExpr(Expr *e,CodeEnv &env,ExprTrans &r,ExprTransTag m)
{
  e->trans(env,r,m==IGNORED);
  r.coerce(env,m);
}

Interval
LabelStmt::trans(CodeEnv &env)
{
  // Check whether an atom has been reserved already. If not, make one
  if (translated==NULL)
    translated = new GotoAtom ;
  return Interval(translated,translated) + stmt->trans(env);
}

Interval
CaseStmt::trans(CodeEnv &env)
{
  if (env.Switch.expr == NULL) {
    Diagnostic(ERROR,pos) << "case is not inside a switch." ;
    return Interval();
  }
    
  // Transform the statement that should be executed if the case matches.
  Interval s3 = stmt->trans(env);
  // The case label. The expression should be a constant.
  C_Expr *label = ConstantExpr(exp,env);
  // Make our own copy of the switch expression
  if ( env.Switch.do_copy_expr )
    env.Switch.expr = env.Switch.expr->copy() ;
  env.Switch.do_copy_expr = true ;
  // Make the comparison expression
  C_Expr* cond = new C_Expr(Eq,env.Switch.expr,label,pos);
  // Create targets for the branch
  s3.force() ;
  GotoAtom *nexttest = new GotoAtom ;
  // Make the branch
  IfAtom *thebranch = new IfAtom(cond,s3.begin,nexttest,pos);
  // Install it in the chain
  env.Switch.TestChain += Interval(thebranch,nexttest);

  btannos.push_back(env.Switch.expr->type);
  return s3;
  /* The result should be:

    (previous test)             (result.begin)
           |                          |
           V                          V
      +------------+           +-----------------+
      |  if ...    |---------->| translated stmt |
      +------------+           |                 |
           |                   |     ....        |
           V                   |                 |
         +---+                 +--- result.end --+
         |   | 
         +---+ ( <-- jump to next test will be installed here
  */
}

Interval
DefaultStmt::trans(CodeEnv &env)
{
  if (env.Switch.expr == NULL )
    Diagnostic(ERROR,pos) << "default is not inside a switch." ;
  if (env.Switch.Default != NULL )
    Diagnostic(ERROR,pos) << "more than one default: in a switch." ;
    
  Interval s3 = stmt->trans(env);
  s3.force() ;
  env.Switch.Default = s3.begin ;

  btannos.push_back(env.Switch.expr->type);
  return s3 ;
}


Interval
CompoundStmt::trans(CodeEnv &env)
{
  Interval code ;
  // Put regular declarations in the local list
  foreach(i,*objects,Plist<VarDecl>) {
    i->translated = new C_Decl(VarDcl,i->type->trans(env),i->name);
    Interval init = i->trans(env,!i->static_local) ;
    if ( i->static_local ) {
      if ( !init.trivial() ) {
        Diagnostic(ERROR,i->pos)
          << "initializer for `static' variable " << i->name
          << " is not constant" ;
      }
      env.program.globals.push_back(i->translated) ;
    } else {
      if ( i->translated->hasInit() ) {
        SimpleAtom *init = new InitAtom(i->translated);
        code += Interval(init,init);
      }
      code += init ;
      env.locals->push_back(i->translated) ;
    }
        
  }
  // Transform each statement;
  foreach( s, *stmts, Plist<Stmt> ) code += s->trans(env) ;
  return code;
}

Interval
ExprStmt::trans(CodeEnv &env)
{
  ExprTrans e ;
  FullExpr(exp,env,e,IGNORED);
  return e.I ;
}

Interval
IfStmt::trans(CodeEnv &env)
{
  ExprTrans e ;
  FullExpr(exp,env,e,BOOL);
  e.conds(btannos);
  GotoAtom *end = new GotoAtom ;
  Interval th = thn->trans(env) + end ; // th is not trivial
  Interval el = els->trans(env) + end ; // el is not trivial
  e.I.end ->link(th.begin) ;
  e.omega0->link(el.begin) ;
  return Interval(e.I.begin,end);
}

Interval
DummyStmt::trans(CodeEnv &)
{
  return Interval() ;
}

Interval
SwitchStmt::trans(CodeEnv &env)
{
  ExprTrans e ;
  FullExpr(exp,env,e,PREVALUE);
  GotoAtom *end = new GotoAtom ;

  SwitchEnv env2a ;
  CodeEnv env2(env,env2a) ;
  env2.breakTarget = end ;
  env2a.expr = e.e ;
  env2a.do_copy_expr = e.do_copy_e ;
  env2a.Default = NULL ;
  env2a.TestChain = e.I ;
    
  stmt->trans(env2) + env2.breakTarget ;
  btannos.push_back(env2a.expr->type);

  if ( env2a.Default == NULL )
    env2a.Default = end ;
  env2a.TestChain += env2a.Default ;
  return Interval(env2a.TestChain.begin,end);
}

Interval
WhileStmt::trans(CodeEnv &env)
{
  ExprTrans cond ;
  FullExpr(exp,env,cond,BOOL);
  cond.conds(btannos);
  GotoAtom *end = new GotoAtom ;
  cond.omega0->link(end) ;
    
  CodeEnv env2(env) ;
  env2.breakTarget = end ;
  env2.continueTarget = cond.I.begin ;
  Interval s = stmt->trans(env2) ;
    
  s += cond.I.begin ; // s is now not trivial
  cond.I.end->link(s.begin);

  return Interval(cond.I.begin,end);
}

// a do loop is the same as a while loop except the entry point is different
Interval
DoStmt::trans(CodeEnv &env)
{
  ExprTrans cond ;
  FullExpr(exp,env,cond,BOOL);
  cond.conds(btannos);
  GotoAtom *end = new GotoAtom ;
  cond.omega0->link(end) ;
    
  CodeEnv env2(env) ;
  env2.breakTarget = end ;
  env2.continueTarget = cond.I.begin ;
  Interval s = stmt->trans(env2) ;
    
  s += cond.I.begin ; // s is now not trivial
  cond.I.end->link(s.begin);

  return Interval(s.begin,end);
}

Interval
ForStmt::trans(CodeEnv &env)
{
  ExprTrans e1f ;
  if ( e1 != NULL )
    FullExpr(e1,env,e1f,IGNORED);
    
  ExprTrans cond ;
  if ( e2 != NULL ) {
    FullExpr(e2,env,cond,BOOL);
    cond.conds(btannos);
  } else {
    cond.I.force();
    cond.omega0 = new GotoAtom ;
  }

  ExprTrans e3f ;
  if ( e3 != NULL )
    FullExpr(e3,env,e3f,IGNORED);

  e1f.I += cond.I.begin ; // now e1f.I is not trivial
  e3f.I += cond.I.begin ; // now e3f.I is not trivial
    
  GotoAtom *end = new GotoAtom ;
  cond.omega0->link(end) ;    
    
  CodeEnv env2(env) ;
  env2.breakTarget = end ;
  env2.continueTarget = e3f.I.begin ;

  Interval s = stmt->trans(env2);

  s += e3f.I ; // now s is not trivial because e3f was not
  cond.I.end->link(s.begin);

  return Interval(e1f.I.begin,end);
}

Interval
GotoStmt::trans(CodeEnv &env)
{
  // Get the label statement through the indirection.
  SimpleAtom* target = ind->stmt->translated;
  if (target==NULL) {
    // Reserve an atom
    target = new GotoAtom ;
    ind->stmt->translated = target;
  }
  return Interval(target,NULL) ;
}

Interval
BreakStmt::trans(CodeEnv &env)
{
  if (env.breakTarget == NULL)
    Diagnostic(ERROR,pos) << "break statement not inside proper block" ;
  return Interval(env.breakTarget,NULL);
}

Interval
ContStmt::trans(CodeEnv &env)
{
  if (env.continueTarget == NULL)
    Diagnostic(ERROR,pos) << "continue statement not inside proper block" ;
  return Interval(env.continueTarget,NULL);
}

Interval
ReturnStmt::trans(CodeEnv &env)
{
  btannos.push_back(env.returnType);
  if (exp==NULL)
    return Interval(env.returnTarget,NULL);
  ExprTrans e;
  FullExpr(exp,env,e,PREVALUE);
  e.force_e() ;
  return e.I + new ReturnAtom(new C_Jump(e.e,pos));
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***   T R A N S L A T I O N    O F    U S E R T Y P E    D E F N S   ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

struct DeferredMemberExpr : DeferredExpr {
  C_UserMemb *tgt ;
  DeferredMemberExpr(Expr *o,C_UserMemb *t) : DeferredExpr(o), tgt(t) {}
  virtual void target(C_Expr *e) { tgt->value(e); }
  ~DeferredMemberExpr() ;
};

DeferredMemberExpr::~DeferredMemberExpr()
{
}

void
StructDecl::trans(C_Pgm &pgm,TypeEnv const&env) {
  assert(translated == NULL);
  translated = new C_UserDef(defseqnum,name,pos,tag == Union);
  foreach(i,*member_list,Plist<MemberDecl>) {
    C_UserMemb *um = new C_UserMemb(i->var->name);
    if ( i->bitfield != NULL )
      env.defer->push_back(new DeferredMemberExpr(i->bitfield,um)) ;
    um->qualifiers = i->var->type->cv ;
    translated->names.push_back(um);
  }
  pgm.usertypes.push_back(translated);
}

void
EnumDecl::trans(C_Pgm &pgm,TypeEnv const&env) {
  assert(translated == NULL);
  translated = new C_EnumDef(defseqnum,name,pos);
  foreach(i,*member_list,Plist<VarDecl>) {
    C_UserMemb *um = new C_UserMemb(i->name);
    i->enum_translated = um ;
    if ( i->init ) {
      assert(i->init->tag == InitElem);
      env.defer->push_back(new DeferredMemberExpr(i->init->expr(),um)) ;
    }
    translated->members().push_back(um);
  }
  pgm.enumtypes.push_back(translated);
}

static void
trans_usertypes(CProgram &orig,C_Pgm &trans,TypeEnv const&env)
{
  foreach(i,*orig.usertypes,Plist<UserDecl>)
    if( i->refer_to == NULL)
      i->trans(trans,env);
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***  T R A N S L A T I O N   O F   G L O B A L S ,   P A S S   I     ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

static void
trans_globals1(CProgram &orig,C_Pgm &trans,TypeEnv const&env)
{
  foreach(i,*orig.functions,Plist<FunDef>) {
    if ( i->stmts == NULL )
      continue ; // do not translate external functions
    assert(i->refer_to == NULL);
    assert(i->translated == NULL);

    C_Type *funtype = i->type->trans(env) ;
    i->translated = new C_Decl(FunDf,funtype,i->name);
    if ( debug ) debug << "made fun frame: " << (void*)i->translated
                       << " for " << i->name << endl ;

    // translate parameters
    Plist<C_Type>::iterator ti = funtype->fun_params().begin();
    foreach(pi,*i->decls,Plist<VarDecl>)
      pi->translated = new C_Decl(VarDcl,*ti++,pi->name) ;
    assert(!ti);
  }
  foreach(ii,*orig.definitions,Plist<VarDecl>) {
    if ( ii->refer_to != NULL )
      continue ; // only translate the main version of each global
    assert(ii->translated == NULL);
    switch(ii->varmode) {
    case VarConstant:
      break ; // do not translate well-known constants
    case VarMu:
      break ; // do not translate typedefs
    case VarEnum:
      assert(0); // this can't happen, can it?
    default:
      ii->translated = new C_Decl(VarDcl,ii->type->trans(env),ii->name);
      if ( debug ) debug << "made var frame: " << (void*)ii->translated
                         << " for " << ii->name << endl ;
    }
  }
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***  T R A N S L A T I N G   I N I T I A L I Z E R S                 ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

C_Init *
Init::trans(CodeEnv &env)
{
  if ( tag == InitElem ) {
    if ( sloppy )
      // sloppy elems is a special code for string initialization
      return new C_Init(exp->string_literal());
    else {
      ExprTrans r ;
      FullExpr(exp,env,r,VALUE) ;
      if ( !r.I.trivial() || !r.s.empty() )
        Diagnostic(ERROR,exp->pos)
          << "initializer must not have side effects" ;
      r.force_e() ;
      return new C_Init(r.e);
    }
  } else {
    Plist<C_Init> *translist = new Plist<C_Init> ;
    foreach(sub,*inits,Plist<Init>)
      translist->push_back(sub->trans(env));
    return new C_Init(sloppy?SloppyBraced:FullyBraced, translist);
  }
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***  T R A N S L A T I O N   O F   G L O B A L S ,   P A S S   I I   ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

C_Jump*
StopperAtom::create_jump()
{
  C_Expr *e ;
  switch(rettype->tag) {
  case FunPtr:
  case Pointer:
    e = new C_Expr(C_Null, rettype->copy(), pos);
    break ;
  case Primitive:
    e = new C_Expr(rettype->copy(), "0", pos);
    break ;
  case Abstract:
    if ( rettype->isVoid() ) {
      e = NULL ;
      break ;
    }
    // else fall through
  default:
    // we cannot create expressions of this type easily; create
    // a variable and use its value
    C_Decl *cheat = new C_Decl(VarDcl,rettype->copy(),"bare_return");
    env.program.globals.push_back(cheat);
    e = new C_Expr(C_DeRef, new C_Expr(cheat,pos), pos);
    break ;
  }
  if ( e != NULL )
    Diagnostic(WARNING,pos) << "return without a value in "
      "non-void function" ;

  return new C_Jump(e,pos);
}

void
FunDef::trans(CodeEnv &env)
{
  if ( debug ) debug << "translating " << name << "()" << endl ;

  C_Decl *const f = translated ;
  assert(f != NULL);

  f->pos = pos ;
    
  CodeEnv theenv(env);
  theenv.locals = new Plist<C_Decl> ;
  theenv.returnType = f->type->fun_ret() ;
  theenv.returnTarget = new StopperAtom(f->type->fun_ret(),env,pos);
                          
  Plist<C_Decl> *paramlist = new Plist<C_Decl>;
    
  // translate parameters
  foreach(pi,*decls,Plist<VarDecl>) {
    Interval I = pi->trans(theenv) ;
    assert( I.trivial() );
    paramlist->push_back(pi->translated);
  }
    
  Interval code = stmts->trans(theenv) ;
  code += theenv.returnTarget ; // now code cannot be trivial
    
  f->blocks(AnyAtom::collectAll(code.begin));
  f->fun_params(paramlist);
  f->fun_locals(theenv.locals);
}        

Interval
VarDecl::trans(CodeEnv &env,bool PreferStatements)
{
  if ( debug ) debug << "translating var " << name << endl ;
  assert(translated != NULL);
  translated->pos = pos ;
  translated->varmode(varmode,varmodeWhy);
  translated->qualifiers = type->cv;

  // Unless there is an initialiser, that is all there is to it.
  if ( init == NULL )
    return Interval() ;

  // only single-element initializers may result in code
  //   
  // but exclude strings that initialize character arrays
  if ( init->tag == InitElem && !init->sloppy ) {
    ExprTrans r ;
    FullExpr(init->expr(),env,r,VALUE) ;
    r.force_e();

    // use a proper initializer if possible, unless statements
    // have explicitly been requested.
    if ( r.I.trivial() && r.s.empty() && !PreferStatements ) {
      translated->init(new C_Init(r.e));
      return Interval() ;
    }

    // non-simple initializer: generate assignment
    C_Stmt *s = new C_Stmt(C_Assign,r.e,pos);
    s->target(new C_Expr(translated,pos));
    return r.I + s + r.s ;
  }

  translated->init(init->trans(env)) ;
  return Interval() ;
}

static void
trans_globals2(CProgram &orig,C_Pgm &trans,CodeEnv &env)
{
  foreach(i,*orig.functions,Plist<FunDef>)
    if ( i->translated != NULL ) {
      i->trans(env) ;
      trans.functions.push_back(i->translated);
    }
    
  foreach(ii,*orig.definitions,Plist<VarDecl>)
    if ( ii->translated != NULL ) {
      Interval init = ii->trans(env) ;
      if ( !init.trivial() ) {
        Diagnostic(ERROR,ii->pos)
          << "initializer for global variable " << ii->name
          << " is not constant" ;
      }
      trans.globals.push_back(ii->translated) ;
    }
}

/************************************************************************
 ************************************************************************
 ***								      ***
 ***  F I T T I N G   I T   A L L   T O G E T H E R                   ***
 ***								      ***
 ************************************************************************
 ************************************************************************/

void
c2core(CProgram &orig,C_Pgm &trans)
{
  // Pass -1: environments
    
  SwitchEnv switchenv ;
  switchenv.expr = NULL ;
  CodeEnv codeenv(switchenv,trans);
  Plist<DeferredExpr> defer ;
  TypeEnv typeenv(codeenv) ;
  typeenv.defer = &defer ;

  // Pass 0: translate usertypes
    
  trans_usertypes(orig,trans,typeenv);

  // Pass I: make placeholder declarations for globals

  trans_globals1(orig,trans,typeenv);

  // Pass II 1/2: translate the deferred expressions

  foreach(i,defer,Plist<DeferredExpr>) {
    i->translate(codeenv) ;
    delete &**i ;
  }
  defer.clear() ;

  // Pass II: tranlate function bodies and initializers

  trans_globals2(orig,trans,codeenv);

  // check for unresolved special functions and callmodes

  foreach(ii,trans.exfuns,Plist<C_Decl>) {
    if ( special_external(ii->get_name()) )
      Diagnostic(WARNING,ii->pos)
        << "this instance of " << ii->get_name()
        << "() could not be translated into the internal "
        << ii->get_name() << " representation." ;
  }

  // remove usertypes without instances
  // (without instances the member types of a C_UserDef are indeterminate;
  // this would severely confuse some of the following analyses).

  for( Plist<C_UserDef>::mutable_iterator iii = trans.usertypes.begin() ;
       iii ; ) {
    if ( iii->instances.empty() )
      trans.usertypes.erase(iii);
    else
      iii++ ;
  }
}
