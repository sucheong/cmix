/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Directive parser wrappers
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cpgm.h"
#include "corec.h"
#include "directives.h"
#include "options.h"
#include "auxilary.h"
#include "parser.h"
#include "symboltable.h"


/* This module collects all user annaotations: Each user annotation is put
   attached to a name by a symbol table. When the C program has been
   constructed, it is traversed and for each name the list of user annotations
   are applied to the construct for that name.
*/

//-----------------------------------------------------
// class AnnoAtom & sons
//

void
AnnoAtom::complain(char const *towhat)
{
  Diagnostic di(ERROR,pos);
  show(di);
  di << " does not apply to " << towhat ;
}

void
StateAnnoAtom::ApplyFun(FunDef *f)
{
  if ( f->effects == NULL )
    f->effects = this ;
  else if ( f->effects->state != state ) {
    Diagnostic di(ERROR,pos);
    show(di);
    di << " specification for " << f->name << " conflicts with" ;
    di.addline(f->effects->pos) << " earlier ";
    f->effects->show(di);
    di << " specification" ;
  } else
    ; // redundant specification is OK.
}

void
StateAnnoAtom::ApplyCore(C_Decl *d)
{
  d->effects(this);
}

void
StateAnnoAtom::show(ostream &o)
{
  switch(state) {
  case SEPure: o << "pure" ; break ;
  case SEStateless: o << "stateless" ; break ;
  case SEROState: o << "rostate" ; break ;
  case SERWState: o << "rwstate" ; break ;
  default: o << '[' << (int)state << ']' ; break ;
  }
}

void
CallAnnoAtom::ApplyFun(FunDef *f)
{
  if ( f->calltime == NULL )
    f->calltime = this ;
  else if ( f->calltime->time != time ) {
    Diagnostic di(ERROR,pos);
    show(di);
    di << " specification for " << f->name << " conflicts with" ;
    di.addline(f->calltime->pos) << " earlier ";
    f->calltime->show(di);
    di << " specification" ;
  } else
    ; // redundant specification is OK.
}

void
CallAnnoAtom::ApplyCore(C_Decl *d)
{
  d->calltime(this);
}

void
CallAnnoAtom::show(ostream &o)
{
  switch(time) {
  case CTSpectime: o << "spectime" ; break ;
  case CTResidual: o << "residual" ; break ;
  case CTNoAnno: o << "anytime" ; break ;
  default: o << '[' << (int)time << ']' ; break ;
  }
}

void
VarAnnoAtom::ApplyVar(VarDecl *vr)
{
  VariableMode ourmode = mode ;
  VariableMode premode = vr->varmode ;
  bool isexternal = false ;
  switch(premode) {
  case VarExtResidual: premode = VarIntResidual ; isexternal = true ; break ;
  case VarExtDefault:  premode = VarIntAuto     ; isexternal = true ; break ;
  case VarExtSpectime:                          ; isexternal = true ; break ;
  default: break ;
  }
  if ( premode == ourmode )
    ; // redundant specification is OK.
  else if ( premode != VarIntAuto ) {
    Diagnostic di(ERROR,pos) ;
    di << ourmode << " specification for " << vr->name << " conflicts with" ;
    di.addline(vr->varmodeWhy) << "earlier " << vr->varmode
                               << " specification" ;
  } else {
    // do special checks
    bool OK = true ;
    switch(ourmode) {
    case VarIntResidual:
      if ( isexternal )
        ourmode = VarExtResidual ;
      break ;
    case VarIntSpectime:
      OK = !isexternal ;
      break ;
    case VarExtSpectime:
      OK = isexternal ;
      break ;
    case VarVisResidual:
    case VarVisSpectime:
      OK = !isexternal ;
      if ( OK && vr->linkage != External ) {
        Diagnostic d(ERROR,pos) ;
        d << ourmode << " only applies to variables with external linkage" ;
      }
      break ;
    case VarConstant:
      break ;
    case VarIntAuto:
    case VarExtResidual:
    case VarExtDefault:
    case VarEnum:
    case VarMu:
      assert(0);
    }
    if ( !OK ) {
      static bool DShelpgiven = true ;
      // the dangerous spectime advice is turned off for the
      // summer school; the manual section does not exist!
      Diagnostic d(ERROR,pos) ;
      d << ourmode << " does " ;
      if ( isexternal )
        d << "not" ;
      else
        d << "only" ;
      d << " apply to external variables" ;
      if ( ourmode == VarIntSpectime && !DShelpgiven ) {
        d.addline() << "If you REALLY want spectime here, read the" ;
        d.addline() << "manual section on `dangerous spectime'." ;
        DShelpgiven = true ;
      }
    } else {
      vr->varmode = ourmode ;
      vr->varmodeWhy = pos ;
    }
  }
}

