/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Representation of a C program.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "Plist.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cpgm.h"
#include "auxilary.h"
#include "options.h"

#define debug debugstream_cpgm

//////////////////////

#define onebasetype(x) case x: { static BaseType can(x); return &can; }
BaseType*
BaseType::get(BaseTypeTag t) {
  switch(t) {
    onebasetype(AbsT);
    onebasetype(UAbsT);
    onebasetype(FloatingAbsT);
    onebasetype(Void);
    onebasetype(Char);
    onebasetype(SChar);
    onebasetype(UChar);
    onebasetype(ShortInt);
    onebasetype(UShortInt);
    onebasetype(Int);
    onebasetype(UInt);
    onebasetype(LongInt);
    onebasetype(ULongInt);
    onebasetype(Float);
    onebasetype(Double);
    onebasetype(LongDouble);
  }
  assert(0);
  return NULL ;
}
#undef onebasetype

//////////////////////

ConstExpr*
makeConstExpr(long i, const Position p)
{
    ConstExpr* e = new ConstExpr(long2a(i),Long,p);
    e->valueKnown = true ;
    e->knownValue = i ;
    return e;
}

//

Type::Type(TypeTag t, bool fun, bool ell)
  : checked(0), realtype(t), cv(false,false),
    isFun(fun), isVoid(false)
{}

Type::Type(Type& t)
  : checked(0), realtype(t.realtype), cv(t.cv),
    isFun(t.isFun), isVoid(t.isVoid)
{}

bool Type::operator==(Type &other) {
    return equals(&other);
}

bool Type::fully_equals(Type* other) {
    return cv.cst == other->cv.cst &&
           cv.vol == other->cv.vol &&
           equals(other) ;
}    

void
Type::show(ostream& os)
{
  if (cv.cst) os << "const ";
  if (cv.vol) os << "volatile ";
}

bool BaseType::equals(Type* other) {
    assert(other != NULL);
    if (other->realtype != realtype) return false ;
    BaseType* real = (BaseType*) other;
    return (tag == real->tag);
}

void
BaseType::show(ostream& os)
{
  Type::show(os);
  os << basetype2str(tag);
}

bool AbsType::equals(Type *other) {
    if ( other->realtype != AbstractT )
        return 0 ;
    const AbsType* real = (AbsType*)other ;
    return strcmp(name,real->name) == 0 ;
}

void AbsType::show(ostream &os) {
    Type::show(os);
    os << name ;
}

bool PtrType::equals(Type* other) {
    assert(other != NULL);
    if (other->realtype != realtype) return 0;
    PtrType* real = (PtrType*) other;
    return(next->fully_equals(real->next));
}

void
PtrType::show(ostream& os)
{
  Type::show(os);
  os << "pointer to ";
  next->show(os);
}

bool ArrayType::equals(Type* other) {
    assert(other != NULL);
    if (other->realtype != realtype) return 0;
    ArrayType* real = (ArrayType*) other;
    // The size of an array is not important. XXX
    return( *next == *real->next );
}

void
ArrayType::show(ostream& os)
{
  Type::show(os);
  os << "array of ";
  next->show(os);
}

bool
FunType::unspecified()
{
  return params->empty() && varargs ;
}

bool FunType::equals(Type* other) {
    assert(other != NULL);
    if (other->realtype != realtype) return 0;
    FunType* real = (FunType*) other;
    // Check the return type.
    if (!( *ret == *real->ret )) // return types are not qualified
        return false ;
    if (debug) debug << "[return OK]";
    // Check the argument types by running through the list.
    Plist<Type>::iterator it1 = params->begin();
    Plist<Type>::iterator it2 = real->params->begin();
    while (it1 && it2)
    {
        // the qualifiers on parameter types matter only inside the function
        if (!( **it1 == **it2 ))
            return false ;
        it1++, it2++;
        if (debug) debug << "[parameter OK]";
    }
    // Check equal length.
    if( it1 ) return real->varargs ;
    if( it2 ) return varargs ;
    return true;
}

void
FunType::show(ostream& os)
{
  os << "function(";
  bool firstArg = true;
  foreach( par, *params, Plist<Type> ) {
    if (!firstArg)
      os << ",";
    else
      firstArg = false;
    par->show(os);
  }
  Type::show(os);
  os << ") returning ";
  ret->show(os);
}

