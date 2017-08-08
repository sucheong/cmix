/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Represention of a CoreC program
 *
 * Copyright © 1998-1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdio.h>
#include "Plist.h"
#include <string.h>
#include "corec.h"
#include "auxilary.h"
#include "diagnostic.h"
#include "options.h"
#include "strings.h"
#include "symboltable.h"

#define debug debugstream_corec

//////////////////////////// Types /////////////////////////

static unsigned do_hash(char const *name) {
  static SymbolTable<unsigned> hash ;
  static unsigned counter = 1 ;

  char const *p ;
  for(p=name; *p; p++)
    if (*p == ' ')
      name = p+1 ;

  unsigned *pu = hash.lookup(name) ;
  if ( pu == NULL ) {
    pu = new unsigned(counter++) ;
    hash.insert(name,pu);
  }
  return *pu ;
}

// Base type
C_Type::C_Type(BaseTypeTag bt)
  : Numbered(Numbered::C_TYPE), tag(bt==Void ? Abstract : Primitive)
{
  abstract.id = basetype2str(bt);
  abstract.tag = bt ;
  abstract.hash = do_hash(abstract.id) ;
}

// Abstract
C_Type::C_Type(char const *n,BaseTypeTag bt)
  : Numbered(Numbered::C_TYPE), tag(bt==Void ? Abstract : Primitive)
{
  abstract.id = n ;
  abstract.tag = bt ;
  abstract.hash = do_hash(n);
}

// Pointer or array
C_Type::C_Type(C_TypeTag t, C_Type* typ)
  : Numbered(Numbered::C_TYPE), tag(t)
{
  switch(tag) {
  case Pointer:
    assert(typ->tag != Function);
    pointer.nxt = typ;
    break ;
  case FunPtr:
    assert(typ->tag == Function);
    pointer.nxt = typ;
    break ;
  case Array:
    assert(typ->tag != Function);
    arr.next = typ;
    arr.size = NULL;
    break ;
  default:
    assert(0);
  }
}

// Function
C_Type::C_Type(Plist<C_Type> *p, C_Type *typ, bool var)
  : Numbered(Numbered::C_TYPE), tag(Function)
{
  fun.params = p;
  fun.ret = typ;
  fun.varargs = var;
}

// Struct/Union.
C_Type::C_Type(C_UserDef *d)
  : Numbered(Numbered::C_TYPE), tag(StructUnion)
{
  usertype.types = new Plist<C_Type> ;
  user_def(d);
  d->new_instance(this);
}

// Enum.
C_Type::C_Type(C_EnumDef* d)
  : Numbered(Numbered::C_TYPE), tag(EnumType)
{ enumtype.def = d; }


C_UserDef*
C_Type::user_def() const
{
  assert(tag == StructUnion);
  return usertype.def;
}

Plist<C_Type>&
C_Type::user_types() const
{
  assert(tag == StructUnion);
  return *usertype.types;
}

char const*
C_Type::primitive() const {
  assert( tag == Abstract || tag == Primitive );
  return abstract.id ;
}

BaseTypeTag
C_Type::basetype() const {
  assert( tag == Abstract || tag == Primitive );
  return abstract.tag ;
}

unsigned
C_Type::hash() const {
  assert( tag == Abstract || tag == Primitive );
  return abstract.hash ;
}

C_EnumDef*
C_Type::enum_def() const {
  assert( tag == EnumType );
  return enumtype.def;
}


C_Type*
C_Type::ptr_next() const
{
  assert(tag == Pointer || tag == FunPtr);
  return pointer.nxt;
}

bool
C_Type::hasSize() const
{
  assert(tag == Array);
  return arr.size != NULL ;
}

C_Expr*
C_Type::array_size() const
{
  assert(tag == Array && arr.size);
  return arr.size;
}

C_Type*
C_Type::array_next() const
{
  assert(tag == Array);
  return arr.next;
}

constvol
C_Type::qualifiers() const
{
  assert(tag == Array || tag == Pointer || tag == FunPtr);
  return cv ;
}
                
Plist<C_Type>&
C_Type::fun_params() const
{
  assert(tag == Function);
  return *fun.params;
}

C_Type*
C_Type::fun_ret() const
{
  assert(tag == Function);
  return fun.ret;
}

bool
C_Type::fun_varargs() const
{
  assert(tag == Function);
  return fun.varargs;
}

void
C_Type::user_def(C_UserDef* d)
{
  assert(tag == StructUnion && d);
  usertype.def = d;
}

void
C_Type::array_size(C_Expr *size)
{
  assert(tag == Array && size != NULL);
  arr.size = size;
}

