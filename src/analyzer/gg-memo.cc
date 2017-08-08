/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix gegen subsystem: memoization-related stuff
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "Nset.h"
#include "gegen.h"
#include "options.h"

void
GegenMemo::EmitMemoizer(const char *name1,const char *name2)
{
  ForCascade fc(MemoBaseType,up,3);
  if ( MemoUserType ) {
    up.ost << "if (!cmix_follow" << UserMemo[MemoUserType]
           << "(cmixCallback,cmixCopy," ;
    if ( MemoUserType != fc.t2->user_def() )
      up.ost << "(struct " << up.names[MemoUserType] << "*)" ;
    up.ost << '&' ;
  } else {
    up.ost << "if (!cmixCallback(cmixCopy," ;
  }
  up.ost << name1 << name2 << fc << ")) return 0;\n" ;
  fc.close() ;
}

bool
GegenMemo::AnyPointersToFollow(C_Type *t)
{
  MemoBaseType = t ;
    
  C_Type *rt = t->array_contents_type() ;
  switch(rt->tag) {
  default:
    return false ;
  case StructUnion:
    MemoUserType = rt->user_def() ;
    while ( MemoUserType->isUnion ) {
      C_Type *fmt
        = MemoUserType->instances.front()->user_types().front();
      if ( fmt->tag != StructUnion )
        return false ;
      MemoUserType = fmt->user_def() ;
    }
    return UserMemo[MemoUserType] != 0 ;
  case Pointer:
    MemoUserType = NULL ;
    return up.spectime(rt->ptr_next()) ;
  }
}

class CompareBindingTimes : public TypeCompareCallback {
  BTresult const &bt;
public:
  CompareBindingTimes(BTresult const &b) : bt(b) {}
  virtual bool deem_different(C_Type const&a,C_Type const&b) const {
    return bt.Dynamic(a) != bt.Dynamic(b);
  }
};

void
GegenMemo::AlocMemodata(C_Decl *d)
{
  if ( !up.spectime(d) )
    return ;
  // XXX if no functions or basic blocks ever need to memoise
  // the object we can bail out now - but we don't because that
  // information is not yet available.

  IdNumber[d] = ++IdCount ;

  if ( !AnyPointersToFollow(d->type) )
    return ;
  // Try to find an object whose type is the same as this one's.
  foreach(prev,memofun_leaders,Plist<C_Decl>)
    if ( d->type->equal(*prev->type,CompareBindingTimes(up.bt))) {
      MemoNumber[d] = MemoNumber[*prev] ;
      return ;
    }

  // This binding-time type has not been seen before. Create
  // a new pointer following function
  MemoNumber[d] = ++fcncounter ;
  memofun_leaders.push_back(d);
  up.oksection("Memoization helper functions")
    << "static int cmix_follow" << fcncounter << "\n"
    "    (cmixPointerCallback cmixCallback, "
    "cmixDataObjectCopy **cmixCopy, void *cmixVoid, "
    "unsigned cmixI) {\n  "
    << EmitType(d->type,"(*const cmixSrc)",d->qualifiers)
    << " = (" << EmitType(d->type,"(*)",d->qualifiers) << ")cmixVoid ;\n"
    "  while ( cmixI-- )\n" ;
  EmitMemoizer("cmixSrc[cmixI]","") ;
  up.ost << "  return 1;\n"
    "}\n" ;
}