bool UserType::equals(Type* other) {
  assert( other != NULL );
  if (other->realtype != realtype) return 0;
  UserDecl *a = def ;
  UserDecl *b = ((UserType*)other)->def ;
  while( a->refer_to != NULL ) a = a->refer_to ;
  while( b->refer_to != NULL ) b = b->refer_to ;
  return a == b ;
}

void
UserType::show(ostream& os)
{
  Type::show(os);  
  os << usertype2str(def->tag) << " " << def->name;
}

void
StructType::show(ostream& os)
{
  UserType::show(os);  
}

void
UnionType::show(ostream& os)
{
  UserType::show(os);  
}

void
EnumType::show(ostream& os)
{
  UserType::show(os);  
}


///////////////////////////////


void
MemberDecl::show(ostream& os)
{
  var->show(os);
}

void
Init::show(ostream& os)
{
  if (tag==InitElem)
    exp->show(os);
  else {
    os << "{";
    foreach( i, *inits, Plist<Init> ) { i->show(os); os << ";"; }
    os << "}";
  }
}


//////////////////////////////////

void
StructDecl::show(ostream& os)
{
  os << usertype2str(tag) << " " << name << " {";
  foreach( mem, *member_list, Plist<MemberDecl> ) {
    (*mem)->show(os);
    os << ";";
  }
  os << " }";
}

void
EnumDecl::show(ostream& os)
{
  os << usertype2str(tag) << " " << name << " {";
  foreach( mem, *member_list, Plist<VarDecl> ) {
    mem->show(os);
    os << ",";
  }
  os << " }";
}



/////////////////////////////////////

//////////////////////////////////////

void
VarDecl::set_initializer(Init* i)
{
    // Maybe we should give a warning (error?) here:
    assert(init==NULL);
    init = i;
}

bool
VarDecl::is_definition()
{
  return init != NULL || varmode == VarIntAuto ;
}

void
VarDecl::show(ostream& os)
{  
  os << name << ": ";
  type->show(os);
}

Init*
VarDecl::initializer()
{
    return init;
}

void
FunDef::set_initializer(Init* init)
{
    DONT_CALL_THIS
}

bool
FunDef::is_definition()
{
  return stmts != NULL ;
}

void
FunDef::show(ostream& os)
{  
  os << name << ": ";
  type->show(os);
}

Init*
FunDef::initializer()
{
  return NULL;
}

/////////////// Evaluate constant expression //////////////////////

Expr*
Expr::copy_plusone()
{
  return new BinaryExpr(this,new ConstExpr("1",Integer,pos),Add,pos);
}

Expr*
BinaryExpr::copy_plusone()
{
  if ( op == Add )
    return new BinaryExpr(left,right->copy_plusone(),Add,pos);
  else
    return Expr::copy_plusone() ;
}

Expr*
ConstExpr::copy_plusone()
{
  if ( valueKnown ) {
    char buffer[10] ;
    sprintf(buffer,"%lu",1+(unsigned long)knownValue);
    return new ConstExpr(stringDup(buffer),tag,pos);
  }
  return Expr::copy_plusone() ;
}

ConstExpr::ConstExpr(char* v,ConstantTag t,const Position p)
    : Expr(p,1), literal(v), tag(t)
{
    switch (t)
    {
    case  Integer:
        knownValue = strtol(literal,NULL,0);
        valueKnown = knownValue <= 0x7FFF ;
        exprType = BaseType::get(Int);
        break;
    case  UInteger:
        knownValue = strtol(literal,NULL,0);
        valueKnown = knownValue <= 0xFFFFl ;
        exprType = BaseType::get(UInt);
        break;
    case  Long:
        knownValue = strtol(literal,NULL,0);
        valueKnown = knownValue <= 0x7FFFFFFFl ;
        exprType = BaseType::get(LongInt);
        break;
    case  ULong:
        knownValue = strtoul(literal,NULL,0);
        valueKnown = knownValue >= 0 && knownValue <= 0x7FFFFFFFl ;
        exprType = BaseType::get(ULongInt);
        break;
    case  Floating:
        exprType = BaseType::get(Float);
        break;
    case  DoubleF:
        exprType = BaseType::get(Double);
        break;
    case  Character:
        exprType = BaseType::get(Char);
        break;
    case  StrConst:
        // A string literal simply has an incomplete array type
        // - it will decay to a perfectly fine char pointer in the check
        // phase, so nobody will look at the missing dimension.
        exprType = new ArrayType(NULL,BaseType::get(Char));
        break;
    }
}

NullExpr::NullExpr(Position p) : Expr(p,1) {}