void
C_Type::qualifiers(constvol qual)
{
  assert(tag == Array || tag == Pointer);
  cv = qual ;
}

bool
C_Type::isVoid() const
{
  return(tag==Abstract && abstract.tag==Void);
}

// Get the first non-array type.
C_Type*
C_Type::array_contents_type()
{
  C_Type* t = this;
  while (t->tag==Array) t = t->array_next();
  return t;
}

// Deep copy of a type. Two lists keep track of which struct types
// that are already copied.
static C_Type*
deep_copy(C_Type* t, Plist<C_Type>& origs, Plist<C_Type>& copies)
{
  C_Type* newOne = NULL;
  switch (t->tag) {
  case StructUnion: {
    // Check whether this struct type has already been copied. If it
    // has, return that copy.
    C_Type* found = NULL;
    double_foreach(orig, copy, origs, copies, Plist<C_Type>) {
      if ( t == (*orig) ) { found = *copy; break; }
    }
    if (found) return found;
    // It has not been copied. Insert it into the lists.
    origs.push_back(t);
    newOne = new C_Type(t->user_def());
    copies.push_back(newOne);
    // Copy it.
    Plist<C_Type>& newUserTypes = newOne->user_types();
    foreach(member, t->user_types(), Plist<C_Type>) 
      newUserTypes.push_back( deep_copy((*member),origs,copies) );
    // Remove this copy from the list.
    assert(origs.back() == t && copies.back() == newOne);
    origs.pop_back(); copies.pop_back();
    break; }
  case Pointer:
    newOne = new C_Type(Pointer, deep_copy(t->ptr_next(), origs, copies));
    newOne->qualifiers(t->qualifiers());
    break;
  case FunPtr:
    newOne = new C_Type(FunPtr, deep_copy(t->ptr_next(), origs, copies));
    break;
  case Array:
    newOne = new C_Type(Array, deep_copy(t->array_next(), origs, copies));
    if ( t->hasSize() )
      newOne->array_size(t->array_size()->copy());
    newOne->qualifiers(t->qualifiers());
    break;
  case Function: {
    Plist<C_Type>* newParTypes = new Plist<C_Type>;
    foreach(param, t->fun_params(), Plist<C_Type>) 
      newParTypes->push_back( deep_copy((*param), origs, copies) );
    newOne = new C_Type(newParTypes,
			deep_copy(t->fun_ret(), origs, copies),
                        t->fun_varargs());  
    break; }
  case Abstract:
  case Primitive:
    newOne = new C_Type(t->primitive(),t->basetype());
    break;
  case EnumType:
    newOne = new C_Type(t->enum_def());
    break;
  }
  // Copy the qualifiers.
  return newOne;
}

C_Type*
C_Type::copy()
{
  // Use a stack to keep track of which user types that has been
  // copied and where their copies are.
  Plist<C_Type> origs;
  Plist<C_Type> copies;
  return deep_copy(this, origs, copies);
}


bool
C_Type::isBase() const
{
  return tag == Primitive || tag == EnumType ;
}

bool
TypeCompareCallback::deem_different(C_Type const&,C_Type const&) const
{
  return false ;
}

bool
C_Type::equal(C_Type const& other, TypeCompareCallback const &cb ) const
{
  if (tag != other.tag)
    return false;
  if ( cb.deem_different(*this,other) )
    return false ;
  switch(tag) {
  case Primitive:
  case Abstract:
    return hash() == other.hash() ;
  case EnumType:
    return enum_def() == other.enum_def() ;
  case StructUnion:
    return user_def() == other.user_def() ;
  case Pointer:
    if ( cv.cst != other.cv.cst || cv.vol != other.cv.vol )
      return false ;
    // else fall through
  case FunPtr:
    return ptr_next()->equal(*other.ptr_next(),cb) ;
  case Array:
    if ( cv.cst != other.cv.cst || cv.vol != other.cv.vol )
      return false ;
    // XXX: What about size?
    return array_next()->equal(*other.array_next(),cb) ;
  case Function:
    double_foreach(a1, a2, fun_params(), other.fun_params(), Plist<C_Type>) {
      if ( ! (*a1)->equal( *(*a2), cb ) ) return false;
    }
    return fun_ret()->equal(*other.fun_ret(),cb) ;
    break;
  }
  assert(0);
  return false;
}

////////////////////////////// Initializers //////////////////////


C_Init::C_Init(const char* n) : tag(StringInit), stringinit(n)
{
  assert(n);
  Owner = NULL ;
}

