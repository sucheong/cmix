/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix gegen subsystem: Generator functions; statements
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "gegen.h"
#include "options.h"

void
GegenEnv::specializable_heading(C_Decl *f,bool proto)
{
  ost << "static " << alwayscode << pushtype(f->fun_ret(),constvol()) ;
  if ( !proto )
    ost << '\n' ;
  ost << names[f] << '(' ;
  unsigned n = 1 ;
  foreach(i,f->fun_params(),Plist<C_Decl>) {
    if ( n > 1 ) ost << ',' ;
    if ( spectime(*i) ) {
      if ( proto )
        ost << AbstractDecl(i->type) ;
      else
        ost << *i ;
    } else {
      ost << "Code" ;
      if ( !proto )
        ost << " cmixRP" << n ;
    }
    n++ ;
  }
  if ( n == 1 )
    ost << "void" ;
  ost << ')' << poptype << ( proto ? ";\n" : "\n" );
};

static void
residual_heading(GegenEnv &env,C_Decl *f,char const *linkage,char const *name)
{
  bool isvoid = f->fun_ret()->isVoid() || env.spectime(f->fun_ret()) ;
  env.ost << enter << linkage ;
  if ( isvoid )
    env.ost << "void" ;
  else
    env.ost << pushtype(f->fun_ret(),constvol()) ;
  env.ost << "\\n" << userhole ;
  bool first = true ;
  foreach(p,f->fun_params(),Plist<C_Decl>) {
    if ( env.spectime(*p) )
      continue ;
    env.ost << ( first ? '(' : ',' ) << *p ;
    first = false ;
  }
  if ( first )
    env.ost << "(void" ;
  env.ost << ')' ;
  if ( !isvoid )
    env.ost << poptype ;
  env.ost << exit << ',' << name << exit ;
}

struct FunGegen {
  GegenEnv &env ;
  C_Decl *const fun ;
  C_Type *returntype ;
  enum { STATIC, VOID, DYNAMIC } retmode ;

  unsigned DOcounter ;
  bool unfolding ;
  bool use_cmixSR ;
  bool cmixSRisExp ;
  bool use_pendinglist ;
  array<C_BasicBlock,unsigned> speclabels ;
    
  FunGegen(GegenEnv*,C_Decl *);

  void EmitStatement(C_Stmt *s) const;
  virtual bool EmitGoto(C_BasicBlock *, bool emitit) const;
  bool EmitJump(C_Jump *j) const;
  void EmitReturnSequence(C_Expr *e) const ;
  void EmitBasicBlock(C_BasicBlock *) const;
  void ConstructResidualCall() const;

  virtual ~FunGegen() {}
};

FunGegen::FunGegen(GegenEnv *e, C_Decl *f)
  : env(*e), fun(f), returntype(f->fun_ret()), speclabels(0)
{
  if ( returntype->isVoid() )            retmode = VOID ;
  else if ( env.bt.Dynamic(returntype) ) retmode = DYNAMIC ;
  else                                   retmode = STATIC ;
}

