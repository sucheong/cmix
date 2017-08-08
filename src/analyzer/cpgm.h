/* -*-c++-*-
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Represention of a C program
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __CPGM__
#define __CPGM__

#include "Plist.h"
#include <iostream.h>
#include "diagnostic.h"
#include "auxilary.h"
#include "tags.h"
#include "output.h"

class SimpleAtom ;
class Interval ;
class ExprTrans ;
class CodeEnv ;
class TypeEnv ;
class OutCpgm ;

class C_UserMemb ;
class C_UserDef ;
class C_EnumDef ;
class C_Init ;
class C_Decl ;
class C_Type ;
class C_Pgm ;

class BTresult ;

class AnnoAtom ;
class StateAnnoAtom ;
class CallAnnoAtom ;

////// DECLARATIONS //////

class Expr;
class Stmt;
class MemberDecl;
class UserDecl;
class Init;
class Type;

// AbstractType can only be induced by a user annotation. It means that
// definition should not be expanded and not included in the generating
// extension nor the residual program. This means that one cannot manipulate
// parts of objects of such type.
enum Linkage { NoLinkage, External, Internal, Typedef, AbstractType };
enum DeclTag { Fundef, Vardecl };

#define VIRTUALS(T,END) \
    virtual void show(ostream&) END \
    virtual ~T() {}

#define DECLVIRTUALS(T,END) \
    VIRTUALS(T,END) \
    public: \
    virtual Output* output(OutCpgm&) END \
    virtual Type* check() END \

#define OBJECTVIRTUALS(T,END) \
    DECLVIRTUALS(T,END) \
    virtual bool is_definition() END \
    virtual void set_initializer(Init*) END \
    virtual Init* initializer() END

class ObjectDecl
{
    bool checked;
public:
    Position pos;
    char const* name;
    DeclTag tag;
    Type* type;
    Linkage linkage;
    C_Decl* translated; // Points to a core object.
    ObjectDecl *refer_to;
    ObjectDecl(char const* n,
               Position p,
               Type* tp,
               DeclTag tg,
               Linkage link)
        : checked(false), pos(p), name(n), tag(tg), type(tp),
	  linkage(link), translated(NULL), refer_to(NULL) {}
    bool isChecked() { return checked || ( checked = true , false ); }
    OBJECTVIRTUALS(ObjectDecl,=0;)
};

// VarDecls contains typedefs, Enum constants, fun prototypes
// and ordinary objects.
class VarDecl : public ObjectDecl
{
public:
    Init* init;
    VariableMode varmode;
    Position varmodeWhy;
    bool static_local; // set whenever declared with static storage class
    C_UserMemb *enum_translated ;
    VarDecl(char const* n,
            Position p,
            Type* t,
            Linkage link,
            VariableMode vm)
        : ObjectDecl(n,p,t,Vardecl,link), init(NULL), varmode(vm),
          static_local(false), enum_translated(NULL) {}
    OBJECTVIRTUALS(VarDecl,;)
    Interval trans(CodeEnv &,bool PreferStatements=false) ;
};

class FunDef : public ObjectDecl
{
public:
    Plist<VarDecl>* decls;
    Stmt* stmts;
    StateAnnoAtom *effects ;
    CallAnnoAtom *calltime ;
    FunDef(char const* n,
           Position p,
           Type* t,
           Linkage link,
           Plist<VarDecl>* ds)
        : ObjectDecl(n,p,t,Fundef,link), decls(ds),
          stmts(NULL), effects(NULL), calltime(NULL)
        {}
    OBJECTVIRTUALS(FunDef,;)
    void trans(CodeEnv &) ;
};

/////////////////////////////////////////////////////////////////////////////////////

#define USERDECLVIRTUALS(T,END) \
    VIRTUALS(T,END) \
    virtual void check() END \
    virtual bool complete() END \
    virtual Output* output(OutCpgm&) END \
    virtual C_Type* trans(TypeEnv const&) END \
    virtual void trans(C_Pgm &,TypeEnv const&) END \
    public:

class UserDecl {
    bool checked;
  public:
    unsigned defseqnum ;
    const Position pos;
    UserDecl *refer_to ;
    char const* name;
    UserTag tag;
    UserDecl(char const* n, Position p, UserTag t)
        : checked(false), defseqnum(0), pos(p),
          refer_to(NULL), name(n), tag(t) {} 
    bool isChecked() { return checked || ( checked = true , false ); }
    void assign_defseqnum();
    USERDECLVIRTUALS(UserDecl,=0;);
};

class StructDecl : public UserDecl  // Both for structs and unions!
{
    static int nextmark; // This is to avoid infinite comparison.
    int mark;
    C_Type *rectrans ;
public:
    Plist<MemberDecl> *member_list;	     // Members
    C_UserDef *translated ;
    int marker; // Used to avoid infinite processing.
    StructDecl(char const* n,
               Position p,
               UserTag t,
               Plist<MemberDecl> *mems = NULL)
        : UserDecl(n,p,t), mark(0), rectrans(NULL),
          member_list(mems != NULL ? mems : new Plist<MemberDecl>),
          translated(NULL) {}
    USERDECLVIRTUALS(StructDecl,;);
};

class EnumDecl : public UserDecl
{
public:
    C_EnumDef *translated ;
    Plist<VarDecl>* member_list;            // Enum-names
    EnumDecl(char const*, Position, Plist<VarDecl>*);
    USERDECLVIRTUALS(EnumDecl,;);
};

class MemberDecl
{
public:
    VarDecl* var;
    Expr* bitfield;
    MemberDecl(VarDecl* v, Expr* bits)
        : var(v), bitfield(bits) {}
    VIRTUALS(MemberDecl,;);
public:
    Output* output(OutCpgm&);
};



////// TYPES //////

typedef enum { BaseT, PtrT, ArrayT, FunT,
               StructT, UnionT, EnumT, AbstractT } TypeTag;
/*	switch (t1->realtype) {
	case BaseT:
	case PtrT:
	case ArrayT:
	case FunT:
	case StructT:
	case UnionT:
	case EnumT:
        case AbstractT:
	} */