C_Init::C_Init(C_Expr* e) : tag(Simple), simple(e)
{
  assert(e);
  Owner = NULL ;
}

C_Init::C_Init(C_InitTag t, Plist<C_Init>* lst) : tag(t), braced(lst)
{
  assert((tag==SloppyBraced || tag==FullyBraced) && lst);
  Owner = NULL ;
}

const char*
C_Init::string_init() const
{
  assert(tag==StringInit);
  return stringinit;
}

C_Expr*
C_Init::simple_init() const
{
  assert(tag==Simple && simple);
  return simple;
}

Plist<C_Init>&
C_Init::braced_init() const
{
  assert(tag==SloppyBraced || tag==FullyBraced && braced);
  return *braced;
}

void
C_Init::owner(C_Decl* o)
{
  Owner = o ;
  switch(tag) {
  case Simple:
    break ;
  case StringInit:
    break ;
  case SloppyBraced: {
    foreach(i,*braced,Plist<C_Init>)
      i->owner(o);
    break ; }
  case FullyBraced:
    switch(o->type->tag) {
    default:
      assert(0);
    case Array: {
      foreach(i,*braced,Plist<C_Init>)
        i->owner(o->members().front());
      break ; }
    case StructUnion: {
      Plist<C_Decl>::iterator di = o->members().begin() ;
      foreach(i,*braced,Plist<C_Init>)
        i->owner(*di++);
      break ; }
    }
    break ;
  }
}

C_Decl*
C_Init::owner() const
{
  assert(Owner);
  return Owner ;
}

//////////////////////////// Declarations /////////////////////////

// Create a list of subsidiary sub-declarations based on the type: Structures
// and arrays give rise to such extra declarations.
static void 
make_sub_declarations(C_Type* t, C_Decl* containing_decl, Plist<C_Decl>& subsidiaries)
{
  switch (t->tag) {
  case Array: {
    C_Type* at = t->array_next();
    C_Decl* sub = new C_Decl(VarDcl,at,NULL);
    sub->containedIn(containing_decl);
    subsidiaries.push_back(sub);
    break; }
  case StructUnion: {
    // Every member has its own declaration.
    foreach(m, t->user_types(), Plist<C_Type>) {
      C_Decl* sub = new C_Decl(VarDcl,*m,NULL);
      sub->containedIn(containing_decl);
      subsidiaries.push_back(sub);
    }
    break; }
  default:
    break;
  }
}

// Declaration. The type must be unique.
C_Decl::C_Decl(C_DeclTag tg,C_Type* typ,char const* n)
  : Numbered(Numbered::C_DECL), name(n), type(typ)
{
  tag = tg ;
  assert(typ || tg==ExtState);
  switch(tg) {
  case FunDf: {
    assert(type->tag == Function);
    break ; }
  case VarDcl:
    assert(type->tag != Function);
    var.isIn = NULL;
    var.init = NULL;
    var.initWhere = NULL;
    var.varmode = VarIntAuto ;
    var.varmodeWhy.set(Position());
    make_sub_declarations(type, this, membs);
    break ;
  case ExtFun:
    assert(type->tag == FunPtr);
    ext.effects = NULL ;
    ext.calltime = NULL ;
    break ;
  case ExtState:
    assert(type==NULL || type->tag != Function);
    break ;
  }
}

//////// Lookup //////////////

bool
C_Decl::hasInit() const
{
  assert(tag==VarDcl);
  return (var.init) ? true : false;
}

C_Init*
C_Decl::init() const
{
  assert(tag==VarDcl && var.init);
  return var.init;
}

C_BasicBlock*
C_Decl::init_where() const
{
  assert(tag==VarDcl && var.init);
  return var.initWhere ;
}

Plist<C_Decl>&
C_Decl::members()
{
  return membs;
}

C_Decl*
C_Decl::getMember(unsigned m)
{
  assert((tag==VarDcl || tag==ExtState) && m>0);
  return membs[m-1];
}

VariableMode
C_Decl::varmode() const
{
  assert(tag==VarDcl);
  return var.varmode ;
}

Position
C_Decl::varmodeWhy() const
{
  assert(tag==VarDcl);
  return var.varmodeWhy ;
}

CallAnnoAtom*
C_Decl::calltime() const
{
  assert(tag==ExtFun);
  return ext.calltime ;
}

StateAnnoAtom*
C_Decl::effects() const
{
  assert(tag==ExtFun);
  return ext.effects ;
}

Plist<C_Decl>&
C_Decl::fun_params() const
{
  assert(tag==FunDf && fun.params);
  return *fun.params;
}