void
FunGegen::EmitStatement(C_Stmt *s) const
{
  switch (s->tag) {
  case C_Assign: { // *right = left
    C_Expr *left = s->target() ;
    C_Expr *right = s->assign_expr() ;
        
    if ( env.bt.Dynamic(left->type->ptr_next()) ) {
      env.ost << "  cmixStmt(" << enter << EmitDeref(left,4) << " = "
              << EmitExpr(right,3) << exit << ");\n" ;
    } else {
      env.ost << "  " << EmitDeref(left,4) << " = " << EmitExpr(right,3)
              << ";\n" ;
    }
    break ; }
  case C_Call: {   // [*e =] f(e_1,...,e_n)
    C_Type *ret = s->call_expr()->type->ptr_next()->fun_ret() ;
        
    bool const retVarStatic
      = s->hasTarget() && env.bt.Static(s->target()->type->ptr_next()) ;
    bool const retVarDynamic
      = s->hasTarget() && env.bt.Dynamic(s->target()->type->ptr_next()) ;
        
    if ( env.bt.Static(s->call_expr()->type) ) {
      // spectime external call and call to specializable function
      // use the same conventions (i.e., these conventions collapse
      // into normal C calling sequences if the call has no dynamic
      // components).
      int nparens = 0 ;
      env.ost << "  " ;
      if ( retVarStatic ) {
        // this guarantees that the return value is not dynamic
        // so we need not emit anything to the residual pgm.
        env.ost << EmitDeref(s->target(),4) << " = " ;
      } else if ( retVarDynamic ) {
        nparens++ ;
        env.ost << "cmixStmt(" << enter << EmitDeref(s->target(),4)
                << " = " ;
        if ( env.bt.Static(ret) ) {
          char const *converter = env.ost.EmitLiftingHole(ret);
          env.ost << exit << ',' << converter ;
          nparens++ ;
        } else
          env.ost << userhole << exit << ',' ;
        env.ost << exit ; // almost no-op: the userhole was last
      } else if ( env.bt.Dynamic(ret) && !ret->isVoid() )
        env.ost << "cmixStmt(\"?\",", nparens++ ;
      env.ost << EmitExpr(s->call_expr(),30) << '(' , nparens++ ;
      bool first = true ;
      Plist<C_Type>::iterator realtypes
        = s->call_expr()->type->ptr_next()->fun_params().begin() ;
      foreach(a,s->call_args(),Plist<C_Expr>) {
        if ( !first ) env.ost << ',' ;
        first = false ;
                
        // variadic parameters always have same binding time
        // as the call itself
        if ( realtypes && env.bt.Dynamic(*realtypes) )
          env.ost << "cmixMkExp("
                  << enter << EmitExpr(*a,4) << exit << ')' ;
        else
          env.ost << EmitExpr(*a,2);
        if ( realtypes ) realtypes++ ;
      }
      while ( nparens-- )
        env.ost << ')' ;
      env.ost << ";\n" ;
    } else {
      // residual external call
            
      env.ost << "  cmixStmt(" << enter ;
      if ( retVarDynamic )
        env.ost << EmitDeref(s->target(),4) << " = " ;
      env.ost << EmitExpr(s->call_expr(),29) << '(' ;
      bool first = true ;
      foreach(a,s->call_args(),Plist<C_Expr>) {
        if ( !first ) env.ost << ',' ;
        first = false ;
        env.ost << EmitExpr(*a,2);
      }
      env.ost << ')' << exit << ");\n" ;
    }
    break ; }
  case C_Alloc: {  // x = malloc(e*sizeof(...))
    // an allocation
    C_Type *pseudoarray = s->alloc_objects()->type ;
    C_Decl *realdecl = s->alloc_objects()->members().front() ;
    C_Type *aloctype = pseudoarray->array_next() ;
    C_Expr *size = NULL ;
    if ( pseudoarray->hasSize() )
      size = pseudoarray->array_size() ;
        
    // if the pseudoarray is dynamically split just residualize
    // the allocation
    if ( env.bt.Dynamic(pseudoarray) ) {
      env.ost << "  cmixStmt(" << enter << EmitDeref(s->target(),3);
      if ( s->isMalloc() ) {
        env.ost << " = malloc(" ;
        if ( size )
          env.ost << EmitExpr(size,26) << '*' ;
      } else {
        assert(size);
        env.ost << " = calloc(" << EmitExpr(size,2) << ',' ;
      }
      env.ost << "sizeof(" << AbstractDecl(aloctype) << "))"
              << exit << ");\n" ;
    } else if ( !env.spectime(realdecl) ) {
      // the contents are dynamic: create global variables
            
      env.ost << "  { typedef "
              << Unqualified(aloctype,"cmixAloctype")
              << ";\n    cmixAloctype *cmixSA = malloc(sizeof(cmixAloctype)" ;
      if ( size )
        env.ost << '*' << EmitExpr(size,26) ;
      env.ost << ");\n" ;
      // .. and name them
      if ( size ) {
        env.ost << "    unsigned cmixIa;\n"
                   "    for(cmixIa=" << EmitExpr(size,2)
                << ";cmixIa--;)\n" ;
      }
      ForCascade fc(aloctype,env,size?4:2);
      env.NameDynamicThing(fc,"cmixHeap",
                           (size?"cmixSA[cmixIa]":"cmixSA[0]"),true,
                           GegenEnv::cmixGlobal,"static ",NULL);
      fc.close() ;
      if ( env.bt.Static(s->target()->type->ptr_next()) )
        env.ost << "    " << EmitDeref(s->target(),4)
                << "=cmixSA;\n  }\n" ;
      else
        env.ost << "    cmixStmt(" << enter
                << EmitDeref(s->target(),4) << " = &?" << exit
                << ",(*cmixSA)" << dotcmix(fc.t2) << ");\n" ;
    } else {
      // the contents are static
      env.ost << "  " << EmitDeref(s->target(),4) << " = ("
              << Unqualified(aloctype,"(*)") << ")cmixAllocStatic("
              << env.memo.IdNumber[realdecl]-1 << ",sizeof("
              << AbstractDecl(aloctype) << ")," ;
      if ( size )
        env.ost << EmitExpr(size,2);
      else
        env.ost << "1" ;
      env.ost << ",cmix_follow" << env.memo.MemoNumber[realdecl]
              << ");\n" ;
    }
    break ; }
  case C_Free:    // deallocation.
    // if the pointer is dynamic just residualize the release
    if ( env.bt.Dynamic(s->free_expr()->type) ) {
      env.ost << "  cmixStmt(" << enter << "free("
              << EmitExpr(s->free_expr(),2) << ')' << exit << ");\n" ;
    } else {
      // Do nothing. Static data is never freed, period.
    }
    break ;                       
  case C_Sequence:
    break ;
  }
}