void
GegenMemo::follow_functions()
{
  // Helper functions for static struct type
  // .. only emitted when there's actually any pointers to follow
  // .. their numbers are recorded in the UserMemo array
  foreach(u,up.pgm.usertypes,Plist<C_UserDef>) {
    if ( u->isUnion || up.bt.Dynamic(u->instances.front()) )
      continue ;
    bool begun = false ;
    foreach(j,**u,C_UserDef) {
      if ( !AnyPointersToFollow(j.type()) )
        continue ;
      if ( !begun ) {
        begun = true ;
        UserMemo[*u] = ++fcncounter ;
        up.oksection("Memoization helper functions")
          << "static int\ncmix_follow" << fcncounter
          << "(cmixPointerCallback cmixCallback, "
          "cmixDataObjectCopy **cmixCopy, struct "
          << up.names[*u] << " *cmixSrc)\n{\n" ;
      }
      EmitMemoizer("cmixSrc->",up.names[j.name()]) ;
    }
    if ( begun )
      up.ost << "   return 1;\n}\n" ;
  }
    
  // Make ID numbers and memoization helper functions
  foreach(v,up.pgm.globals,Plist<C_Decl>)
    // Spectime global vars are memoized unless they are
    // - VarExtSpectime (hence the 'dangerous spectime:' syntax
    //   for declaring them), or
    // - never side-effected at all (constant tables etc.)
    //   (in which case they can only point to other global
    //   variables, so there is no need to follow pointers
    //   in them either).
    if ( v->varmode() != VarExtSpectime &&
         up.pa.sideEffectedObjects.find(*v) )
      AlocMemodata(*v);
  foreach(vv,up.pgm.heap,Plist<C_Decl>) {
    assert(vv->type->tag==Array);
    AlocMemodata(vv->members().front());
  }
  foreach(f,up.pgm.functions,Plist<C_Decl>) {
    foreach(p,f->fun_params(),Plist<C_Decl>)
      AlocMemodata(*p);
    foreach(v,f->fun_locals(),Plist<C_Decl>)
      AlocMemodata(*v);
  }

  // Clear the temporary data structures that were used to implement
  // sharing of functions for like types.
  memofun_leaders.clear();
  UserMemo.clear();
}

void
GegenMemo::InitDO(C_Decl *d,bool local)
{
  if ( IdNumber[d] ) {
    up.ost << "\n\t{" << IdNumber[d]-1 << ',' ;
    if ( local )
      up.ost << '0' ;
    else
      up.ost << '&' << up.names[d] ;
    up.ost << ",sizeof " << up.names[d]
           << ", cmix_follow" << MemoNumber[d] << "}," ;
  }
}

void
GegenMemo::emit_globalmemodata()
{
  up.ost << "static cmixDataObject cmix_globals[] = {" ;
  foreach(i,up.pgm.globals,Plist<C_Decl>)
    InitDO(*i,false);
  up.ost << " {0} };\n" ;
}

void
GegenMemo::emit_globalmemocode()
{
  up.ost << "  cmixPushGlobals(cmix_globals);\n" ;
}

static C_Decl*
outerMostContainer(C_Decl* d)
{
  while ( d->isContained() ) {
    C_Decl* container = d->containedIn();
    if ( container->type->tag == Function ) break;
    d = container;
  }
  return d;
}

void
GegenMemo::emit_memowhat(Pset<C_Decl> const &setA,Pset<C_Decl> const &setB,
                         int leftinline)
{
  unsigned bytes = (IdCount+7)/8 ;
  unsigned char *tempset = new unsigned char[bytes] ;
  memset(tempset,0,bytes);

  foreach(i,setA,Pset<C_Decl>) {
    unsigned u = IdNumber[outerMostContainer(*i)] ;
    if ( u-- == 0 ) continue;
    tempset[u/8] |= (1 << (u&7)) ;
  }
  foreach(i,setB,Pset<C_Decl>) {
    unsigned u = IdNumber[outerMostContainer(*i)] ;
    if ( u-- == 0 ) continue;
    tempset[u/8] |= (1 << (u&7)) ;
  }

  up.ost << '\"' ;
  for(unsigned i=0;i < bytes; i++) {
    unsigned data = tempset[i] ;
    if( i == bytes-1 && data == 0 )
      break ;
    if( leftinline < 0 ) {
      up.ost << "\"\n\t\"" ;
      leftinline = 65 ;
    }
    up.ost << '\\' ;
    if( data >= 010 ) {
      if( data >= 0100 )
        up.ost << (char)('0'+(7&(data>>6))) ; leftinline-- ;
      up.ost << (char)('0'+(7&(data>>3))) ; leftinline-- ;
    }
    up.ost << (char)('0'+(7&data)) ;
    leftinline -= 2;
  }
  up.ost << '\"' ;

  delete tempset ;
}
  