void
VarAnnoAtom::show(ostream &ost)
{
  ost << mode ;
}

void
TimeAnnoAtom::show(ostream &ost)
{
  VarAnnoAtom::show(ost);
}

void
DualAnnoAtom::ApplyFun(FunDef *f)
{
  a1->ApplyFun(f);
  a2->ApplyFun(f);
}

void
DualAnnoAtom::ApplyCore(C_Decl *d)
{
  a1->ApplyCore(d);
  a2->ApplyCore(d);
}

void
DualAnnoAtom::show(ostream &ost)
{
  a1->show(ost);
  a2->show(ost << ' ');
}

//-----------------------------------------------------
// class UserAnno & sons
//

class EnBlocDeleteable {
  const EnBlocDeleteable *ebd_next ;
public:
  EnBlocDeleteable(EnBlocDeleteable *&root) : ebd_next(root) {
    root = this ;
  }
  virtual ~EnBlocDeleteable() {
    if ( ebd_next )
      delete ebd_next ;
  }
} ;

class UserAnno : EnBlocDeleteable {
protected:
  // The Hit class is used for consistency check: there should be exactly one
  // hit for each annotation, which is checked by the destructor.
  struct Hit {
    Position pos ;
    Hit *next ;
    Hit(Position p,Hit *n) : pos(p), next(n) {}
    ~Hit() { if ( next ) delete next; }
  } ;
private:
  char const *const thingtype ;
  Hit *hits ;
  void ApplyHere(Position apos) {
    hits = new Hit(apos,hits);
  }
  static EnBlocDeleteable *all_of_us ;
  static SymbolTable<UserAnno> symboltable ;
  UserAnno *next ;
  UserAnno(UserAnno&);
public:
  Position pos ;
  const char *name ;
protected:
  virtual void app_extfunction(FunDef *) = 0;
  virtual void app_specfunction(FunDef *) = 0;
  virtual void app_var(VarDecl *) = 0;
  UserAnno(const char *n,Position p,char const *tt) :
    EnBlocDeleteable(all_of_us), thingtype(tt),
    hits(NULL), pos(p), name(n) {
    // Attach this annotation to the right name.
    UserAnno *bigbrother = symboltable.lookup(n) ;
    if ( bigbrother ) {
      next = bigbrother->next ;
      bigbrother->next = this ;
    } else {
      next = NULL ;
      symboltable.insert(n,this);
    }
  }
  static UserAnno *Find(const char *n) { return symboltable.lookup(n); }
public:
  static void Apply(char const *name,FunDef *f) {
    // Run through all annotations for 'name' (which is function f).
    for( UserAnno *diver = Find(name) ; diver ; diver = diver->next ) {
      // Select external/internal function by whether f has statements.
      if(f->stmts)
        diver->app_specfunction(f);
      else
        diver->app_extfunction(f);
      // Record where the annotation has been used.
      diver->ApplyHere(f->pos);
    }
  }
  static void Apply(char const *name,VarDecl *vr) {
    for( UserAnno *diver = Find(name) ; diver ; diver = diver->next ) {
      diver->app_var(vr);
      diver->ApplyHere(vr->pos);
    }
  }
  virtual ~UserAnno() {
    if ( thingtype ) {
      if ( hits == NULL ) {
        Diagnostic(ERROR,pos) << thingtype << " `" << name
                              << "' not found" ;
        return ;
      }
      if ( hits->next != NULL ) {
        Diagnostic d(WARNING,pos) ;
        d << '`' << name << "' ambiguous:" ;
        for ( Hit *diver = hits ; diver ; diver = diver->next )
          d.addline(diver->pos) << "here is one candidate" ;
      }
    }
    delete hits ;
  }
  static void DeleteThemAll() {
    symboltable.clear();
    if ( all_of_us ) delete all_of_us ;
  }
} ;

SymbolTable<UserAnno> UserAnno::symboltable ;
EnBlocDeleteable *UserAnno::all_of_us = NULL ;

struct VarAnno : UserAnno {
protected:
  AnnoAtom *anno ;
  virtual void app_extfunction(FunDef*) {
    Diagnostic d(ERROR,pos) ;
    d << "Annotation for function" << name << " missing trailing '()'" ;
  }
  virtual void app_specfunction(FunDef* f) { app_extfunction(f); }
  virtual void app_var(VarDecl *vr) { anno->ApplyVar(vr); }
public:
  VarAnno(const char *n,AnnoAtom *an) :
    UserAnno(n,an->pos,"variable"), anno(an) {}
};