bool
FunGegen::EmitGoto(C_BasicBlock *bb, bool emitit) const
{
  DONT_CALL_THIS
    return false ;
}

static void
SplitIntoSDlists(GegenEnv &env,C_Expr *e,BinOp op,
                 Plist<C_Expr> &slist,Plist<C_Expr> &dlist)
  // Input: "a @ ((b @ c) @ d", "@"
  // Output: a list of those of a,b,c,d that are static
  //         a list of those of a,b,c,d that are dynamic
{
  if ( e->tag == C_Binary && e->binary_op() == op ) {
    SplitIntoSDlists(env,e->binary_expr1(),op,slist,dlist);
    SplitIntoSDlists(env,e->binary_expr2(),op,slist,dlist);
  } else if ( env.bt.Static(e->type) )
    slist.push_back(e);
  else
    dlist.push_back(e);
}

bool
FunGegen::EmitJump(C_Jump *s) const
  // EmitJump returns true if emitting a static jump, false if
  // an explicit jump back to the pending loop must be generated
{
  switch(s->tag) {
  case C_If:
    if ( env.bt.Static(s->cond_expr()->type) ) {
      bool handled ;
      env.ost << "  if( " << EmitExpr(s->cond_expr()) << " ) " ;
      handled = EmitGoto(s->cond_then(),true);
      env.ost << "\telse " ;
      handled = EmitGoto(s->cond_else(),true) && handled ;
      return handled ;
    } else {
      assert( use_pendinglist );

      // try to jump on static components of the expression
      Plist<C_Expr> dlist(s->cond_expr()) ;
      BinOp bop ;
      while ( dlist.size() == 1 && dlist.front()->tag == C_Binary ) {
        C_Expr *e = dlist.front() ;
        C_BasicBlock *jumpto = NULL ;
        char const *memfront ;
        int memprec ;
        switch(bop=e->binary_op()) {
          case And: jumpto=s->cond_else(); memfront="!"; memprec=27; break;
          case Or:  jumpto=s->cond_then(); memfront="" ; memprec=8 ; break;
          default: break ;
        }
        if ( jumpto == NULL ) break ;
        Plist<C_Expr> slist ;
        dlist.clear() ;
        SplitIntoSDlists(env,e,bop,slist,dlist);
        assert(!dlist.empty());
        if ( slist.empty() ) break ;
        env.ost << "  if( " ;
        bool first = true ;
        foreach(i,slist,Plist<C_Expr>) {
          if ( !first ) env.ost << " || " ;
          first = false ;
          env.ost << memfront << EmitExpr(*i,memprec) ;
        }
        env.ost << " ) " ;
        EmitGoto(jumpto,true);
        env.ost << "  else\n" ;
      }
          
      env.ost << "  cmixIf(" ;
      EmitGoto(s->cond_then(),false);
      env.ost << ','  ;
      EmitGoto(s->cond_else(),false);
      env.ost << ",\n\t" << enter ;
      if ( dlist.size() == 1 )
        bop = BOr ; // the binary operator with lowest priority
      bool first = true ;
      foreach(i,dlist,Plist<C_Expr>) {
        if ( !first ) env.ost << ' ' << binary2str(bop) << ' ' ;
        first = false ;
        env.ost << EmitExpr(*i,binary2prec(bop)) ;
      }
      env.ost << exit << ");\n" ;
      return false ;
    }
  case C_Goto:
    env.ost << "  " ;
    return EmitGoto(s->goto_target(),true);
  case C_Return:
    if ( use_cmixSR ) {
      if ( retmode == STATIC ) {
        env.ost << "  cmixSR = " << EmitExpr(s->return_expr(),4)
                << ";\n" ;
      } else if ( cmixSRisExp ) {
        env.ost << "  cmixSR = cmixMkExp(" << enter
                << EmitExpr(s->return_expr(),4) << exit << ");\n";
      } else {
        env.ost << "  cmixStmt(" << enter << userhole << " = "
                << EmitExpr(s->return_expr(),3)
                << exit << ",cmixSR" << exit << ");\n" ;
      }
    }
    if ( unfolding ) {
      if ( use_pendinglist )
        env.ost << "  cmixGoto(cmixEndLabel);\n" ;
    } else {
      env.ost << "  cmixReturn(" ;
      if ( retmode == DYNAMIC )
        env.ost << enter << EmitExpr(s->return_expr()) << exit ;
      else
        env.ost << "\"\"" ;
      env.ost << ");\n" ;
    }

    if ( !use_pendinglist ) {
      // XXX gcc2.8.1 on the HPs sometimes produces incorrect
      // code when the returns are too early. DRAT that compiler!
#if 0
      EmitReturnSequence(s.hasExpr() ? &s.return_expr() : NULL);
#else
      env.ost << "  goto cmix_return;\n" ;
#endif
      return true ;
    }
    return false ;
  }
  assert(0);
  return false ;
}