Plist<C_Decl>&
C_Decl::fun_locals() const
{
  assert(tag==FunDf && fun.localdecls);
  return *fun.localdecls;
}

Plist<C_BasicBlock>&
C_Decl::blocks() const
{
  assert(tag==FunDf && fun.body);
  return *fun.body;
}

bool
C_Decl::isContained() const
{
  return tag == VarDcl && var.isIn ;
}

C_Decl*
C_Decl::containedIn() const
{
  assert(tag==VarDcl && var.isIn);
  return var.isIn;
}

C_Type*
C_Decl::fun_ret() const
{
  assert(tag==FunDf);
  return type->fun_ret();
}

//////// Update //////////////

void
C_Decl::varmode(VariableMode varmode,Position why)
{
  assert(tag==VarDcl);
  var.varmode = varmode ;
  var.varmodeWhy.set(why) ;
}

void
C_Decl::calltime(CallAnnoAtom *a)
{
  assert(tag==ExtFun);
  ext.calltime = a ;
}

void
C_Decl::effects(StateAnnoAtom *a)
{
  assert(tag==ExtFun);
  ext.effects = a ;
}

void
C_Decl::blocks(Plist<C_BasicBlock>* bbs)
{
  assert(tag==FunDf);
  assert(bbs != NULL && !bbs->empty());
  fun.body = bbs;
}

void
C_Decl::fun_params(Plist<C_Decl>* ds)
{
  assert(tag==FunDf);
  assert(ds!=NULL);
  fun.params = ds;
  // Run through all the parameter declarations and set their 'isIn' pointer to
  // this function.
  foreach( d, *fun.params, Plist<C_Decl> ) {
    assert( d->tag == VarDcl && d->var.isIn == NULL);
    d->var.isIn = this;
  }
}

void
C_Decl::fun_locals(Plist<C_Decl>* ds)
{
  assert(tag==FunDf);
  assert(ds!=NULL);
  fun.localdecls = ds;
  // Run through all declarations and set their 'isIn' pointer to this
  // function.
  foreach( d, *fun.localdecls, Plist<C_Decl> ) {
    assert( d->tag == VarDcl && d->var.isIn == NULL);
    d->var.isIn = this;
  }
}

void
C_Decl::containedIn(C_Decl* d)
{
  assert(tag==VarDcl);
  var.isIn = d;
}

void
C_Decl::init(C_Init* i)
{
  assert(tag==VarDcl);
  var.init = i;
  if ( i != NULL )
    i->owner(this);
}

void
C_Decl::init_where(C_BasicBlock* bb)
{
  assert(tag==VarDcl && var.init);
  var.initWhere = bb ;
}

void
C_Decl::subsidiary(C_Decl *d)
{
  assert(tag==ExtState);
  membs.push_back(d);
}

/////////

// Construct a name from a type.
static char*
Hungarian(C_Type* t,unsigned need)
{
  C_Type *tt = t ;
  // Calculate the needed length.
  while ( tt->tag == Pointer || tt->tag == Array )
    need++,
      tt = ( tt->tag == Pointer ? tt->ptr_next() : tt->array_next() );
  if ( tt->tag == Abstract )
    need += strlen(tt->primitive());
  else
    need += 2 ;
  char *result = new char[need];
  // Construct the name.
  char *pc = result ;
  while (1)
    switch(t->tag) {
    case Primitive: {
      const char* what = t->primitive();
      if (what==str_char)   { strcpy(pc,"c"); return result ; }
      if (what==str_uchar)  { strcpy(pc,"b"); return result ; }
      if (what==str_short)  { strcpy(pc,"s"); return result ; }
      if (what==str_ushort) { strcpy(pc,"us"); return result ; }
      if (what==str_int)    { strcpy(pc,"i"); return result ; }
      if (what==str_uint)   { strcpy(pc,"u"); return result ; }
      if (what==str_long)   { strcpy(pc,"l"); return result ; }
      if (what==str_ulong)  { strcpy(pc,"ul"); return result ; }
      if (what==str_float)  { strcpy(pc,"f"); return result ; }
      if (what==str_double) { strcpy(pc,"d"); return result ; }
      if (what==str_ldouble){ strcpy(pc,"ld"); return result ; }
      *pc = 0; return result ;
    }
    case StructUnion:
      strncpy(pc,t->user_def()->get_name(),2);
      pc[2] = 0 ; return result ;
    case EnumType:
      strncpy(pc,t->enum_def()->get_name(),2);
      pc[2] = 0 ; return result ;
    case FunPtr:
      strcpy(pc,"fn"); return result ;
    case Pointer:
      *pc++ = 'p' ;
      t = t->ptr_next() ;
      break;
    case Array:
      *pc++ = 'a' ;
      t = t->array_next() ;
      break;
    case Function:
      strcpy(pc,"fn"); return result ;
    case Abstract:
      if ( t->isVoid() )
        strcpy(pc,"v");
      else
        strcpy(pc,t->primitive());
      return result ;
    }
}