void
ConstExpr::show(ostream& os)
{  
  os << literal;
}

void
NullExpr::show(ostream& os)
{
    os << '(' ;
    if ( exprType )
        exprType->show(os);
    else
        os << '*' ;
    os << ")0" ;
}

UnaryExpr::UnaryExpr(Expr* e, UnOp o, Position p)
  : Expr(p), exp(e), op(o)
{
  if ( e->valueKnown ) {
    switch(o) {
    case Addr:
    case DeRef:
    case Not:
      return ;
    case Pos:
      knownValue = e->knownValue ;
      break;
    case Neg:
      knownValue = -e->knownValue ;
      break;
    case Bang:
      knownValue = !e->knownValue ;
      break;
    }
    valueKnown = true ;
  }
}

void
UnaryExpr::show(ostream& os)
{  
  os << unary2str(op) << "(";
  exp->show(os);
  os << ")";
}

void
TypeSize::show(ostream& os)
{  
  os << "sizeof(";
  typesz->show(os);
  os << ")";
}

void
ExprSize::show(ostream& os)
{  
  os << "sizeof(";
  exp->show(os);
  os << ")";
}

void
CastExpr::show(ostream& os)
{  
  os << "(";
  exprType->show(os);
  os << ")";
  exp->show(os);
}

BinaryExpr::BinaryExpr (Expr* lt, Expr* rt, BinOp o, Position p)
  : Expr(p), left(lt), right(rt), op(o)
{
  if ( lt->valueKnown && rt->valueKnown ) {
    long a = lt->knownValue;
    long b = rt->knownValue;
    switch (op) {
    case Mul:
      knownValue = a * b;
      break;
    case Div:
      if ( b <= 0 || a < 0 ) return ;
      knownValue = a / b; break;
    case Mod:
      if ( b <= 0 || a < 0 ) return ;
      knownValue = a % b; break;
    case Add: knownValue = a + b; break;
    case Sub: knownValue = a - b; break;
    case LShift:
      if ( a < 0 || b < 0 ) return ;
      knownValue = a << b; break;
    case RShift:
      if ( b < 0 || a > 0x7FFFFFFFl << b ) return ;
      knownValue = a >> b; break;
    case LT:
      if ( b < 0 || a < 0 ) return ;
      knownValue = a < b; break;
    case GT:
      if ( b < 0 || a < 0 ) return ;
      knownValue = a > b; break;
    case LEq:
      if ( b < 0 || a < 0 ) return ;
      knownValue = a <= b; break;
    case GEq:
      if ( b < 0 || a < 0 ) return ;
      knownValue = a >= b; break;
    case Eq:  knownValue = a == b; break;
    case NEq: knownValue = a != b; break;
    case BAnd:
      if ( b < 0 || a < 0 ) return ;
      knownValue = a & b; break;
    case BEOr:
      if ( b < 0 || a < 0 ) return ;
      knownValue = a ^ b; break;
    case BOr:
      if ( b < 0 || a < 0 ) return ;
      knownValue = a | b; break;
    case And:  knownValue = a && b; break;
    case Or:   knownValue = a || b; break;
    }
    valueKnown = true ;
  }
}

void
BinaryExpr::show(ostream& os)
{  
  os << binary2str(op) << "(";
  left->show(os);
  os << ",";
  right->show(os);
  os << ")";
}

CondExpr::CondExpr(Expr* c,Expr* lt, Expr* rt,Position p)
  : Expr(p), cond(c), left(lt), right(rt)
{
  if ( c->valueKnown ) {
    if ( c->knownValue ) {
      valueKnown = lt->valueKnown ;
      knownValue = lt->knownValue ;
    } else {
      valueKnown = rt->valueKnown ;
      knownValue = rt->knownValue ;
    }
  }
}

void
CondExpr::show(ostream& os)
{  
  cond->show(os);
  os << "?";
  left->show(os);
  os << ":";
  right->show(os);
}


VarExpr::VarExpr(ObjectDecl* d,Position p)
  : Expr(p), decl(d), annos(NULL)
{
  assert(d!=NULL);
  Init* init = decl->initializer();
  if (init != NULL && init->isSimple() ) {
    valueKnown = init->expr()->valueKnown ;
    knownValue = init->expr()->knownValue ;
  }
}

void
VarExpr::show(ostream& os)
{  
  os << decl->name;
}

void
CallExpr::show(ostream& os)
{  
  fun->show(os);
  os << "(";
  bool firstArg = true;
  foreach( arg, *args, Plist<Expr> ) {
    if (!firstArg) 
      os << "," ;
    else
      firstArg = false;
    arg->show(os);
  }
}