void
FunGegen::EmitReturnSequence(C_Expr *e) const
{
  env.ost << "  cmixPopLocals(" << DOcounter << ");\n";
  if ( !unfolding ) {
    if ( !env.bt.Dynamic(fun) )
      env.memo.emit_endstate(fun) ;
    env.ost << "  cmixPopFun();\n" ;
    if ( retmode == DYNAMIC ) {
      env.ost << "  return cmixMkExp(" ;
      ConstructResidualCall();
      env.ost << ");\n" ;
      return ;
    }
    env.ost << "  cmixStmt(";
    ConstructResidualCall();
    env.ost << ");\n" ;
  }
  env.ost << "  return " ;
  if ( use_cmixSR )
    env.ost << "cmixSR";
  else switch(retmode) {
  case STATIC:
    assert(e);
    env.ost << EmitExpr(e);
    break ;
  case VOID:
    break ;
  case DYNAMIC:
    assert(e);
    env.ost << "cmixMkExp(" << enter << EmitExpr(e,4) << exit <<')';
    break ;
  }
  env.ost << ";\n" ;
}

void
FunGegen::EmitBasicBlock(C_BasicBlock *bb) const
{
  if ( speclabels[bb] == 0 )
    env.ost << "cmixL" << bb->Numbered_ID << ":\n" ;
  else
    env.ost << "case " << speclabels[bb] << ": /* cmixL"
            << bb->Numbered_ID << ": */\n" ;
  foreach(stmt,bb->getBlock(),Plist<C_Stmt>)
    EmitStatement(*stmt);
  if ( EmitJump(bb->exit()) )
    ;
  else
    env.ost << "  goto cmix_loop;\n" ;
}

