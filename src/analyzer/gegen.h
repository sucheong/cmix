/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Header file for Gegen subsystem
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __GEGEN__
#define __GEGEN__

#include "analyses.h"
#include "renamer.h"

class GegenEnv ;
class GegenStream ;

/////////////////////////////////////////////////////////////////////////////
//
//  F O R   C A S C A D E S
//                                                             gg-cascades.cc

class ForCascade {
  unsigned depth ;
  unsigned indent ;
  unsigned nbraces ;
  C_Type *t1 ;
  GegenStream &ost ;
public:
  C_Type *t2 ;
  constvol cv ;
  ForCascade(C_Type *t,GegenEnv &,unsigned i = 2);
  GegenStream &addline();
  friend GegenStream &operator<<(GegenStream &,ForCascade const&) ;
  void close();
  ~ForCascade();
};

/////////////////////////////////////////////////////////////////////////////
//
//  M E M O I Z A T I O N   C O N T R O L
//                                                                 gg-memo.cc

class GegenMemo {
  friend class GegenLocalMemo ;
  GegenEnv &up ;
  DFresult const &df ;
  
  unsigned fcncounter ;
  array<C_UserDef,unsigned> UserMemo ;
    
  // these members communicate between AnyPointersToFollow and EmitMemoizer
  C_Type *MemoBaseType ;
  C_UserDef *MemoUserType ;
  void EmitMemoizer(char const *,char const*);
  bool AnyPointersToFollow(C_Type *t);

  Plist<C_Decl> memofun_leaders ;
  void AlocMemodata(C_Decl *d);

  void emit_memowhat(Pset<C_Decl> const&,Pset<C_Decl> const&,int);
  
  void InitDO(C_Decl *,bool local);
public:
  array<C_Decl,unsigned> IdNumber ;
  unsigned IdCount ;
  array<C_Decl,unsigned> MemoNumber ;
  GegenMemo(GegenEnv&,DFresult const &);
  void emit_globalmemodata();
  void emit_globalmemocode();
  void emit_funmemo(C_Decl*);
  void emit_endstate(C_Decl*);

  void follow_functions() ;
};

class GegenLocalMemo {
  GegenMemo &up ;
  C_Decl &fun ;
  array<C_Decl,unsigned> LocalPositions ;
  Plist<char const> addresses ;
public:
  GegenLocalMemo(GegenMemo &,C_Decl *);
  unsigned emit_data(bool funmemo,bool bbmemo);
  // emit_data returns the number to be passed to cmixPopObjects() ;
  void emit_code();
  void emit_memodata(C_BasicBlock *) const;
};

/////////////////////////////////////////////////////////////////////////////
//
//  T Y P E   A N D   E X P R E S S I O N   R E N D E R I N G
//                                                                 gg-expr.cc

struct EmitExpr {
  C_Expr *expr ;
  int prec ;
  EmitExpr(C_Expr *e,int p=0) : expr(e), prec(p) {}
};
struct EmitDeref {
  C_Expr *expr ;
  int prec ;
  EmitDeref(C_Expr *e,int p=0) : expr(e), prec(p) {}
};
struct EmitType {
  C_Type *type ;
  char const *middle ;
  constvol cv ;
  EmitType(C_Type *t,char const *m,constvol q): type(t), middle(m), cv(q) {}
};
#define pushtype(t,cv) EmitType(t,NULL,cv)
#define AbstractDecl(t) EmitType(t,"",constvol())
#define Unqualified(t,mt) EmitType(t,mt,constvol())

class GegenStream {
  ostream &ost ;
  BTresult const &bt ;
  NameMap const &names ;
  Pset<C_UserDef> &residual_structs ;

  friend void GegenMemo::emit_funmemo(C_Decl*);
    
  bool pgenMode ;
  bool exit2pending ;
  bool nullprotect ;
  Plist<C_Expr> interpolate ;
  int interpolate1 ;
  Plist<C_Type> waitingtypes ;

  bool alwayscode ;
    