void
ArrayExpr::show(ostream& os)
{  
  left->show(os);
  os << "[";
  right->show(os);
  os << "]";
}

void
DotExpr::show(ostream& os)
{  
  left->show(os);
  os << ".";
  os << member;
}

void
PreExpr::show(ostream& os)
{
  if (op==Decr)
    os << "--";
  else
    os << "++";
  exp->show(os);
}

void
PostExpr::show(ostream& os)
{
  exp->show(os);
  if (op==Decr)
    os << "--";
  else
    os << "++";
}

CommaExpr::CommaExpr (Expr* lt, Expr* rt,Position p)
  : Expr(p), left(lt), right(rt)
{
  valueKnown = rt->valueKnown;
  knownValue = rt->knownValue;
}

void
CommaExpr::show(ostream& os)
{  
  left->show(os);
  os << ",";
  right->show(os);
}

void
AssignExpr::show(ostream& os)
{  
  left->show(os);
  os << " o= ";
  right->show(os);
}

///////// Deep Copy ///////////////

AbsType* AbsType::copy() {
    return new AbsType(*this) ;
}

BaseType*
BaseType::copy()
{
  BaseType *cp = new BaseType(tag) ;
  cp->cv = cv ;
  return cp ;
}

PtrType*
PtrType::copy()
{
    PtrType* t = new PtrType(*this);
    t->next = next->copy();
    return t;
}

ArrayType*
ArrayType::copy()
{
    ArrayType* t = new ArrayType(*this);
    t->next = t->next->copy();
    // What about the size? XXX
    return t;
}

FunType*
FunType::copy()
{
    FunType* t = new FunType(*this);
    // XXX
    return t;
}

StructType*
StructType::copy()
{
    return new StructType(*this);
}

UnionType*
UnionType::copy()
{
    return new UnionType(*this);
}

EnumType*
EnumType::copy()
{
    return new EnumType(*this);
}

//////////////////////////////////////////

bool AbsType::isIntegral()  { return strchr(properties,'i'); }
bool AbsType::isArithmic()  { return strchr(properties,'a') || isIntegral() ; }
bool AbsType::isAggregate() { return false; }
bool AbsType::isDerived()   { return strchr(properties,'p'); }

///////////////////////////////////////////

bool BaseType::isIntegral()
{
    switch (tag) {
    case AbsT:
    case UAbsT:
    case Char:
    case SChar:
    case UChar:
    case ShortInt:
    case UShortInt:
    case Int:
    case UInt:
    case LongInt:
    case ULongInt:   return true;
    case FloatingAbsT:
    case Float:
    case Double:
    case LongDouble:
    case Void:       return false;
    }
    return false;
}

bool BaseType::isArithmic()
{
    return tag != Void ;
}

bool BaseType::isAggregate()
{
    return false;
}

bool BaseType::isDerived()
{
    return false;
}

//////////////////////////////////

bool PtrType::isIntegral()  { return false; }
bool PtrType::isArithmic()  { return false; }
bool PtrType::isAggregate() { return false; }
bool PtrType::isDerived()   { return true; }

///////////////////////////////////

bool ArrayType::isIntegral()  { return false; }
bool ArrayType::isArithmic()  { return false; }
bool ArrayType::isAggregate() { return true; }
bool ArrayType::isDerived()   { return true; }

//////////////////////////////////////

bool FunType::isIntegral()  { return false; }
bool FunType::isArithmic()  { return false; }
bool FunType::isAggregate() { return false; }
bool FunType::isDerived()   { return true; }

///////////////////////////////////////

bool StructType::isIntegral()  { return false; }
bool StructType::isArithmic()  { return false; }
bool StructType::isAggregate() { return true; }
bool StructType::isDerived()   { return false; }

///////////////////////////////////////////////

bool UnionType::isIntegral()  { return false; }
bool UnionType::isArithmic()  { return false; }
bool UnionType::isAggregate() { return true; }
bool UnionType::isDerived()   { return false; }

//////////////////////////////////////

bool EnumType::isIntegral()  { return true; }
bool EnumType::isArithmic()  { return true; }
bool EnumType::isAggregate() { return false; }
bool EnumType::isDerived()   { return false; }

/////////////////////////////////

