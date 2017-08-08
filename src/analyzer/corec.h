/* -*- c++ -*-
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Represention of a CoreC program
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __COREC__
#define __COREC__

#include <stdio.h>
#include "Plist.h"
#include "auxilary.h"
#include "tags.h"
#include "diagnostic.h"

////// Forward Declarations //////

class StateAnnoAtom ;
class CallAnnoAtom ;

class C_Type;
class C_Decl;
class C_UserDef;
class C_EnumDef;
class C_BasicBlock;
class C_Jump;
class C_Stmt;
class C_Expr;
class C_Pgm;

////// Types //////

/*
    switch(tag) {
    case Abstract:
        break;
    case Primitive:
        break;
    case EnumType:
        break;
    case FunPtr:
        break;
    case Pointer:
        break;
    case Array:
        break;
    case Function:
        break;
    case StructUnion:
        break;
}
*/
typedef enum { Abstract, Primitive, EnumType,
               FunPtr, Pointer, Array, Function, StructUnion } C_TypeTag;

class TypeCompareCallback {
public:
    virtual bool deem_different(C_Type const&,C_Type const&) const;
};

class C_Type : public Numbered {
public:
  const C_TypeTag tag;
private:
  C_Type(C_Type const&); // don't ever call this copy constructor!
public:
  struct US_abstract {
    const char* id;		// Abstract/base type name.
    BaseTypeTag tag;
    unsigned hash;
  };
  struct US_enumtype {
    C_EnumDef* def;           // Enum definition.
  };
  struct US_pointer {
    C_Type* nxt;		// Pointer to.
  };
  struct US_arr {	   
    C_Expr* size;		// Array size.
    C_Type* next;		// Array contents.
  };
  struct US_fun {			
    Plist<C_Type>* params;    // Function parameters. 
    C_Type* ret;              // Function return type
    bool varargs;             // Function has varargs
  };
  struct US_usertype {
    C_UserDef* def;		// User type defintion
    Plist<C_Type>* types;     // Member types.
  };
private:
  union {
    struct US_abstract abstract;
    struct US_enumtype enumtype;
    struct US_pointer pointer;
    struct US_arr arr;
    struct US_fun fun;
    struct US_usertype usertype;
  };
  constvol cv ;
public:
  // Abstract/Base.
  C_Type(BaseTypeTag); // standard types
  C_Type(char const*,BaseTypeTag); // abstract types. Use Void for Abstract
  // Pointer or array
  C_Type(C_TypeTag, C_Type*);
  // Function
  C_Type(Plist<C_Type>*, C_Type*, bool var=false);
  // Struct/Union.
  C_Type(C_UserDef*);
  // Enum.
  C_Type(C_EnumDef*);
  
  
  // ------------------------------------------------------------------------
  // -- Lookup --

  // Primitive or abstract type
  const char* primitive() const;
  BaseTypeTag basetype() const;
  unsigned hash() const;
  // User type.
  C_UserDef* user_def() const;
  Plist<C_Type>& user_types() const;
  // Enum type.
  C_EnumDef* enum_def() const;
  // Pointer type.
  C_Type* ptr_next() const;
  // Array type.
  bool hasSize() const; // Has the array a size?
  C_Expr* array_size() const;
  C_Type* array_next() const;
  // Pointer or array type
  constvol qualifiers() const;
  // Function type.
  Plist<C_Type>& fun_params() const;
  C_Type* fun_ret() const;
  bool fun_varargs() const;

  // ------------------------------------------------------------------------
  // -- Update functions --

  void user_def(C_UserDef*);
  void array_size(C_Expr*);
  void fun_ret(C_Type*);
  void qualifiers(constvol);

  // ------------------------------------------------------------------------

  bool isBase() const;	// True for BasTypes, Abstracts and enums.
  bool isVoid() const;  // Check void type.
  C_Type* array_contents_type(); // Get the first non-array type.
  // Compare types.
  bool equal(C_Type const&,TypeCompareCallback const& =TypeCompareCallback()) const;
  C_Type* copy();
};

////// Initializers //////

typedef enum { Simple, FullyBraced, SloppyBraced, StringInit } C_InitTag;