bool
C_Decl::hasName()
{
  return name != NULL && name[0] != '$' ;
}

// Construct a name if necessary.
const char*
C_Decl::get_name()
{
  // Temporary variables have no name -> make a name.
  if (name == NULL || name[0] == '$' ) {
    const char * suffix = name ? name+1 : "Tmp" ;
    if ( hasInit() && init()->tag==StringInit ) {
      // String constant: use prefix of the string as name.
      char buffer[25] ;
      char const *pc = init()->string_init() ;
      int i ;
      bool allow_ = false ;
      for ( i = 0 ; i < 24 && *pc ; pc++ )
        if ( ( *pc == ' ' || *pc == '_') && allow_ )
          buffer[i++] = '_', allow_ = false ;
        else if ( *pc >= 'a' && *pc <= 'z' ||
                  *pc >= 'A' && *pc <= 'Z' ||
                  *pc >= '0' && *pc <= '9' ) {
          if ( *pc >= '0' && *pc <='9' && i == 0 )
            buffer[i++] = 's' ;
          buffer[i++] = *pc ;
          allow_ =true ;
        }
      if ( i ) {
        buffer[i] = '\0' ;
        return stringDup(buffer);
      } else
        return "str" ;
    } else {
      char *hung = Hungarian(type,strlen(suffix)+1) ;
      strcat(hung,suffix);
      return hung ;
    }
  }
  return name;
}

//////////////////////////// UserTypes /////////////////////////

C_UserMemb::C_UserMemb(const char* n)
  : Numbered(Numbered::C_USERMEMB), name(n), expr(NULL)
{}

const char*
C_UserMemb::get_name()
{
  return name;
}

bool
C_UserMemb::hasValue() const
{
  return expr != NULL ;
}

C_Expr*
C_UserMemb::value() const
{
  assert(expr);
  return expr;
}

void
C_UserMemb::value(C_Expr* e)
{
  expr = e;
}

C_EnumDef::C_EnumDef(unsigned nr, const char* n, Position p)
  : Numbered(Numbered::C_ENUMDEF), name(n), pos(p), defseqnum(nr)
{}

const char*
C_EnumDef::get_name()
{
  // Anonymous structures variables have no name. Invent one.
  if (name == NULL || name[0] == '$' )
    name = "Anonymous" ;
  return name;  
}

Plist<C_UserMemb>&
C_EnumDef::members()
{
  return membs;
}


C_UserDef::C_UserDef(unsigned seqnum, const char* n, Position p, bool isUnio)
  : Numbered(Numbered::C_USERDEF), name(n), isUnion(isUnio),
    defseqnum(seqnum), pos(p)
{}

C_UserDef*
C_UserDef::copy()
{
  C_UserDef *cp = new C_UserDef(defseqnum,name,pos,isUnion) ;
  foreach(i,names,Plist<C_UserMemb>)
    cp->names.push_back(*i);
  return cp ;
}

const char*
C_UserDef::get_name()
{
  // Anonymous structures variables have no name. Invent one.
  if (name == NULL || name[0] == '$' )
    name = "Anonymous" ;
  return name;
}

void
C_UserDef::new_instance(C_Type* t)
{
  assert(t);
  instances.push_back(t);
}

C_UserDef::iterator::iterator(C_UserDef *u)
  : t(u->instances.front()->user_types().begin()),
    m(u->names.begin())
{
}

C_Type*
C_UserDef::iterator::type()
{
  return *t ;
}

C_UserMemb*
C_UserDef::iterator::name()
{
  return *m ;
}

void
C_UserDef::iterator::operator ++()
{
  t++ ;
  m++ ;
}

C_UserDef::iterator::operator bool()
{
  return t ;
}

//////////////////////////// Control /////////////////////////

// Return.
C_Jump::C_Jump(C_Expr *e, Position p)
  : Numbered(Numbered::C_JUMP), exp(e), tag(C_Return), pos(p)
{ }

// If-then-else.
C_Jump::C_Jump(C_Expr *e, C_BasicBlock *thn, C_BasicBlock *els, Position p)
  : Numbered(Numbered::C_JUMP), tag(C_If), pos(p)
{
  cond.expr = e;
  cond_then(thn);
  cond_else(els);
}