int ConstExpr::precedence(){ return 31; }
int NullExpr::precedence() { return 29; }
int VarExpr::precedence()  { return 31; }
int CallExpr::precedence() { return 29; }
int ArrayExpr::precedence(){ return 29; }
int DotExpr::precedence()  { return 29; }
int PostExpr::precedence() { return 29; }
int PreExpr::precedence()  { return 27; }
int TypeSize::precedence() { return 31; }
int ExprSize::precedence() { return 27; }
int CastExpr::precedence() { return silent ? exp->precedence() : 27; }
int UnaryExpr::precedence(){ return 27; }
int BinaryExpr::precedence() {
  switch (op) {
  case Mul:   
  case Div:   
  case Mod:    return 25;
  case Add:   
  case Sub:    return 23;
  case LShift:
  case RShift: return 21;
  case LT:   
  case GT:   
  case LEq:  
  case GEq:    return  19;
  case Eq:   
  case NEq:    return  17;
  case BAnd:   return  15;
  case BEOr:   return  13;
  case BOr:    return  11;
  case And:    return  9;
  case Or:     return  7;
  }
  return 0;
}
int CondExpr::precedence() { return 5; }
int AssignExpr::precedence() { return 3; }
int CommaExpr::precedence() { return 1; }

CompoundStmt *Stmt::this_as_compound() { return NULL; }

CompoundStmt *CompoundStmt::this_as_compound() { return this; }

void NullaryStmt::preorder(StmtCallbackClosure *cb) {
    cb->whatever(this);
}

void UnaryStmt::preorder(StmtCallbackClosure *cb) {
    cb->whatever(this);
    stmt->preorder(cb);
}

void CompoundStmt::preorder(StmtCallbackClosure *cb) {
    cb->whatever(this);
    foreach( i, *stmts, Plist<Stmt> )
        i->preorder(cb);
}

void IfStmt::preorder(StmtCallbackClosure *cb) {
    cb->whatever(this);
    thn->preorder(cb);
    els->preorder(cb);
}


//////////////////////////////////////////////////////

void
LabelStmt::show(ostream& os)
{
  os << label << ": ";
  stmt->show(os);
}

void
CaseStmt::show(ostream& os)
{
  os << "case ";
  exp->show(os);
  os << ": ";
  stmt->show(os);
}

void
DefaultStmt::show(ostream& os)
{
  os << "default: ";
  stmt->show(os);
}

void
CompoundStmt::show(ostream& os)
{
  os << "{ ";
  foreach( ob, *objects, Plist<VarDecl> ) { ob->show(os); os << "; "; }
  foreach( s, *stmts, Plist<Stmt> ) { s->show(os); os << "; "; }
  os << " }";
}

void
Indirection::show(ostream& os)
{
  os << "==>";
  stmt->show(os);
}

void
ExprStmt::show(ostream& os)
{
  exp->show(os);
}

void
IfStmt::show(ostream& os)
{
  os << "if(";
  exp->show(os);
  os <<") ";
  thn->show(os);
  os << " else ";
  els->show(os);
}

void
DummyStmt::show(ostream& os)
{
  os << "/*nothing*/";
}

void
SwitchStmt::show(ostream& os)
{
  os << "switch(";
  exp->show(os);
  os << ")";
  stmt->show(os);
}

void
WhileStmt::show(ostream& os)
{
  os << "while(";
  exp->show(os);
  os << ")";
  stmt->show(os);  
}

void
DoStmt::show(ostream& os)
{
  os << "do";
  stmt->show(os);
  os << "while(";
  exp->show(os);
  os << ")";
}

void
ForStmt::show(ostream& os)
{
  os << "for(";
  e1->show(os);
  os << ";";
  e2->show(os);
  os << ";";
  e3->show(os);
  os << ")";
  stmt->show(os);  
}

void
GotoStmt::show(ostream& os)
{
  os << "goto ";
  ind->show(os);
}

void
BreakStmt::show(ostream& os)
{
  os << "break";
}

void
ContStmt::show(ostream& os)
{
  os << "continue";
}

void
ReturnStmt::show(ostream& os)
{
  os << "return ";
  exp->show(os);
}



//////////////////////////////////////////////////

void UserDecl::assign_defseqnum() {
    static unsigned seqnumcounter = 0 ;
    if ( defseqnum == 0 )
        defseqnum = ++seqnumcounter ;
}

EnumDecl::EnumDecl(char const* n, Position p, Plist<VarDecl>* mems)
  : UserDecl(n,p,Enum), translated(NULL)
{
  assert( mems!=NULL && !mems->empty() );
  member_list = mems;
}