class C_Init {
  union {
    C_Expr* simple;
    Plist<C_Init>* braced;
    const char* stringinit;
  };
  C_Decl *Owner ;
public:
  C_InitTag tag;
  // Constructors
  C_Init(C_Expr*);
  C_Init(C_InitTag, Plist<C_Init>*);
  C_Init(const char*); // string

  void owner(C_Decl*);
  // Lookup
  C_Expr* simple_init() const;
  Plist<C_Init>& braced_init() const;
  const char* string_init() const;
  C_Decl* owner() const;
};


////// Declarations //////

/*
  switch (d->tag) {
    case VarDcl:
    break;
    case FunDf:
    break;
    case ExtFun:   // there is one of these for each mention of any
    break;         // external function
    case ExtState: // represents part of the internal state of one
    break;         // or a set of external functions
  }
*/
typedef enum { VarDcl, FunDf, ExtFun, ExtState } C_DeclTag;

class C_Decl : public Numbered {
public:
  C_DeclTag tag;
private:
  char const* name;
public:
  C_Type* type;
  constvol qualifiers;
public:
  struct US_fun {
    Plist<C_Decl>* params;	// For function definitons
    Plist<C_Decl>* localdecls;	// For function definitons
    Plist<C_BasicBlock>* body;	// For function definitons
  };
  struct US_var {                        // For variables.
    C_Decl* isIn;               // back pointer
    VariableMode varmode ;
    PositionInUnion varmodeWhy ;
    C_Init* init;               // Initializer.
    C_BasicBlock *initWhere ;
  };
  struct US_ext {
    StateAnnoAtom *effects ;
    CallAnnoAtom *calltime ;
  };
private:
  union {
    struct US_fun fun ;
    struct US_var var ;
    struct US_ext ext ;
  };
  Plist<C_Decl> membs;
  // For structures and arrays: members.
public:
  Position pos;

  // -- Constructor --
  C_Decl(C_DeclTag,C_Type*,char const*);
  C_Decl(C_Decl*); // defined in init.cc

  // -- Lookup --
  bool isContained() const; // Has a containing declaration?
  C_Decl* containedIn() const; // Containing declaration.
  Plist<C_Decl>& members();
  C_Decl* getMember(unsigned);
  // functions.
  Plist<C_Decl>& fun_params() const;
  Plist<C_Decl>& fun_locals() const;
  Plist<C_BasicBlock>& blocks() const;
  C_Type* fun_ret() const;
  // Ordinany vars.
  bool hasInit() const; // Has initializer?
  C_Init* init() const; // Initializer.
  C_BasicBlock* init_where() const;
  VariableMode varmode() const;
  Position varmodeWhy() const;
  // External functions
  CallAnnoAtom *calltime() const;
  StateAnnoAtom *effects() const;

  // -- Update --
  // Functions.
  void fun_params(Plist<C_Decl>* ds);
  void fun_locals(Plist<C_Decl>* ds);
  void blocks(Plist<C_BasicBlock>* bs);
  // Ordinany vars.
  void containedIn(C_Decl*);
  void init(C_Init*);
  void init_where(C_BasicBlock*);
  void varmode(VariableMode,Position);
  // External functions
  void calltime(CallAnnoAtom *);
  void effects(StateAnnoAtom *);
  // External state models
  void subsidiary(C_Decl*);

  // Get the name of the declaration. (Generate one if this is a temp var).
  bool hasName();
  const char* get_name();
};

////// User Types //////

// User type member.
class C_UserMemb : public Numbered {
  const char* name; // Guarded by the name manager.
  C_Expr* expr;
public:
  constvol qualifiers;
  // -- Constructors --
  C_UserMemb(const char*);
  // -- Update --
  void value(C_Expr*);  
  // -- Lookup --
  const char* get_name();
  bool hasValue() const; // Has a value / bitfield size?
  C_Expr* value() const;
};

// Enum definition.
class C_EnumDef : public Numbered {
  const char *name; // Guarded by a name manager.
  Plist<C_UserMemb> membs;  // Members.
public:
  Position pos;
  const unsigned defseqnum ; // Records when the _definition_ was first seen.
  // -- Constructors --
  C_EnumDef(unsigned, const char*, Position);
  // -- Lookup --
  const char* get_name();
  Plist<C_UserMemb>& members();
};