struct FunAnno : UserAnno {
protected:
  virtual void app_extfunction(FunDef*) = 0;
  virtual void app_specfunction(FunDef* f) = 0;
  virtual void app_var(VarDecl*) {
    Diagnostic d(ERROR,pos) ;
    d << "Variable " << name << " is not a function" ;
  }
public:
  FunAnno(const char *n, Position p) :
    UserAnno(n,p,"function") {}
} ;

struct ExtFunAnno : FunAnno {
protected:
  AnnoAtom *const atom ;
  virtual void app_extfunction(FunDef* f) { atom->ApplyFun(f); }
  virtual void app_specfunction(FunDef* f) {
    Diagnostic d(ERROR,pos) ;
    d << "You cannot specify conditions for calls to " << name ;
    d.addline(f->pos) << "because it has a body (and will be specialised)";
  }
public:
  ExtFunAnno(const char *n, AnnoAtom *ua) :
    FunAnno(n,ua->pos), atom(ua) {}
} ;

struct SpecfAnno : FunAnno {
protected:
  SpecfDirective* const owner ;
  virtual void app_specfunction(FunDef *f);
  virtual void app_extfunction(FunDef*) {
    Diagnostic(ERROR,pos) << name <<
      "() has no body and thus cannot be specialized";
  }
public:
  SpecfAnno(SpecfDirective *sd)
    : FunAnno(sd->subject_name,sd->pos), owner(sd) {}
};

//-----------------------------------------------------
// class Directives
//

Directives directives;

void SourceDirective::Read(const char* cppargs) {
  if ( quiet_mode <= 1 )
    cout << "Reading source file " << filename << endl ;
  cccLexWrapper(filename,cppargs,pos).parse();
  Diagnostic::EnterNewPhase();
}

Directives::Directives()
  : cppargs((char*)malloc(1)),
    cppargs_len(1),
    sources()
{
  cppargs[0] = '\0' ;
}

AnnoAtom* Directives::AnnoFromString(const char* str,Position pos)
{
  if ( !cmxLexWrapper(str,strlen(str),pos).parse() )
    return NULL ;
  if ( TheCmxResult.status != CmxResult::GotLoneAnnotation ) {
    Diagnostic(ERROR,pos) << '"' << str << "\" is not an annotation" ;
    return NULL ;
  }
  return TheCmxResult.lone_which ;
}

void Directives::add_source(const char *newfile,Position pos) {
  static SymbolTable<const char> all ;
  if( all.lookup(newfile) != NULL )
    Diagnostic(WARNING,pos) << "Extraneous specification of " << newfile
                            << " ignored" ;
  else {
    all.insert(newfile,"foo");
    sources.push_back(new SourceDirective(newfile,pos));
  }
}

void Directives::add_cpparg(char key,const char *cpparg) {
  size_t newlen = cppargs_len + 5 + strlen(cpparg);
    
  cppargs = (char*)realloc(cppargs,newlen);
  cppargs_len = newlen ;
  if (NULL==cppargs)
    Diagnostic(FATAL,Position()) << "Out of memory" ;
  static char beginstring[] = "'-x" ;
  beginstring[2] = key ;
  strcat(cppargs,beginstring);
  strcat(cppargs,cpparg);
  strcat(cppargs,"' ");
}

void
Directives::add_globalanno(AnnoAtom *ua,const char *name)
{
  (void)new VarAnno(name,ua);
}

void
Directives::add_localanno(AnnoAtom *ua,const char *name, const char *localname)
{
  char *compound_name = new char[strlen(name)+3+strlen(localname)];
  strcpy(compound_name,name);
  strcat(compound_name,"::");
  strcat(compound_name,localname);
  (void)new VarAnno(compound_name,ua);
}

void
Directives::add_funanno(AnnoAtom *ua,const char *name)
{
  (void)new ExtFunAnno( name, ua );
}

void
GeneratorDirective::finalize()
{
  Plist<GeneratorDirective::Param>::iterator diri = params->begin() ;
  Plist<VarDecl>::iterator ci = subject_def->decls->begin();

  while ( diri && ci ) {
    if ( (*diri)->tag == GeneratorDirective::Residual )
      switch( (*ci)->varmode ) {
      case VarIntAuto:
      case VarIntResidual:
        (*ci)->varmode = VarIntResidual ;
        (*ci)->varmodeWhy = pos ;
        break ;
      case VarIntSpectime:
        {
          Diagnostic d(ERROR,(*ci)->pos) ;
          d << "parameter " << (*ci)->name << " cannot be residual by" ;
          d.addline(pos) << "this generator directive, because" ;
          d.addline((*ci)->varmodeWhy) << "it is declared spectime here";
          break ;
        }
      default:
        Diagnostic(INTERNAL,(*ci)->pos) <<
          "unexpected varmode for " << (*ci)->name ;
      }
    ci++ ;
    diri++ ;
  }
  if ( diri ) {
    Diagnostic d(ERROR,pos) ;
    d << "Too many parameters to " << subject_name << "() specified" ;
    d.addline(subject_def->pos) << "(definition of "
                                << subject_name << ")" ;
  }
  if ( ci ) {
    Diagnostic d(ERROR,pos) ;
    d << "Too few parameters to " << subject_name << "() specified" ;
    d.addline(subject_def->pos) << "(definition of "
                                << subject_name << ")" ;
  }
}