#define ALLTYPEVIRTUALS(T,END) \
    VIRTUALS(T,END); \
    virtual Output* output(OutCpgm&,Output* =NULL, bool precedence=0) END \
    virtual T* copy() END \
    /* ANSI C section 6.1.2.5: */ \
    virtual bool isObject() END \
    virtual bool isIntegral() END \
    virtual bool isArithmic() END \
    virtual bool isAggregate() END \
    virtual bool isDerived() END \
    virtual void check() END

#define TYPEVIRTUALS(T,END) \
    ALLTYPEVIRTUALS(T,END); \
    protected: \
    virtual bool equals (Type*) END \
    public: \
    virtual C_Type* trans(TypeEnv const&) END 

class Type {    
    bool checked ;
public:
    Plist<C_Type> btannos ;
    const TypeTag realtype; // Used to compare types.
    constvol cv ;
    const bool isFun; // is this a function type?
    bool isVoid;
    Type(TypeTag t, bool fun=false, bool ell=false);
    Type(Type& t);
    bool isChecked() { return checked || ( checked = true , false ); }

    TYPEVIRTUALS(Type,=0;);
    virtual bool isModifiable() { return !cv.cst && isObject(); }
    virtual Type *isPointer() { return NULL; }
    virtual constvol qualifiers() { return cv; }

    bool fully_equals(Type*);
    // The == operator defines a useful type equality concept.
    // It ignores qualifiers at the top level but requires qualifiers
    // of pointed-to types to match.
    bool operator==(Type &);
};

class AbsType : public Type {
public:
    char const *name ;
    char const *properties ;
    AbsType(char const *n, char const *p)
        : Type(AbstractT), name(n), properties(p) {}
    AbsType(AbsType& t) : Type(t), name(t.name), properties(t.properties) {}
    TYPEVIRTUALS(AbsType,;);
};               

class BaseType : public Type {
public:
  const BaseTypeTag tag;
  BaseType(BaseTypeTag t): Type(BaseT), tag(t)
    { if (t==Void) isVoid=true; }
  TYPEVIRTUALS(BaseType,;);
  static BaseType* get(BaseTypeTag t);
};