void
FunGegen::ConstructResidualCall() const
{
  env.ost << "\"?(" ;
  bool first = true ;
  foreach(i,fun->fun_params(),Plist<C_Decl>)
    if ( env.bt.Dynamic(i->type) ) {
      if ( !first ) env.ost << ", " ;
      first = false ;
      env.ost << '?' ;
    }
  env.ost << ")\",cmix_fun" ;

  unsigned n = 1 ;
  foreach(ii,fun->fun_params(),Plist<C_Decl>) {
    if ( env.bt.Dynamic(ii->type) )
      env.ost << ",cmixRP" << n ;
    n++ ;
  }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class StdFunGegen : FunGegen {
  GegenLocalMemo localmemo ;
  Plist<C_BasicBlock> pendusers ;
  void penduser(C_BasicBlock *);
  
  virtual bool EmitGoto(C_BasicBlock *, bool emitit) const;
public:
  StdFunGegen(GegenEnv*,C_Decl *);
  void emit() ;
  virtual ~StdFunGegen() {}
};

void
StdFunGegen::penduser(C_BasicBlock *bb) {
  if ( speclabels[bb] == 0 ) {
    pendusers.push_back(bb) ;
    speclabels[bb] = pendusers.size() ;
  }
}
  
StdFunGegen::StdFunGegen(GegenEnv *e, C_Decl *f)
  : FunGegen(e,f), localmemo(e->memo,f)
{
  // we unfold exactly the functions that are marked
  // as unsharable.
  unfolding = env.sa.find(fun);
    
  // use the pending list for any BB that is the target of a dynamic
  // conditional
  // AND (makholm 1999-07-14) any BB that ends with a dynamic conditional,
  // unless it is not itself under dynamic control.
  foreach(bb,fun->blocks(),Plist<C_BasicBlock>) {
    C_Jump *j = bb->exit() ;
    if ( j->tag == C_If && env.bt.Dynamic(j->cond_expr()->type) ) {
      if ( env.bt.Dynamic(*bb) )
        penduser(*bb);
      penduser(j->cond_then());
      penduser(j->cond_else());
    }
  }
  use_pendinglist = !pendusers.empty();
    
    // if memo_unconditional_mode is set, use pending list for ALL
    // BBs that are under dynamic control
  if ( use_pendinglist && memo_unconditional_mode )
    foreach(bb,fun->blocks(),Plist<C_BasicBlock>)
      if ( env.bt.Dynamic(*bb) )
        penduser(*bb);
    
    // Bugger. With gcc 2.8.1 on the HPs, P_gen tends to be
    // incorrect and get a SIGSEGV when the return is in
    // the middle of a function with many gotos, so it seems
    // we *always* have to use cmix_SR for returning static
    // values.
#if 0
  use_cmixSR = use_pendinglist &&
    ( retmode==STATIC || retmode==DYNAMIC && unfolding );
#else
  use_cmixSR = retmode==STATIC || retmode==DYNAMIC && unfolding ;
#endif
  cmixSRisExp = use_cmixSR && retmode==DYNAMIC && !use_pendinglist ;
}

bool
StdFunGegen::EmitGoto(C_BasicBlock *bb, bool emitit) const
{
  unsigned speclabel = speclabels[bb] ;
  if ( speclabel != 0 ) {
    if ( emitit ) env.ost << "cmixGoto(" ;
    env.ost << "cmixPendinsert(" << speclabel << ")" ;
    if ( emitit ) env.ost << ");\n" ;
    return false ;
  } else {
    env.ost << "goto cmixL" << bb->Numbered_ID ;
    if ( emitit )
      env.ost << ";\n" ;
    return true ;
  }
}

void
StdFunGegen::emit()
{
  // function heading
  env.oksection("Generating functions") ;
  env.specializable_heading(fun,false) ;
  env.ost << "{\n" ;

  if ( !unfolding ) {
    env.ost <<
      "  Code cmix_fun;\n"
      "  static struct cmixFunPool cmix_funpool = {0};\n" ;
  }
    
  // local variables
  foreach(j,fun->fun_locals(),Plist<C_Decl>) {
    env.ost << "  " << *j ;
    if ( env.spectime(*j) && j->hasInit() )
      env.ost << '=' << j->init() ;
    env.ost << ";\n" ;
  }
  // parameters
  foreach(jj,fun->fun_params(),Plist<C_Decl>)
    if ( !env.spectime(*jj) )
      env.ost << "  " << *jj << ";\n" ;
    
  // static return value placeholders
  if ( use_cmixSR ) {
    if ( retmode == STATIC )
      env.ost << "  " << Unqualified(returntype,"cmixSR") ;
    else {
      env.ost << "  Code cmixSR" ;
      if ( !cmixSRisExp ) {
        env.ost << '=' << env.resnames.request(fun->get_name()) ;
      }
    }
    env.ost << ";\n" ;
  }
  // we need a label to jump to when unfolding a function with
  // a pending loop
  if ( unfolding && use_pendinglist )
    env.ost << "  cmixLabelTag cmixEndLabel = cmixMakeLabel();\n" ;

  DOcounter = localmemo.emit_data(!unfolding,use_pendinglist);

  if ( use_pendinglist ) {
    env.ost << "  cmixLocalMemoWhat cmix_blocks[] = { 0" ;
    foreach(bb,pendusers,Plist<C_BasicBlock>) {
      env.ost << ",\n    /* cmixL" << bb->Numbered_ID << " */ " ;
      localmemo.emit_memodata(*bb);
    }
    env.ost << "};\n" ;
  }
  
  localmemo.emit_code();
  
  if ( use_cmixSR && retmode == DYNAMIC && !cmixSRisExp )
    env.ost << "  cmixDeclare(cmixLocal,cmixSR," << enter
            << pushtype(returntype,constvol()) << userhole
            << poptype << exit << ",cmixSR" << exit << ");\n" ;
    
  if ( unfolding ) {
    // name, declare and transfer parameters
    unsigned n = 1 ;
    foreach(j,fun->fun_params(),Plist<C_Decl>) {
      if ( !env.spectime(*j) ) {
        env.NameDynamicObject(*j,"cmixLocal");
        env.ost << "  cmixStmt(\"? = ?\"," << env.names[*j]
                << dotcmix(j->type) << ",cmixRP" << n << ");\n" ;
      }
      n++ ;
    }
  } else {
    // not unfolding: emit function sharing and creation code
    env.memo.emit_funmemo(fun);
    env.ost << 
      "  if (cmix_fun) {\n"
      "    cmixPopLocals(" << DOcounter << ");\n" ;
    if ( retmode == DYNAMIC ) {
      env.ost << "    return cmixMkExp(" ;
      ConstructResidualCall();
      env.ost << ");\n" ;
    } else {
      env.ost << "    cmixStmt(";
      ConstructResidualCall();
      env.ost << "); return;\n" ;
    }
    env.ost << "  }\n" ;
        
        // We need to have the parameters named at this point because
        // the name request nodes go into the residual heading
    foreach(j,fun->fun_params(),Plist<C_Decl>)
      if ( !env.spectime(*j) )
        env.NameDynamicObject(*j,NULL);

        // now we can create the residual function itself
    env.ost << "  cmix_fun = " << env.resnames.request(fun->get_name())
            << ";\n  cmixFunHeading(cmix_fun," ;
    residual_heading(env,fun,"static ","cmix_fun");
    env.ost << ");\n" ;
  }

  // name/declare residual locals
  foreach(jjj,fun->fun_locals(),Plist<C_Decl>)
    if ( !env.spectime(*jjj) )
      env.NameDynamicObject(*jjj,"cmixLocal");
    
    // check that not too many functions are created
  if ( env.max_residual_versions[fun] )
    env.ost << "  { static unsigned cmixDebugCount = "
            << env.max_residual_versions[fun] << ";\n"
      "  if ( cmixDebugCount-- == 0 ) {\n"
      "    fprintf(stderr,\"too many instances of "
            << fun->get_name() << "(), abort.\\n\");\n"
      "    exit(117);\n"
      "  }}\n" ;
    
  if ( use_pendinglist ) {
    // pending loop control:
      
    // create a new pending list
    env.ost << "  cmixPushPend(cmix_locals,cmix_blocks,"
            << pendusers.size() << ");\n  " ;

    // if the first BB is not handled by the pending list, simply
    // jump to it. Else insert it properly into the pending list
    EmitGoto(fun->blocks().front(),false);
    env.ost << ";\n" 
      "cmix_loop: switch(cmixPending()) {\n"
      "  case 0:\n"
      "  cmixPopPend();\n" ;
    if ( unfolding )
      env.ost << "  cmixLabel(cmixEndLabel);\n" ;
    // GCC 2.8.1 has been known to fail on early returns...
#if 0
    EmitReturnSequence(NULL);
#else
    env.ost << "  goto cmix_return;\n" ;
#endif
  } else {
    // no pending loop - we simply continue with the first block
  }
    
  // code for each basic block
  foreach(bb,fun->blocks(),Plist<C_BasicBlock>)
    EmitBasicBlock(*bb);

  if ( use_pendinglist )
    env.ost << "  } /* end of pending blocks */\n" ;
    
  env.ost << "cmix_return:\n" ;
  EmitReturnSequence(NULL);
  env.ost << "}\n" ;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct EntryPointGegen : FunGegen {
  EntryPointGegen(GegenEnv*,C_Decl *);
  void emit() const;
  virtual ~EntryPointGegen() {}
};

EntryPointGegen::EntryPointGegen(GegenEnv *e, C_Decl *f)
  : FunGegen(e,f)
{
  unfolding = false ;
  use_cmixSR = false ;
  cmixSRisExp = false ;
  use_pendinglist = true ;
  DOcounter = 0 ;
}

void
EntryPointGegen::emit() const
{
  static char const cmixMain[]="cmixMain";

  const char *generator_name = env.names[fun] ;
  if ( strcmp(generator_name,"main") == 0 )
    generator_name = cmixMain ;

  env.oksection("User-callable entry points")
    << "void\n" << generator_name << '(' ;
  unsigned statparams = 0 ;
  Plist<C_Decl>::iterator paramit = fun->fun_params().begin() ;
  for ( bool first = true ;
        paramit && paramit->varmode() == VarVisSpectime ; paramit++ ) {
    if ( !first )
      env.ost << ',' ;
    first = false ;
    statparams++ ;
    env.ost << *paramit ;
  }
  env.ost << ")\n{\n"
    "  static cmixDataObject cmix_params[1] = {{0}};\n"
    "  Code cmix_fun;\n" ;
  foreach( v, fun->fun_locals(), Plist<C_Decl> )
    env.ost << "  " << *v << ";\n" ;
  for ( Plist<C_Decl>::iterator rp=paramit ; rp ; rp++ )
    env.ost << "  " << *rp << ";\n" ;
  C_BasicBlock *bb = fun->blocks().front() ;
  Plist<C_Stmt>::iterator s = bb->getBlock().begin() ;
  assert(s->tag == C_Call);
  if ( s->call_args().size() != 1 ) {
    env.ost << "  char *cmixFunname = malloc(80); /* fixed limit! */\n"
      "  sprintf(cmixFunname" ;
    foreach(e,s->call_args(),Plist<C_Expr>)
      env.ost << ',' << EmitExpr(*e,2);
    env.ost << ");\n";
  } else {
    env.ost << "  static const char cmixFunname[] = "
            << s->call_args().front()->cnst() << ";\n" ;
  }

  // parameters for the residual function
  for ( Plist<C_Decl>::iterator rp2=paramit ; rp2 ; rp2++ )
    env.NameDynamicObject(*rp2,NULL);

  env.ost <<
    "  cmixPushFun();\n"
    "  cmix_fun = cmixMkExp(cmixFunname);\n"
    "  cmixFunHeading(cmix_fun," ;
  residual_heading(env,fun,"","cmix_fun");
  env.ost << ");\n" ;

  // local variable in the residual function
  foreach( vv, fun->fun_locals(), Plist<C_Decl> )
    env.NameDynamicObject(*vv,"cmixLocal");

  s++ ;
        
  // call the actual generator function
  EmitStatement(*s);
  s++ ;
  assert (!s) ;

  // emit a return statement and end the function
  EmitJump(bb->exit());
  env.ost <<
    "  cmixPopFun();\n"
    "}\n" ;

  if ( generator_name != cmixMain )
    return ;

  env.ost << "int\nmain(int argc,char *argv[])\n{\n"
    "  extern int cmixRestruct;\n"
    "  for ( ; argc > " << statparams+1 << "; argc--, argv++ ) {\n"
    "    if ( argv[1][0] != '-' ) break ;\n"
    "    if ( argv[1][1] == 'R' ) {\n"
    "      cmixRestruct = 0 ;\n"
    "    } else break ;\n"
    "  }\n"
    "  if ( argc != " << statparams+1 << ") {\n"
    "    fprintf(stderr,\"Expected " << statparams
          << " parameters\\n\");\n"
    "    return 1 ;\n"
    "  }\n"
    "  cmixGenInit();\n"
    "  cmixMain(" ;
  paramit = fun->fun_params().begin();
  for(unsigned i=1; i<=statparams; i++, paramit++) {
    if ( i > 1 ) env.ost << ',' ;
    switch(paramit->type->tag) {
    case Primitive: {
      static char const*const cvtletters[]
        = { "cchar", "ffloat", "fdouble", NULL } ;
      char cvtletter = 'i' ;
      char const *cp = paramit->type->primitive() ;
      while ( strchr(cp,' ') != NULL )
        cp = strchr(cp,' ')+1 ;
      for ( char const* const* cpp = cvtletters ; *cpp ; cpp++ )
        if ( strcmp(cp,*cpp+1) == 0 )
          cvtletter = **cpp ;
      env.ost << "cmix_ato" << cvtletter << "(argv[" << i << "])" ;
      break; }
    case Pointer:
      if ( paramit->type->ptr_next()->tag == Primitive &&
           ( paramit->type->ptr_next()->basetype() == Char ||
             paramit->type->ptr_next()->basetype() == UChar ||
             paramit->type->ptr_next()->basetype() == SChar ) ) {
        env.ost << "argv[" << i << ']' ;
        break ;
      }
      // else fall through
    default:
      Diagnostic d(ERROR,fun->pos);
      d << "argument #" << i << " to the generator" ;
      d.addline() << "cannot be parsed from the command line" ;
      d.addline() << "its type is " ;
      Pset<C_UserDef> dummy ;
      GegenStream gs(d,env.bt,env.names,dummy);
      gs << AbstractDecl(paramit->type) ;
      break ;
    }
  }
  env.ost << ");\n"
    "  cmixGenExit(stdout);\n"
    "  return 0;\n"
    "}\n" ;
}

void
GegenEnv::define_functions()
{
  foreach(fun,pgm.functions,Plist<C_Decl>)
    StdFunGegen(this,*fun).emit();
  foreach(fun2,pgm.generators,Plist<C_Decl>)
    EntryPointGegen(this,*fun2).emit() ;
}