// Goto.
C_Jump::C_Jump(C_BasicBlock *ta)
  : Numbered(Numbered::C_JUMP), tag(C_Goto)
{
  goto_target(ta);
}

bool
C_Jump::hasExpr()
{
  assert(tag == C_Return);
  return exp ;
}

C_Expr*
C_Jump::return_expr() const
{
  assert(tag == C_Return);
  return exp;
}


C_Expr*
C_Jump::cond_expr() const
{
  assert(tag == C_If);
  return cond.expr;
}


C_BasicBlock*
C_Jump::cond_then() const
{
  assert(tag == C_If);
  return cond.thn;
}


C_BasicBlock*
C_Jump::cond_else() const
{
  assert(tag == C_If);
  return cond.els;
}


C_BasicBlock*
C_Jump::goto_target() const
{
  assert(tag == C_Goto);
  return target;
}

void
C_Jump::cond_then(C_BasicBlock *b)
{
  assert(tag == C_If);
  assert(b!=NULL);
  cond.thn = b;
}


void
C_Jump::cond_else(C_BasicBlock *b)
{
  assert(tag == C_If);
  assert(b!=NULL);
  cond.els = b;
}


void
C_Jump::goto_target(C_BasicBlock *b)
{
  assert(tag == C_Goto);
  assert(b!=NULL);
  target = b;
}



//////////////////////////// Statements /////////////////////////

// The basic constructor makes a sequence point.
C_Stmt::C_Stmt()
  : Numbered(Numbered::C_STMT), tag(C_Sequence)
{}

// Free / Assignment.
C_Stmt::C_Stmt(C_StmtTag t, C_Expr *e, Position p)
  : Numbered(Numbered::C_STMT), tag(t), lefthand(NULL), pos(p)
{
  if (tag == C_Free)
    fre.e = e;
  else if (tag == C_Assign)
    assign.source = e;
  else
    assert(0);
}
 
// Call
C_Stmt::C_Stmt(C_Expr *f, Plist<C_Expr> *args, Position p)
  : Numbered(Numbered::C_STMT), tag(C_Call), lefthand(NULL), pos(p)
{
  call.fun = f; call.args = args;
}

// Alloc
C_Stmt::C_Stmt(C_Decl* d, Position p)
  : Numbered(Numbered::C_STMT), tag(C_Alloc), lefthand(NULL), pos(p)
{
  alloc.obj = d;
  alloc.isMalloc = true ;
}

// copy

C_Stmt*
C_Stmt::copy() {
  assert(tag == C_Assign);
  // the only statements that currently need to be copied are
  // assignments resulting from post-increment
  C_Stmt *s = new C_Stmt(C_Assign,assign.source->copy(),pos);
  if ( lefthand )
    s->target(lefthand->copy());
  return s;
}
    
// Lookup

C_Expr*
C_Stmt::free_expr() const
{
  assert(tag == C_Free);
  return fre.e;
}


C_Expr*
C_Stmt::assign_expr() const
{
  assert(tag == C_Assign);
  return assign.source;
}

bool
C_Stmt::hasTarget() const
{
  return lefthand ;
}

C_Expr*
C_Stmt::target() const
{
  assert(lefthand);
  return lefthand;
}


C_Expr*
C_Stmt::call_expr() const
{
  assert(tag == C_Call);
  return call.fun;
}


Plist<C_Expr>&
C_Stmt::call_args() const
{
  assert(tag == C_Call);
  return *call.args;
}

bool
C_Stmt::isMalloc() const
{
  assert(tag == C_Alloc);
  return alloc.isMalloc;
}

C_Decl*
C_Stmt::alloc_objects() const
{
  assert(tag == C_Alloc);
  return alloc.obj;
}

// ------------------------------------------------------------------------
// Update functions


/*void
C_Stmt::call_mode(CallDescription* f)
{
  assert( tag == C_Call );
  call.callmode = f ;
}*/

void
C_Stmt::isMalloc(bool m)
{
  assert( tag == C_Alloc );
  assert( m || alloc.obj->type->hasSize() );
  alloc.isMalloc = m ;
}

void
C_Stmt::target(C_Expr *e)
{
  assert( tag != C_Free );
  assert( lefthand == NULL );
  assert( e != NULL );
  lefthand = e ;
}

//////////////////////////// Basic Blocks /////////////////////////

C_BasicBlock::C_BasicBlock()
  : Numbered(Numbered::C_BASICBLOCK), last(NULL)
{}


Plist<C_Stmt>&
C_BasicBlock::getBlock()
{
  return stmts;
}