  enum Direction { Prefix, Postfix };
  void emitcv(constvol,C_Type*);
  void typerec(C_Type*,Direction,bool prec);
  void std_hole(C_Type*,int);
  void fill_hole(C_Expr*);
  void userhole() ;
public:
  GegenStream(ostream&,BTresult const&,NameMap const&,Pset<C_UserDef>&);
  GegenStream &operator<<(char const *s)   { ost << s; return *this; }
  GegenStream &operator<<(char c)          { ost << c; return *this; }
  GegenStream &operator<<(unsigned u)      { ost << u; return *this; }
  GegenStream &operator<<(unsigned long u) { ost << u; return *this; }
  GegenStream &operator<<(void(*f)(GegenStream*)) { f(this); return *this; }
  GegenStream &operator<<(EmitExpr);
  GegenStream &operator<<(EmitDeref); 
  GegenStream &operator<<(EmitType);
  GegenStream &operator<<(C_Init*);
  GegenStream &operator<<(C_Decl*); // outputs basic declaration
  ~GegenStream() ;

  char const* EmitLiftingHole(C_Type *t);
  // returns a conversion function (with beginning parenthesis) for use
  // in the exit phase
    
  friend void enter(GegenStream*) ;
  friend void userhole(GegenStream*) ;
  friend void exit(GegenStream*) ;
  friend void alwayscode(GegenStream*) ;
  friend void poptype(GegenStream*) ;
};

/////////////////////////////////////////////////////////////////////////////
//
//  G E N E R A T I N G   F U N C T I O N   C O N S T R U C T I O N
//                                                                 gg-code.cc

// defines GegenEnv::define_functions()

/////////////////////////////////////////////////////////////////////////////
//
//  S T R U C T / U N I O N   H A N D L I N G
//                                                               gg-struct.cc

char const* dotcmix(C_Type *t);
// defines GegenEnv::putnameseq

// defines GegenEnv::pgen_usertype_fwds
// defined GegenEnv::pgen_usertype_decls
// defines GegenEnv::define_cmixMember
// defines GegenEnv::define_cmixPutNameX

// defines GegenEnv::init_struct
// defines GegenEnv::exit_struct

/////////////////////////////////////////////////////////////////////////////
//
//  D E C L A R A T I O N S   (static or dynamic)
//                                                                 gg-decl.cc

// defines GegenEnv::NameDynamicThing(bloodylongparameterlist)
// defines GegenEnv::NameDynamicObject(likewise)
// defines GegenEnv::define_pgen_globals()

/////////////////////////////////////////////////////////////////////////////
//
//  E N V I R O N M E N T   &   O T H E R   M I S C .   S T U F F
//

struct GegenEnv {
  GegenStream ost ;
  ResNameMgr resnames ;
  
  struct multichar {
    char ch ;
    int count ;
    multichar(char a,int b): ch(a), count(b) {}
  };    
private:
  char const *cursection ;
public:
  GegenStream &oksection(char const*) ;
  
  C_Pgm const &pgm ;
  BTresult const &bt ;
  PAresult const &pa ;
  NameMap const &names ;
  
  bool spectime(C_Type *);
  bool spectime(C_Decl *); // returns whether the object exists in pgen only
  
  Pset<C_UserDef> &residual_structs ;
  array<C_UserDef,unsigned> PutName ;
  unsigned putnameseq(C_Type*);
  
  GegenMemo memo ;
  
  static char const cmixGlobal[];
  static char const cmixGinit[];
  bool NameDynamicThing(ForCascade&, char const *resname,char const *genname,
                        bool manage_name,char const *registry,
                        char const *linkage,C_Init *);
  void NameDynamicObject(C_Decl *d,char const *registry);
  
  SAresult const &sa ;
  void specializable_heading(C_Decl *,bool proto); // in gg-code.cc
  array<C_Decl,char const*> max_residual_versions ;
  Plist<char const> res_taboos ;
  
  void emit_header();
  void pgen_usertype_fwds() ;  // in gg-struct.cc
  void pgen_usertype_decls() ; // in gg-struct.cc
  void define_cmixMember() ;   // in gg-struct.cc
  void define_cmixPutName() ;  // in gg-struct.cc
  void define_pgen_globals() ; // in gg-decl.cc
  void emit_prototypes() ;
  void define_functions() ;    // in gg-code.cc
  void define_initfun();
  void define_exitfun();
  
  void init_struct();          // in gg-struct.cc
  void exit_struct();          // in gg-struct.cc
  
  GegenEnv(ostream&,C_Pgm const &PGM,
           BTresult const &BT, PAresult const &PA,
           SAresult const &SA, DFresult const &DF,
           NameMap const &NAMES, Pset<C_UserDef> &RESSTRUCTS);
};

GegenStream &operator <<(GegenStream &,GegenEnv::multichar);

#endif
