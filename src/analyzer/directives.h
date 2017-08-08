/* -*-c++-*-
 * Author:   Henning Makholm (makholm@diku.dk)
 *           Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Wrapper classes for directive parser & lexer.
 *
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */
  
#ifndef __DIRECTIVES__
#define __DIRECTIVES__

#include <stdio.h>
#include <stdarg.h>
#include <iostream.h>
#include "Plist.h"
#include "diagnostic.h"
#include "cpgm.h"

void JamInDirective(const char* directive,Position pos);

struct SourceDirective {
    const char*const filename ;
    const Position pos ;
    SourceDirective ();
    SourceDirective (const char*fn,Position p) : filename(fn), pos(p) {}

    void Read(const char* cppargs);
} ;

struct SpecfDirective {
    char const *subject_name ;
    FunDef* subject_def ;
    Position pos ;
    SpecfDirective(char const *n,Position p)
        : subject_name(n), subject_def(NULL), pos(p) {}
    virtual void finalize();
} ;

struct GeneratorDirective : SpecfDirective {
    enum ParamTag { Residual, SpectimeArg, ConstInteger, ConstString };
    struct Param {
        ParamTag tag ;
        unsigned argpos ;
        const char *constval ;
        C_Decl *matching ;
        Param(ParamTag t,int i): tag(t), argpos(i) {
            assert( t==SpectimeArg );
        }
        Param(ParamTag t,const char *s): tag(t), constval(s) {
            assert( t==ConstString || t==ConstInteger );
        }
        Param(ParamTag t): tag(t) {
            assert( t==Residual );
        }
    private:
        Param() { DONT_CALL_THIS }
    };
    
    const char* generator_name ;     // e.g. "foo_gen"
    Plist<Param>* params ;
    const char* residual_name_base ; // e.g. "\"foo_res%s\""
    Plist<Param>* residual_name_parts ;
    GeneratorDirective(char const *n,Position p):
        SpecfDirective(n,p), generator_name(NULL),
        params(NULL), residual_name_base(NULL), residual_name_parts(NULL) {}
    virtual void finalize();
    void g2core(C_Pgm&);
};

struct DebugDirective : SpecfDirective {
    char const *limit ;
    DebugDirective(char const *n, char const *l, Position p)
        : SpecfDirective(n,p), limit(l) {}
};

class C_Decl ;

struct AnnoAtom {
  Position const pos ;
  AnnoAtom(Position p): pos(p) {}
  virtual void ApplyFun(FunDef *) { complain("functions"); }
  virtual void ApplyCore(C_Decl *) { complain("functions"); }
  virtual void ApplyVar(VarDecl *) { complain("variables"); }
  virtual void show(ostream&) =0 ;
private:
  void complain(char const *towhat);
};

struct StateAnnoAtom : AnnoAtom {
  enum SideEffects state ;
  StateAnnoAtom(Position p,SideEffects s): AnnoAtom(p), state(s) {}
  virtual void ApplyFun(FunDef *);
  virtual void ApplyCore(C_Decl *);
  virtual void show(ostream&);
};

struct CallAnnoAtom : virtual AnnoAtom {
  enum CallTime time ;
  CallAnnoAtom(Position p,CallTime c): AnnoAtom(p), time(c) {}
  virtual void ApplyFun(FunDef *);
  virtual void ApplyCore(C_Decl *);
  virtual void show(ostream&);
};

struct VarAnnoAtom : virtual AnnoAtom {
  enum VariableMode mode ;
  VarAnnoAtom(Position p,VariableMode m): AnnoAtom(p), mode(m) {}
  virtual void ApplyVar(VarDecl *);
  virtual void show(ostream&);
};

struct TimeAnnoAtom : virtual AnnoAtom, CallAnnoAtom, VarAnnoAtom {
  TimeAnnoAtom(Position p,VariableMode m,CallTime c):
    AnnoAtom(p), CallAnnoAtom(p,c), VarAnnoAtom(p,m) {}
  virtual void show(ostream&);
};

struct DualAnnoAtom : AnnoAtom {
  AnnoAtom *a1, *a2 ;
  DualAnnoAtom(AnnoAtom *aa1,AnnoAtom *aa2)
    : AnnoAtom(aa1->pos), a1(aa1), a2(aa2) {}
  virtual void ApplyFun(FunDef *) ;
  virtual void ApplyCore(C_Decl *) ;
  virtual void show(ostream&);
};

extern class Directives {
    friend class AddDirective ;
    char *cppargs ;
    size_t cppargs_len ;
    Plist<SourceDirective> sources ;
public:
    Plist<GeneratorDirective> generators ;
    Plist<DebugDirective> debug_flags ;
    Plist<const char> headers ;
    Plist<const char> taboos ;
    Directives();
    AnnoAtom* AnnoFromString(const char *,Position pos);
    void add_source(const char*,Position);
    void add_cpparg(char key,const char*);
    void add_funanno(AnnoAtom*,const char*);
    void add_globalanno(AnnoAtom*,const char*);
    void add_localanno(AnnoAtom*,const char*,const char*);
    void add_generator(GeneratorDirective*);
    void add_debug(DebugDirective*);
    void add_wellknown(const char*);
    bool is_wellknown(const char*);
    void add_taboo(const char*);
    void ReadSubjectProgram();
} directives ;

ostream &operator<<(ostream&,VariableMode);
void ApplyUserAnnotations(CProgram *);

//--------------------------------------
// Flex-bison interface
//
// apart from what bison writes to the header file
//

extern struct CmxResult {
    enum { GotDirectives, GotLoneAnnotation } status ;
    AnnoAtom *lone_which ;
    CmxResult(): status(GotDirectives) {}
    CmxResult(AnnoAtom *s):
        status(GotLoneAnnotation), lone_which(s) {}
} TheCmxResult ;

extern LexerTracker cmxhere;
extern int cmx_yyerror_called ;
int cmxlex();

struct cmxLexWrapper {
    bool ok ;
    FILE* f ;
    static int InUse ;
    static void Entry(Position&);
public:
    cmxLexWrapper(const char *filename,Position ref);
    cmxLexWrapper(const char *buffer, size_t length, Position pos);
    int parse();
    ~cmxLexWrapper();
};

#endif /* __DIRECTIVES__ */