bool
C_BasicBlock::hasExit() const
{
  return last ;
}
    
C_Jump*
C_BasicBlock::exit() const
{
  assert (last);
  return last;
}

void
C_BasicBlock::exit(C_Jump *j)
{
  last = j ;
}

//////////////////////////// Expressions /////////////////////////

// Constant
C_Expr::C_Expr(C_Type* t, const char* c, Position p)
  : Numbered(C_EXPR), tag(C_Cnst), type(t), cst(c), pos(p)
{ }

// Enum constant
C_Expr::C_Expr(C_UserMemb* d, Position p)
  : Numbered(C_EXPR), tag(C_EnumCnst), type(new C_Type(Int)), pos(p) 
{
  assert(d);
  enumcnst = d;
}

// Null pointer constant/Type size.
C_Expr::C_Expr(C_ExprTag tg, C_Type *t, Position p)
  : Numbered(C_EXPR), tag(tg), pos(p)
{
  if (tag==C_Null) {
    type = t;
  }
  else {
    type = new C_Type(UInt);
    stype = t;
    assert(tag==C_SizeofT);
  }
}

// Var.
C_Expr::C_Expr(C_Decl *v,  Position p)
  : Numbered(C_EXPR), tag(tag),
    vr(v), pos(p)
{
  switch(v->tag) {
  case VarDcl:
    tag = C_Var ;
    type = new C_Type(Pointer,v->type) ;
    break ;
  case FunDf:
    tag = C_FunAdr ;
    assert(v->type->tag == Function);
    type = new C_Type(FunPtr,v->type) ;
    break ;
  case ExtFun:
    tag = C_ExtFun ;
    assert(v->type->tag == FunPtr);
    type = v->type;
    break ;
  case ExtState:
    assert(v->tag!=ExtState);
  }
}

// Struct.
C_Expr::C_Expr(C_Expr *e, unsigned n, Position p)
  : Numbered(C_EXPR), tag(C_Member), pos(p)
{
  subexpr1 = e;
  assert(n>=1);
  member_nr = n;
  // Find the type of the member from the type of e.
  type = e->type->ptr_next();
  // Get the n'th member.
  Plist<C_Type>::iterator m = type->user_types().begin();
  while (--n != 0) m++;
  type = new C_Type(Pointer,*m);
}

// Unary.
C_Expr::C_Expr(UnOp o, C_Expr *e, Position p)
  : Numbered(C_EXPR), tag(C_Unary), pos(p)
{
  switch (o) {
  case Addr:
  case DeRef:
  case Pos:
    // This cannot happen since these constructs are not present in CoreC.
    assert(0);
  case Neg:
  case Not:
    // Negation of unsigned produces unsigned.
    type = e->type->copy();
    break;
  case Bang:
    type = new C_Type(Int);
    break;
  }
  unary = o;
  subexpr1 = e;
}

// Binary/PtrCmp/PtrArith.
C_Expr::C_Expr(BinOp o, C_Expr *e1, C_Expr *e2, Position p)
  : Numbered(C_EXPR), pos(p)
{
  if ( e1->type->tag == Pointer ) {
    if ( e2->type->tag == Pointer ) {
      // Pointer comparison.
      tag = C_PtrCmp ;
      assert( o==Sub || o==LT || o==GT || o==LEq || o==GEq || o==Eq || o==NEq );
      type = new C_Type(Int);
    }
    else {
      // Pointer arithemetic.
      assert ( o==Add || o==Sub );
      tag = C_PtrArith;
      type = new C_Type(Pointer,e1->type->ptr_next());
    }
  } else if ( e1->type->tag == FunPtr ) {
    tag = C_PtrCmp ;
    assert( e2->type->tag == FunPtr );
    assert( o==Eq || o==NEq );
    type = new C_Type(Int);
  } else {
    assert( e2->type->tag != Pointer );
    tag = C_Binary ;
    type = e1->type->copy();
  }
  binary.op = o;
  subexpr1 = e1;
  binary.subexpr2 = e2;
}

// Dereference, sizeof expr, Array decay.
C_Expr::C_Expr(C_ExprTag tg, C_Expr* e, Position p)
  : Numbered(C_EXPR), tag(tg), pos(p)
{
  switch(tag) {
  case C_DeRef:
    type = e->type->ptr_next();
    break ;
  case C_Array:
    type = new C_Type(Pointer,e->type->ptr_next()->array_next());
    break ;
  case C_SizeofE:
    type = new C_Type(UInt);
    break ;
  default:
    assert(0);
  }
  subexpr1 = e;
}
  