class PtrType : public Type {
protected:
    PtrType(TypeTag tag,Type *t) : Type(tag), next(t) {}
public:
    Type* next;
    PtrType(Type* t) : Type(PtrT), next(t) {}
    PtrType(PtrType& t) : Type(t), next(t.next) {}
    TYPEVIRTUALS(PtrType,;);
    virtual Type *isPointer() { return next; }
};

class ArrayType : public PtrType {
public:
    Expr* size;  // NULL means no size.
    ArrayType(ArrayType& t) : PtrType(t), size(t.size) {}
    ArrayType(Expr* s, Type* t)
        : PtrType(ArrayT,t), size(s) {}
    TYPEVIRTUALS(ArrayType,;);
    virtual Type *isPointer() { return NULL; }
    virtual constvol qualifiers() { return next->qualifiers(); }
};

class FunType : public Type {
  public:
    Plist<Type>* params;
    Type* ret;
    bool varargs ;
    Position pos ;
    FunType(FunType& t)
      : Type(t), params(t.params), ret(t.ret), varargs(t.varargs) {}
    FunType(Plist<Type>* par, bool v, Type* t, Position p)
      : Type(FunT, true), params(par), ret(t), varargs(v), pos(p) {}
    bool unspecified() ;
    
    TYPEVIRTUALS(FunType,;);
};


////////////////////////

#define USERTYPEVIRTUALS(T,END) \
    ALLTYPEVIRTUALS(T,END); 

class UserType : public Type {
public:
    UserDecl* def;
    UserType(TypeTag t, UserDecl* d ) : Type(t), def(d) {}
    UserType(UserType& t) : Type(t), def(t.def) {}
    USERTYPEVIRTUALS(UserType,=0;);
    virtual bool equals(Type*) ;
    virtual C_Type* trans(TypeEnv const&);
};

class StructType : public UserType {
  public:
    StructType(UserDecl* d) : UserType(StructT, d) {}
    StructType(StructType& t) : UserType(t) {}
    USERTYPEVIRTUALS(StructType,;);
    virtual bool isModifiable();
};

class UnionType : public UserType {
  public:
    UnionType(UserDecl* d) : UserType(UnionT, d) {}
    UnionType(UnionType& t) : UserType(t) {}
    USERTYPEVIRTUALS(UnionType,;);
    virtual bool isModifiable();
};

class EnumType : public UserType {
  public:
    EnumType(UserDecl* d) : UserType(EnumT, d) {}
    EnumType(EnumType& t) : UserType(t) {}
    USERTYPEVIRTUALS(EnumType,;);
};


///////////////////////// STATEMENTS ////////////////////////////////

struct StmtCallbackClosure {
  // JPS: Here be dragons...
    virtual void whatever(Stmt*) =0;
} ;

#define STMTVIRTUALS(T,END) \
    VIRTUALS(T,END) \
    virtual void check() END \
    virtual Output* output(OutCpgm&) END \
    virtual Interval trans(CodeEnv &) END 

class CompoundStmt ;
    
class Stmt
{
    bool checked;
public:
    const Position pos;
    Stmt(Position p) : checked(0), pos(p) {}
//    bool isChecked() { return checked || ( checked = true , false ); }
    STMTVIRTUALS(Stmt,=0;)
    virtual void preorder(StmtCallbackClosure*) =0 ;
    virtual CompoundStmt *this_as_compound();
};

class NullaryStmt : public Stmt {
public:
    virtual void preorder(StmtCallbackClosure*) ;
    NullaryStmt(Position p) : Stmt(p) {}
};

class UnaryStmt : public Stmt {
public:
    Stmt* stmt;
    virtual void preorder(StmtCallbackClosure*) ;
    UnaryStmt(Position p,Stmt *s) : Stmt(p), stmt(s) {}
};

class LabelStmt : public UnaryStmt
{
public:
    char* label;
    SimpleAtom* translated; // Points to a jump point in the proto-flowgraph
    LabelStmt (char* n, Stmt* s, Position p)
        : UnaryStmt(p,s), label(n), translated(NULL) {}
    STMTVIRTUALS(LabelStmt,;)
};