// Struct/union definition.
class C_UserDef : public Numbered {
  const char *name; // Guarded by a name manager.
public:
  const bool isUnion;
  const unsigned defseqnum ; // Records when the _definition_ was first seen.
  Position pos;
  Plist<C_UserMemb> names;  // The members.
  Plist<C_Type> instances; // Instances of this definition.
  // -- Constructors --
  C_UserDef(unsigned, const char*, Position, bool);
  // -- Update --
  void new_instance(C_Type*);
  // Get the name of the declaration. (Generate one if this is a temp var).
  const char* get_name();
  C_UserDef* copy();

  inline C_UserDef* begin() { return this; }
  inline C_UserDef const* begin() const { return this; }
  class iterator {
      Plist<C_Type>::iterator t ;
      Plist<C_UserMemb>::iterator m ;
  public:
      iterator(C_UserDef*);
      C_Type* type();
      C_UserMemb* name();
      void operator ++() ;
      void operator ++(int) {operator ++();}
      operator bool();
  } ;
};


///////// Control Statements ////////////

/*
  switch (tag) {
  case C_If:
    break;
  case C_Goto:
    break;
  case C_Return:
    break;
  }
*/
typedef enum { C_If, C_Goto, C_Return } JumpTag;

class C_Jump : public Numbered {
public:
  JumpTag tag;
public:
  struct US_cond {
    C_Expr *expr;
    C_BasicBlock *thn, *els;
  };
private:
  union {
    C_Expr *exp;		// Return
    struct US_cond cond ;	// If
    C_BasicBlock *target;       // Goto
  };
public:
  Position pos;
  // Return.
  C_Jump(C_Expr*, Position);
  // If-then-else.
  C_Jump(C_Expr*, C_BasicBlock*, C_BasicBlock*, Position);
  // Goto.
  C_Jump(C_BasicBlock *ta);

  //// Lookup functions
  bool hasExpr(); // Has return expression?
  C_Expr* return_expr() const;
  C_Expr* cond_expr() const;
  C_BasicBlock* cond_then() const;
  C_BasicBlock* cond_else() const;
  C_BasicBlock* goto_target() const;

  // Update functions
  void cond_then(C_BasicBlock *b);
  void cond_else(C_BasicBlock *b);
  void goto_target(C_BasicBlock *b);
};

////// Statements //////
/*
  switch (tag) {
  case C_Assign:  // assignments of form *e = e
  case C_Call:    // call of form [*e =] (e)(e_1,...,e_n)
  case C_Alloc:   // allocation of form *e = calloc(e,sizeof(...))   
  case C_Free:    // deallocation of the form free(e).
  case C_Sequence // Statements between sequence points can be evaluated in any order.
  }
*/

typedef enum { C_Assign, C_Call, C_Alloc, C_Free, C_Sequence } C_StmtTag;

class C_Stmt : public Numbered {
public:
  const C_StmtTag tag;
private:
  C_Expr* lefthand;
public:
  struct US_fre {
    C_Expr* e;        
  };
  struct US_assign {		
    C_Expr* source;
  };
  struct US_call {
    C_Expr* fun;
    Plist<C_Expr>* args;
  };
  struct US_alloc {			
    C_Decl* obj;                // Decl that pointers can point to.
    bool isMalloc;              // malloc or calloc.
  };
private:
  union {
    struct US_fre fre ;
    struct US_assign assign ;
    struct US_call call ;
    struct US_alloc alloc ;
  };
public:
  Position pos;
  // The basic constructor makes a sequence point.
  C_Stmt();
  // The left-hand side is missing from the following constructors;
  // it is filled in later when generating Core C
  // Free, assignment
  C_Stmt(C_StmtTag, C_Expr*, Position);
  // Call
  C_Stmt(C_Expr*, Plist<C_Expr>*, Position);
  // Alloc
  C_Stmt(C_Decl*, Position);

  // ------------------------------------------------------------------------
  // Lookup functions
  bool hasTarget() const; // Has left-hand side?
  C_Expr* target() const; // Left-hand side.
  C_Expr* assign_expr() const;
  C_Expr* call_expr() const;
  Plist<C_Expr>& call_args() const;
  bool isMalloc() const;
  C_Decl* alloc_objects() const;
  C_Expr* free_expr() const;
  
  // ------------------------------------------------------------------------
  // Update functions
  void target(C_Expr*); // Left-hand side.
  void isMalloc(bool);
  
  C_Stmt* copy() ;
};