// Cast.
C_Expr::C_Expr(C_Type *t, C_Expr *e, Position p)
  : Numbered(C_EXPR), tag(C_Cast), type(t), pos(p)
{
  subexpr1 = e;
}

const char*
C_Expr::cnst() const
{
  assert(tag == C_Cnst);
  return cst;
}

C_UserMemb*
C_Expr::enum_cnst() const
{
  assert(tag == C_EnumCnst);
  return enumcnst ;
}        

C_Decl*
C_Expr::var() const
{
  assert(tag==C_Var || tag==C_FunAdr || tag==C_ExtFun);
  return vr;
}

unsigned
C_Expr::struct_nr() const
{
  assert(tag == C_Member);
  return member_nr;
}

C_UserMemb*
C_Expr::struct_name() const
{
  assert(tag==C_Member);
  C_Type* stype = subexpr1->type->ptr_next();
  return stype->user_def()->names[struct_nr()-1];
}

UnOp
C_Expr::unary_op() const
{
  assert(tag == C_Unary);
  return unary;
}


C_Expr*
C_Expr::subexpr() const
{
  switch(tag) {
  default:
    assert(0);
  case C_SizeofE:
  case C_Unary:
  case C_Member:
  case C_Array:
  case C_DeRef:
  case C_Cast:
    return subexpr1;
  }
}

BinOp
C_Expr::binary_op() const
{
  assert(tag == C_Binary || tag == C_PtrArith || tag == C_PtrCmp );
  return binary.op;
}

void
C_Expr::binary_op(BinOp op)
{
  assert(tag == C_Binary || tag == C_PtrArith || tag == C_PtrCmp );
  binary.op = op;
}


C_Expr*
C_Expr::binary_expr1() const
{
  assert(tag == C_Binary || tag == C_PtrArith || tag == C_PtrCmp );
  return subexpr1;
}

void
C_Expr::binary_expr1(C_Expr *e)
{
  assert(tag == C_Binary || tag == C_PtrArith || tag == C_PtrCmp );
  subexpr1 = e;
}


C_Expr*
C_Expr::binary_expr2() const
{
  assert(tag == C_Binary || tag == C_PtrArith || tag == C_PtrCmp );
  return binary.subexpr2;
}

void
C_Expr::binary_expr2(C_Expr *e)
{
  assert(tag == C_Binary || tag == C_PtrArith || tag == C_PtrCmp );
  binary.subexpr2 = e;
}

static bool
primitive_type(C_TypeTag t)
{
  return t == Primitive || t == EnumType ;
}

bool
C_Expr::isBenign() const
{
  assert(tag==C_Cast);
  if (type->tag == Pointer && subexpr()->type->tag == Pointer)
    return type->ptr_next()->equal(*subexpr()->type->ptr_next());
  return primitive_type(type->tag) && primitive_type(subexpr()->type->tag)
    || type->equal(*subexpr()->type);
}

C_Type*
C_Expr::sizeof_type() const
{
  switch(tag) {
  case C_SizeofT:
    return stype;
  case C_SizeofE:
    return subexpr1->type ;
  default:
    assert(0);
    return NULL ;
  }
}

C_Expr*
C_Expr::copy()
{
  C_Expr* newE = NULL;
  switch (tag) {
  case C_Cnst:
    newE = new C_Expr(type->copy(), cnst(), pos);
    break;
  case C_EnumCnst:
    newE = new C_Expr(type->copy(), cnst(), pos);
    break;
  case C_Null:
    newE = new C_Expr(tag, type->copy(), pos);
    break;
  case C_Var:
  case C_FunAdr:
  case C_ExtFun:
    newE = new C_Expr(var(), pos);
    break;
  case C_Member:
    newE = new C_Expr(subexpr()->copy(), struct_nr(), pos);
    break;
  case C_Array:
  case C_DeRef:
  case C_SizeofE:
    newE = new C_Expr(tag, subexpr()->copy(), pos);
    break;
  case C_Unary:
    newE = new C_Expr(unary_op(), subexpr()->copy(), pos);
    break;
  case C_PtrArith:
  case C_PtrCmp:
  case C_Binary:
    newE = new C_Expr(binary_op(), binary_expr1()->copy(),
                      binary_expr2()->copy(), pos);
    break;
  case C_SizeofT:
    newE = new C_Expr(C_SizeofT, sizeof_type()->copy(), pos);
    break;
  case C_Cast:
    newE = new C_Expr(type->copy(), subexpr()->copy(), pos);
    break;
  }
  return newE;
}

////////////////// Program ////////////////////////////

C_Pgm::C_Pgm()
{
}