void
SpecfDirective::finalize()
{
}

void
SpecfAnno::app_specfunction(FunDef *f)
{
  owner->subject_def = f ;
  owner->finalize();
}

void Directives::add_generator(GeneratorDirective *gd) {
  generators.push_back(gd);
  (void)new SpecfAnno(gd);
}

void
Directives::add_debug(DebugDirective *dd)
{
  debug_flags.push_back(dd);
  (void)new SpecfAnno(dd);
}

void Directives::ReadSubjectProgram() {
  while(!sources.empty()) {
    SourceDirective *thissource = sources.front() ;
    sources.pop_front() ;
    thissource->Read(cppargs);
  }
}


// JPS: I guess this means "Apply user annotations callback" which is used to
// apply user annotations to local variables in a function body. Only compound
// statements are affected since only they can have declarations.
class AUA_Callback : public StmtCallbackClosure {
  char const*const funname ;
  size_t const sizeplus ;
public:
  AUA_Callback(char const *n) : funname(n), sizeplus(strlen(n)+3) {}
  void fordecllist(Plist<VarDecl>*l) {
    foreach( i, *l, Plist<VarDecl> ) {
      char *tempname = new char[sizeplus + strlen(i->name)] ;
      strcpy(tempname,funname);
      strcat(tempname,"::");
      strcat(tempname,i->name);
      UserAnno::Apply(tempname,*i);
      delete[] tempname ;
    }
  }
  virtual void whatever(Stmt *s) {
    CompoundStmt *cs = s->this_as_compound() ;
    if ( cs )
      fordecllist(cs->objects);
  }
} ;

void ApplyUserAnnotations(CProgram *pgm) {
  // Functions.
  foreach( i, *pgm->functions, Plist<FunDef> ) {
    // Create a callback instance that knows the name of the containing
    // function, which is used to create full variable names such that
    // annotations can be applied.
    if( i->refer_to != NULL )
      continue ;
    AUA_Callback callback(i->name) ;
    if ( i->stmts ) { // An internal function
      callback.fordecllist(i->decls); // the parameters
      i->stmts->preorder(&callback); // locals on various levels
    }
    UserAnno::Apply(i->name,*i);
  }
  // Variables.
  foreach( ii, *pgm->definitions, Plist<VarDecl> ) {
    if( ii->refer_to != NULL )
      continue ;
    UserAnno::Apply(ii->name,*ii);
  }
  // Cleanup.
  UserAnno::DeleteThemAll();
}

//---------------------------------------------------

static SymbolTable<const char> wellknowns;

void Directives::add_wellknown(const char *word) {
  wellknowns.insert(word,"well-known");
  add_taboo(word);
}

bool Directives::is_wellknown(const char *word) {
  return wellknowns.lookup(word) != NULL;
}

//---------------------------------------------------

void Directives::add_taboo(const char *word) {
  taboos.push_back(word);
}

ostream& operator<< (ostream &ost,VariableMode ua) {
  switch(ua) {
  case VarIntAuto:      return ost << "[internal unannotated]" ;
  case VarIntResidual:  return ost << "residual" ;
  case VarIntSpectime:  return ost << "spectime" ;
  case VarVisResidual:  return ost << "visible residual" ;
  case VarVisSpectime:  return ost << "visible spectime" ;
  case VarExtResidual:  return ost << "residual [external]" ;
  case VarExtDefault:   return ost << "[external unannotated]" ;
  case VarExtSpectime:  return ost << "dangerous spectime" ;
  case VarConstant:     return ost << "well-known constant" ;
  case VarEnum:         return ost << "[enumeration member]" ;
  case VarMu:           return ost << "[mu varmode]" ;
  default:              return ost << "[unknown varmode " << (int)ua << ']' ;
  }
}

//---------------------------------------------------

CmxResult TheCmxResult ;

void JamInDirective(const char *directive,Position pos) {
  if ( cmxLexWrapper(directive,strlen(directive),pos).parse()
       && TheCmxResult.status != CmxResult::GotDirectives )
    Diagnostic(ERROR,pos) << "this is not a directive (colon missing)" ;
}