// A basic block is a list of statements with no ingoing edges except to the
// first statement. The last statement in the block must either be an if state-
// ment, a goto statement, or a return statement.

class C_BasicBlock : public Numbered {
private:
  Plist<C_Stmt> stmts;               // Regular statements.
  C_Jump* last;                       // Last statement.
public:
  C_BasicBlock();

  // -- Lookup --
  bool hasExit() const; // Has an jump statement?
  C_Jump* exit() const;
  Plist<C_Stmt>& getBlock();

  // -- Update --
  void exit(C_Jump* s);
};




////// CoreC Expressions //////

/*
  switch (tag) {
  case C_Var:
  case C_ExtFun:
  case C_FunAdr:
  case C_EnumCnst:
  case C_Cnst:
  case C_Null:
  case C_Unary:
  case C_PtrArith:
  case C_PtrCmp:
  case C_Binary:
  case C_Member:
  case C_Array:
  case C_DeRef:
  case C_Cast:
  case C_SizeofT:
  case C_SizeofE:
  default:
  }
*/
typedef enum {
  C_EnumCnst, C_Cnst, C_Null, C_Var, C_ExtFun, C_FunAdr, C_Member,
  C_DeRef, C_Unary, C_PtrArith, C_PtrCmp, C_Binary, C_SizeofT,
  C_SizeofE, C_Array, C_Cast } C_ExprTag;

class C_Expr : public Numbered {
public:
  C_ExprTag tag;
  C_Type* type;
private:
  C_Expr* subexpr1 ;          // all unary and binary exprs
public:
  struct US_binary {
    BinOp op;
    C_Expr *subexpr2;
  };
private:
  union {
    C_UserMemb* enumcnst;       // Enum
    const char* cst;	       	// Constant
    C_Decl *vr;			// Variable
    unsigned member_nr;		// Struct/Union (e.i)
    UnOp unary;			// Unary
    struct US_binary binary;	// Binary/PtrArith/PtrCmp
    C_Type* stype;	       	// Sizeof(T)
  };
public:
  Position pos;

  // Constant
  C_Expr(C_Type*, const char*, Position);
  // Enum constant
  C_Expr(C_UserMemb*, Position);
  // Null pointer constant/Type size.
  C_Expr(C_ExprTag, C_Type*, Position);
  // C_Var / C_FunAdr / C_ExtFun
  C_Expr(C_Decl*, Position);
  // Struct.
  C_Expr(C_Expr*, unsigned, Position);
  // Unary.
  C_Expr(UnOp o, C_Expr*, Position);
  // Binary/PtrCmp/PtrArith.
  C_Expr(BinOp o, C_Expr*, C_Expr*, Position);
  // Dereference/Array decay
  C_Expr(C_ExprTag, C_Expr*, Position);
  // Cast.
  C_Expr(C_Type*, C_Expr*, Position);

  // ------------------------------------------------------------------------
  // Lookup functions
  const char* cnst() const;
  C_Decl* var() const;
  C_UserMemb* enum_cnst() const;
  unsigned struct_nr() const;
  C_UserMemb* struct_name() const;
  UnOp unary_op() const;
  BinOp binary_op() const;
  C_Expr* subexpr() const;
  C_Expr* binary_expr1() const;
  C_Expr* binary_expr2() const;
  C_Type* sizeof_type() const;
  bool isBenign() const;
  
  // ------------------------------------------------------------------------
  // Update functions
  void binary_op(BinOp);
  void binary_expr1(C_Expr*);
  void binary_expr2(C_Expr*);
  void add_member_level() ; // in init.cc
    
  // ------------------------------------------------------------------------
  C_Expr* copy();
};

////// CoreC Programs //////

struct C_Pgm {
  C_Pgm();
  Plist<C_UserDef>    usertypes;	// Struct/union definitions.
  Plist<C_EnumDef>    enumtypes;	// Enum definitions.
  Plist<C_Decl>       globals;	        // Global variables.
  Plist<C_Decl>       functions;	// Specializable functions.
  Plist<C_Decl>       generators;       // Generator pseudo-functions.
  Plist<C_Decl>       exfuns;           // special objects for ext. functions
  // the 'heap' list provide convenient cross-references
  Plist<C_Decl>       heap;	        // Heap allocations.
};

#endif /* __COREC__ */