class CaseStmt : public UnaryStmt
{
    Plist<C_Type> btannos ;
public:
    Expr* exp; // This must be an integral constant.
    CaseStmt (Position p, Expr* e, Stmt* s)
        : UnaryStmt(p,s), exp(e) {}
    STMTVIRTUALS(CaseStmt,;)
};

class DefaultStmt : public UnaryStmt
{
    Plist<C_Type> btannos ;
public:
    DefaultStmt (Position p, Stmt* s)
        : UnaryStmt(p,s) {}
    STMTVIRTUALS(DefaultStmt,;)
};

class CompoundStmt : public Stmt
{
public:
    Plist<VarDecl>* objects;
    Plist<Stmt>* stmts;
    CompoundStmt (Position p,
                  Plist<VarDecl>* os,
                  Plist<Stmt>* ss)
        : Stmt(p), objects(os), stmts(ss) {}
    STMTVIRTUALS(CompoundStmt,;)
    virtual void preorder(StmtCallbackClosure*) ;
    virtual CompoundStmt *this_as_compound();
};

class ExprStmt : public NullaryStmt
{
public:
    Expr* exp;
    ExprStmt (Position p, Expr* e)
        : NullaryStmt(p), exp(e) {}
    STMTVIRTUALS(ExprStmt,;)
};

class IfStmt : public Stmt
{
    Plist<C_Type> btannos ;
public:
    Expr* exp;
    Stmt *thn,*els;
    IfStmt (Position p, Expr* e, Stmt* s1, Stmt* s2)
        : Stmt(p), exp(e), thn(s1), els(s2) {}
    STMTVIRTUALS(IfStmt,;)
    virtual void preorder(StmtCallbackClosure*) ;
};

class DummyStmt : public NullaryStmt
{
public:
    DummyStmt (Position p)
        : NullaryStmt(p) {}
    STMTVIRTUALS(DummyStmt,;)
};

class SwitchStmt : public UnaryStmt
{
    Plist<C_Type> btannos ;
public:
    Expr* exp;
    SwitchStmt (Position p, Expr* e, Stmt *s)
        : UnaryStmt(p,s), exp(e) {}
    STMTVIRTUALS(SwitchStmt,;)
};

class WhileStmt : public UnaryStmt
{
    Plist<C_Type> btannos ;
public:
    Expr* exp;
    WhileStmt (Position p, Expr* e, Stmt *s)
        : UnaryStmt(p,s), exp(e) {}
    STMTVIRTUALS(WhileStmt,;)
};

class DoStmt : public UnaryStmt
{
    Plist<C_Type> btannos ;
public:
    Expr* exp;
    DoStmt (Position p, Expr* e, Stmt *s)
        : UnaryStmt(p,s), exp(e) {}
    STMTVIRTUALS(DoStmt,;)
};

class ForStmt : public UnaryStmt
{
    Plist<C_Type> btannos ;
public:
    Expr* e1,*e2,*e3;
    ForStmt (Position p, Expr* a, Expr* b, Expr* c, Stmt *s)
        : UnaryStmt(p,s), e1(a), e2(b), e3(c) {}
    STMTVIRTUALS(ForStmt,;)
};

class Indirection 
{
    Position pos ;
public:
    LabelStmt* stmt;
    Indirection (Position p, LabelStmt* s)
        : pos(p), stmt(s) {}

    void check();
    Output* output(OutCpgm &);
    void show(ostream&);
};

class GotoStmt : public NullaryStmt
{
public:
    Indirection* ind;
    GotoStmt (Position p, Indirection* i)
        : NullaryStmt(p), ind(i) {}
    STMTVIRTUALS(GotoStmt,;)
};

class BreakStmt : public NullaryStmt
{
public:
    BreakStmt (Position p)
        : NullaryStmt(p) {}
    STMTVIRTUALS(BreakStmt,;)
};

class ContStmt : public NullaryStmt
{
public:
   ContStmt (Position p)
        : NullaryStmt(p) {}
    STMTVIRTUALS(ContStmt,;)
};

class ReturnStmt : public ExprStmt
{
    Plist<C_Type> btannos ;
public:
    ReturnStmt (Position p, Expr* e) : ExprStmt(p,e) {}
    STMTVIRTUALS(ReturnStmt,;)
};




//////////////////////// EXPRESSIONS /////////////////////////