void
GegenMemo::emit_funmemo(C_Decl *fun)
{
  up.ost << "  cmix_fun = cmixFindOrPushFun(&cmix_funpool,cmix_params," ;
  emit_memowhat(df.GetReadSet(fun),df.GetWriteSet(fun),15);
  up.ost << ");\n" ;
}

void
GegenMemo::emit_endstate(C_Decl *fun)
{
  up.ost << "  cmixSaveExitState(cmix_locals," ;
  Pset<C_Decl> empty ;
  emit_memowhat(df.GetWriteSet(fun),empty,54);
  up.ost << ");\n" ;
}

GegenMemo::GegenMemo(GegenEnv &env,DFresult const &DF)
  : up(env), df(DF),
    fcncounter(0), UserMemo(0), IdNumber(0), IdCount(0), MemoNumber(0)
{}

/*****************************************************************************/

GegenLocalMemo::GegenLocalMemo(GegenMemo &u,C_Decl *f)
  : up(u), fun(*f), LocalPositions(0)
{}

unsigned
GegenLocalMemo::emit_data(bool funmemo, bool bbmemo)
{
  unsigned localcounter, totalcounter ;

  localcounter = 0 ;
  foreach(v,fun.fun_locals(),Plist<C_Decl>)
    if ( up.IdNumber[*v] ) {
      addresses.push_back(up.up.names[*v]) ;
      LocalPositions[*v] = ++localcounter ;
    }
  totalcounter = localcounter ;
  foreach(vv,fun.fun_params(),Plist<C_Decl>)
    if ( up.IdNumber[*vv] ) {
      addresses.push_back(up.up.names[*vv]) ;
      LocalPositions[*vv] = ++totalcounter ;
    }

  if ( totalcounter || bbmemo || funmemo ) {
    up.up.ost << "  cmixDataObject cmix_locals[] = {" ;
    foreach(v,fun.fun_locals(),Plist<C_Decl>)
      up.InitDO(*v,true);
    foreach(vv,fun.fun_params(),Plist<C_Decl>)
      up.InitDO(*vv,true);
    up.up.ost << " {0} };\n" ;
  }

  if ( funmemo )
    up.up.ost << "  cmixDataObject * const cmix_params = cmix_locals + "
              << localcounter << ";\n" ;

  return totalcounter ;
}

void
GegenLocalMemo::emit_code()
{
  unsigned u = 0 ;
  foreach(n,addresses,Plist<char const>)
    up.up.ost << "  cmix_locals[" << u++ << "].obj = &" << *n << ";\n" ;
  if ( !addresses.empty() )
    up.up.ost << "  cmixPushLocals(cmix_locals);\n" ;
}

void
GegenLocalMemo::emit_memodata(C_BasicBlock *bb) const
{
  Pset<C_Decl>const &memoset = up.df.GetMemoSet(bb);
  Nset numberset ;
  foreach(i,memoset,Pset<C_Decl>) {
    numberset.insert(LocalPositions[outerMostContainer(*i)]);
  }
  up.up.ost << '\"' ;
  foreach(n,numberset,Nset) {
    unsigned data = *n ;
    if( data == 0 ) continue ;
    while( data >= 255 ) {
      up.up.ost << "\\xFF" ;
      data -= 255 ;
    }
    up.up.ost << '\\' ;
    if( data >= 010 ) {
      if( data >= 0100 )
        up.up.ost << (char)('0'+(7&(data>>6))) ;
      up.up.ost << (char)('0'+(7&(data>>3))) ;
    }
    up.up.ost << (char)('0'+(7&data)) ;
  }
  up.up.ost << '\"' ;
}
