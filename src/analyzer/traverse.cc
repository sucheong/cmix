/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm
 * Content:  C-Mix system: Prototype code for Core C traversal
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "corec.h"

#define debug debugstream_xxx

static void traverse(C_Type&);
static void traverse(C_UserDef&);
static void traverse(C_EnumDef&);
static void traverse(C_Decl&);
static void traverse(C_Init&);
static void traverse(C_Jump&);
static void traverse(C_Stmt&);
static void traverse(C_Expr&);

////// Types //////

static void
traverse(C_Type &t)
{
    switch(t.tag) {
    case Primitive:
    case Abstract:
        (const char*)t.primitive();
        t.hash();
        break;
    case EnumType:
        (C_EnumDef&)t.enum_def();
        break;
    case FunPtr:
    case Pointer:
        traverse(t.ptr_next());
        break;
    case Array:
        if ( t.hasSize() )
            (C_Expr&)t.array_size();
        traverse(t.array_next());
        break;
    case Function:
        (bool)t.fun_varargs();
        foreach(arg,t.fun_params(),Plist<C_Type>)
            traverse(**arg);
        traverse(t.fun_ret());
        break;
    case StructUnion:
        // Be sure to prevent infinte recursion somehow
        (C_UserDef&)t.user_def();
        foreach(member,t.user_types(),Plist<C_Type>)
            traverse(**member);
        break;
    }
}

////// User declarations //////

static void
traverse(C_UserDef &ud)
{
    foreach(m,ud,C_UserDef) {
        if ( m.name().hasValue() )
            traverse(m.name().value());
        (C_Type&)m.type() ;
    }
    (Plist<C_Type>&)ud.instances ;
}

static void
traverse(C_EnumDef &ed)
{
    foreach(m,ed.members(),Plist<C_UserMemb>) {
        if ( m->hasValue() )
            traverse(m->value());
    }
}
    
/////// Declarations ///////

static void
traverse(C_Decl &d)
{
  switch (d.tag) {
  case VarDcl:
      traverse(d.type);
      (VariableMode)d.varmode();
      foreach(subsiduary,d.members(),Plist<C_Decl>)
          traverse(**subsiduary);
      if ( d.hasInit())
          traverse(d.init());
      break;
  case FunDf:
      traverse(d.type);
      foreach(par,d.fun_params(),Plist<C_Decl>)
          traverse(**par);
      foreach(local,d.fun_locals(),Plist<C_Decl>)
          traverse(**local);
      foreach(bb,d.blocks(),Plist<C_BasicBlock>) {
          foreach(stmt,bb->getBlock(),Plist<C_Stmt>)
              traverse(**stmt);
          traverse(bb->exit());
      }
  }
}
  

static void
traverse(C_Init &i)
{
    switch(i.tag) {
    case Simple:
        traverse(i.simple_init());
        break ;
    case FullyBraced:
    case SloppyBraced:
        foreach(j,i.braced_init(),Plist<C_Init>)
            traverse(**j);
        break ;
    case StringInit:
        (const char*)i.string_init();
        break ;
    }
}

////// Control statements //////

static void
traverse(C_Jump &j)
{
    switch (j.tag) {
    case C_If:
        traverse(j.cond_expr());
        (C_BasicBlock&)j.cond_then();
        (C_BasicBlock&)j.cond_else();
        break;
    case C_Goto:
        (C_BasicBlock&)j.goto_target();
        break;
    case C_Return:
        if ( j.hasExpr() )
            traverse(j.return_expr());
        break;
    }
}

////// Statements //////

static void
traverse(C_Stmt &s)
{
    if ( s.hasTarget() )
        traverse(s.target());
    switch (s.tag) {
    case C_Assign:
        traverse(s.assign_expr());
        break;
    case C_Call:    // call of form [x =] (e_0)(e_1,...,e_n)
        traverse(s.call_expr());
        foreach(arg,s.call_args(),Plist<C_Expr>)
            traverse(**arg);
        break;
    case C_Alloc:
        (bool)s.isMalloc();
        traverse(s.alloc_size());
        traverse(s.alloc_object());
        break ;
    case C_Free:    // deallocation.
        traverse(s.free_expr());
        break;
    case C_Sequence: // sequence point
        break;
    }
}

////// CoreC Expressions //////

static void
traverse(C_Expr &e)
{
    (C_Type*)e.type ;
    switch (e.tag) {
    case C_Cnst:            // Constants - NB! can have complex types
        traverse(*e.type);
        (const char*)e.cnst();
        break;
    case C_EnumCnst:
        traverse(*e.type);
        (C_UserMemb&)e.enum_cnst();
        break ;
    case C_Null:            // Null pointer constants of various types
        traverse(*e.type);
        break;
    case C_ExtFun:         // the address of an external function
        e.var();  // whose type is FunPtr
        break ;
    case C_FunAdr:         // the address of an internal function
        e.var();
        break;
    case C_Var:            // the address of a variable
        e.var();
        break;
    case C_Member:         // pointer to struct ==> pointer to member
        // owns the topmost level of its type
        traverse(e.subexpr());
        (C_UserDecl)e.struct_name();
        (int)e.struct_nr();
        break;
    case C_DeRef:           // pointer to value ==> value
        traverse(e.subexpr());
        break ;
    case C_Unary:           // unary arithmetic operator
        traverse(*e.type);
        switch (e.unary_op())
        {
        case Neg:
        case Not:
        case Bang:
        default:    // other values should never occur
            ;
        }        
        traverse(e.subexpr());
        break;
    case C_Binary:          // arith @ arith ==> arith
    case C_PtrCmp:          // pointer @ pointer ==> integral
        traverse(*e.type);
        // fall through
    case C_PtrArith:        // pointer @ integral ==> pointer
        switch (e.binary_op()) {
        case Mul:
        case Div:
        case Mod:
        case Add:
        case Sub:
        case LShift:
        case RShift:
        case LT:
        case GT:
        case LEq:
        case GEq:
        case Eq:
        case NEq:
        case BAnd:
        case BEOr:
        case BOr:
        case And:
        case Or:
            ;
        }
        traverse(e.binary_expr1());
        traverse(e.binary_expr2());
        break;
    case C_SizeofT:         // sizeof (type)
        traverse(*e.type);
        traverse(e.sizeofT_type());
        break;
    case C_SizeofE:
        traverse(*e.type);
        traverse(e.subexpr());
        break;
    case C_Array:           // pointer to array ==> pointer to first element
        traverse(e.subexpr());
        break;
    case C_Cast:            // random cast. Possibly arithmetic, possibly sick.
        traverse(*e.type);
        traverse(e.subexpr());
        break;
    }
}

////// CoreC program //////

static void
traverse(C_Pgm &pgm)
{
  if (debug^1) debug << "\ndoing something ";
  // Traverse the program.
  foreach(ud,pgm.usertypes,Plist<C_UserDef>)
      traverse(**ud);
  foreach(ed,pgm.enumtypes,Plist<C_EnumDef>)
      traverse(**ed);
  foreach(var,pgm.globals,Plist<C_Decl>)
      traverse(**var);
  foreach(fun,pgm.functions,Plist<C_Decl>)
      traverse(**fun);
  if (debug^1) debug << endl;
}