#define EXPRVIRTUALS(T,END) \
    VIRTUALS(T,END) \
    virtual Type* check() END \
    virtual Output* output(OutCpgm&) END \
    virtual int precedence() END \
    virtual void trans(CodeEnv&,ExprTrans&,bool ignore) END

class Expr
{
    Expr();
    bool checked ;
protected:
    bool isChecked() { return checked || ( checked = true , false ); }
    Plist<C_Type> btannos ;
public:
    const Position pos;
    const bool isConst ;
    const bool isAssign ;
    bool isRval ;
    bool valueKnown ;
    long knownValue ;
    Type* exprType;
public:
    Expr(Position p, int c = 0, int a = 0)
        : checked(0), pos(p), isConst(c), isAssign(a), isRval(true),
          valueKnown(false),
          exprType(NULL) {}
    void setRealType(Type* t) { exprType = t; }
    // Check that an expression is an rvalue (cast if necessary).
    inline Type* type() { return exprType; }
    void ok(ExprTrans&);
    virtual char const *string_literal() { return NULL; }
    virtual Expr *copy_plusone() ;
    EXPRVIRTUALS(Expr,=0;)
};

class ConstExpr : public Expr
{
public:
    const char* literal;
    const ConstantTag tag;
    ConstExpr(char*,ConstantTag,const Position);
    virtual Expr *copy_plusone() ;
    EXPRVIRTUALS(ConstExpr,;)
    virtual char const *string_literal()
        {
            if ( tag == StrConst )
                return literal ;
            return NULL;
        }
};

ConstExpr* makeConstExpr(long, const Position);

class NullExpr : public Expr {
public:
    NullExpr(const Position);
    EXPRVIRTUALS(NullExpr,;)
};

class VarExpr : public Expr
{
public:
    ObjectDecl* decl;
    AnnoAtom *annos ;
    VarExpr(ObjectDecl* d,Position p);
    EXPRVIRTUALS(VarExpr,;)
};

class CallExpr : public Expr
{
public:
    Expr* fun;
    Plist<Expr>* args;
    CallExpr (Expr* f, Position p, Plist<Expr>* a)  
        : Expr(p), fun(f), args(a) {}
    EXPRVIRTUALS(CallExpr,;)
};

class ArrayExpr : public Expr
{
public:
    Expr* left,*right;
    ArrayExpr (Expr* lf, Expr* rt, Position p)
        : Expr(p), left(lf), right(rt) {}
    EXPRVIRTUALS(ArrayExpr,;)
};

class DotExpr : public Expr
{
public:
    Expr* left;
    char* member;
    unsigned memindex;
    DotExpr (Expr* lf, char* m, Position p)
        : Expr(p), left(lf), member(m) {}
    EXPRVIRTUALS(DotExpr,;)
};

class UnaryExpr : public Expr
{
public:
    Expr* exp;
    UnOp op;
    UnaryExpr (Expr* e)
        : Expr(e->pos), exp(e), op(DeRef) {}
    UnaryExpr (Expr* e, UnOp o, Position p);
    EXPRVIRTUALS(UnaryExpr,;)
};

typedef enum { Decr, Inc } Incrementer;
class PreExpr : public Expr
{
public:
    Expr* exp;
    Incrementer op;
    PreExpr (Expr* e, Incrementer o, Position p)
        : Expr(p), exp(e), op(o) {}
    EXPRVIRTUALS(PreExpr,;)
};

class PostExpr : public Expr
{
public:
    Expr* exp;
    Incrementer op;
    PostExpr (Expr* e, Incrementer o, Position p)
        : Expr(p), exp(e), op(o) {}
    EXPRVIRTUALS(PostExpr,;)
};

class TypeSize : public Expr
{
public:
    Type* typesz;
    TypeSize (Type* t, Position p) : Expr(p), typesz(t) {}
    EXPRVIRTUALS(TypeSize,;)
};

class ExprSize : public Expr
{
public:
    Expr* exp;
    ExprSize (Expr* e, Position p)
        : Expr(p), exp(e) {}
    EXPRVIRTUALS(ExprSize,;)
};

class CastExpr : public Expr
{
public:
    // The type of the expression is of course the type of the cast.
    Expr* exp;
    bool silent ;
    CastExpr (Type* t, Expr* e, Position p, bool s = false)
        : Expr(p), exp(e), silent(s)
        {
            setRealType(t);
            valueKnown = e->valueKnown ;
            knownValue = e->knownValue ;
        }
    EXPRVIRTUALS(CastExpr,;)
};

class BinaryExpr : public Expr
{
public:
    Expr *left,*right;
    BinOp op;
    BinaryExpr (Expr* lt, Expr* rt, BinOp o, Position p);
    virtual Expr *copy_plusone() ;
    EXPRVIRTUALS(BinaryExpr,;)
};

class CommaExpr : public Expr
{
public:
    Expr* left, *right;
    CommaExpr (Expr* lt, Expr* rt,Position p);
    EXPRVIRTUALS(CommaExpr,;)
};

class CondExpr : public Expr
{
public:
    Expr *cond, *left, *right;
    CondExpr (Expr* c,Expr* lt, Expr* rt,Position p);
    EXPRVIRTUALS(CondExpr,;)
};

typedef enum { MulAsgn, DivAsgn, ModAsgn, AddAsgn, SubAsgn,
               LSAsgn, RSAsgn, AndAsgn, EOrAsgn, OrAsgn, Asgn } AssignOp;
/*    switch (op) {
    case MulAsgn:
    case DivAsgn:
    case ModAsgn:
    case AddAsgn:
    case SubAsgn:
    case LSAsgn:
    case RSAsgn:
    case AndAsgn:
    case EOrAsgn:
    case OrAsgn:
    case Asgn:
    } */
class AssignExpr : public Expr
{
public:
    Expr *left, *right;
    AssignOp op;
    AssignExpr (Expr* lt, Expr* rt, AssignOp o, Position p)
        : Expr(p,0,1), left(lt), right(rt), op(o) {}
    EXPRVIRTUALS(AssignExpr,;)
};


//////////////////////////////////////////

typedef enum { InitList, InitElem } InitTag;

class Init {
    union {
	Expr* exp;
	Plist<Init>* inits;
    };
  public:
    const InitTag tag;
    bool sloppy ;
    Init() : tag(InitList) { inits = new Plist<Init>(); }
    Init(Expr *e) : tag(InitElem), sloppy(false), exp(e) {}
    Init(Init *i)
        : tag(InitList), sloppy(false) { inits = new Plist<Init>(i); }
    Init(Plist<Init>* lst) : tag(InitList), sloppy(false), inits(lst) {}
    Expr* expr()
    {
        assert(tag==InitElem);
        return exp;
    }
    void expr(Expr* e)
    {
        assert(tag==InitElem);
        exp = e;
    }
    bool isSimple() { return tag==InitElem; }
    Init* append(Init* i)
    {
	assert(tag == InitList);
	inits->push_back(i);
	return this;
    }
    int size()
    {
        assert(tag==InitList);
        return inits->size();
    }
    int empty()
    {
        assert(tag==InitList);
        return inits->empty();
    }
    void removeFirst()
    {
        assert(tag==InitList);
        assert(inits->size() != 0);
        inits->pop_front();
    }
    Init* front()
    {
        assert(tag==InitList);
        assert(inits->size() != 0);
        return inits->front();
    }
    Plist<Init>::iterator begin()
    {
        assert(tag==InitList);
        return inits->begin();
    }
    Position pos()
    {
        Init *th = this ;
        while(th->tag==InitList)
            th = th->inits->back() ;
        return th->exp->pos ;
    }
    VIRTUALS(Init,;)
    C_Init *trans(CodeEnv &);
    Output* output(OutCpgm &);
};

/////// C PROGRAMS //////

struct CProgram {
    Plist<FunDef>*   functions;
    Plist<VarDecl>*  definitions;    // Real objects (for which storage must be allocated.
    Plist<UserDecl>* usertypes;	     // structures, unions, and enumerations
    void output(OutputContainer &,BTresult &);
    void check();
};

/////// ANALYSIS PHASES OPERATING ON ANSI C REPRESENTATION /////

void check_program (CProgram* pgm);

#endif /* __CPGM__ */

