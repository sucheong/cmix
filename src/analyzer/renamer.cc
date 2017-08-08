/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Generator of unique names
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_REGEX_H
#include <regex.h>  // for OSF alphas with buggy GCC installations
#endif
#include <ctype.h>
#include "analyses.h"
#include "renamer.h"
#include "directives.h"

class GenNameMgr : public NameMgr {
  multiArray<NameBase*> bases ;
  multiArray<unsigned> wantseq ;
  Plist<Numbered> *curscope ;
  Plist<Plist<Numbered> > scopes ;
  Plist<char const> taboos ;

  NameMap &map ;

public:
  GenNameMgr(NameMap &) ;
  void newscope();
  void suggest(Numbered *,const char *);
  void dictate(Numbered *,const char *);
  void resolve();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

extern char const * const PgenTaboos[] ; // in taboos.cc

// An important task of the renamer is to identify the usertypes
// which must keep their names in the generated programs. This is
// done by traversing any 'externally relevant' types and collecting
// the C_UserDef and C_EnumDef met along the way.
//
// The collection stops once a C_UserDef that has been registered
// is met (it is an invariant of Core C that all the instances of
// a C_UserDef have equal component types).
//
// C_EnumDef are not separated and should keep their names whether
//           they are used statically or dynamically.
// C_UserDef only keeps their names if they are used statically.
//           For dynamic uses the phase might collect a set which
//           gegen then uses to rename their residual versions.

static void
collect_type(C_Type *t,Pset<C_UserDef> &User, Pset<C_EnumDef> &Enum)
{
 tail_call:
  switch(t->tag) {
  case Abstract:
  case Primitive:
    break;
  case EnumType:
    Enum.insert(t->enum_def());
    break;
  case FunPtr:
  case Pointer:
    t = t->ptr_next() ;
    goto tail_call;
  case Array:
    t = t->array_next() ;
    goto tail_call ;
  case Function: {
    foreach(i,t->fun_params(),Plist<C_Type>)
      collect_type(*i,User,Enum);
    t = t->fun_ret() ;
    goto tail_call ; }
  case StructUnion:
    if ( !User.insert(t->user_def()) ) {
      foreach(i,t->user_types(),Plist<C_Type>)
        collect_type(*i,User,Enum);
    }
    break;
  }
}

// goodname checks that the name of a usertype is a real name
// (this is used for guaranteeing that unnamed usertypes
// have their names managed).
static bool
good_name(const char* name)
{
  return name != NULL && name[0] != '\0' && name[0] != '$' ;
}

#define RENEW(itr) mgr.suggest(*itr,itr->get_name())
#define BASTA(itr) mgr.dictate(*itr,itr->get_name())

void
MakeUniqueNames(C_Pgm const&corepgm,BTresult const &bt,
                NameMap &result,Pset<C_UserDef> *res_ud)
{
  GenNameMgr mgr(result) ;

  Pset<C_UserDef> new_user_dyn ;
  Pset<C_UserDef> user_stat ;
  Pset<C_UserDef> &user_dyn = res_ud == NULL ? new_user_dyn : *res_ud ;
  Pset<C_EnumDef> enum_ext ;

  // collect taboos and externally visible types from external functions
  foreach(e,corepgm.exfuns,Plist<C_Decl>) {
    mgr.dictate(NULL,e->get_name()) ;
    if ( bt.Dynamic(e->type) )
      collect_type(e->type,user_dyn,enum_ext) ;
    else
      collect_type(e->type,user_stat,enum_ext) ;
  }
  // manage names and and collect visible types from external variables
  foreach(var,corepgm.globals,Plist<C_Decl>)
    switch ( (*var)->varmode() ) {
    case VarExtResidual:
    case VarExtDefault:
    case VarVisResidual:
      collect_type(var->type,user_dyn,enum_ext);
      BASTA(var);
      break ;
    case VarExtSpectime:
    case VarVisSpectime:
      collect_type(var->type,user_stat,enum_ext);
      BASTA(var);
      break ;
    case VarIntAuto:
    case VarIntResidual:
    case VarIntSpectime:
      RENEW(var);
      break ;
    default:
      assert(0);
    }

  // manage usertype names
  foreach(ud,corepgm.usertypes,Plist<C_UserDef>) {
    if ( user_stat.find(*ud) && good_name(ud->get_name()) )
      BASTA(ud);
    else
      RENEW(ud);
  }

  // manage enum names and members
  foreach(ed,corepgm.enumtypes,Plist<C_EnumDef>) {
    if ( enum_ext.find(*ed) && good_name(ed->get_name()) )
      BASTA(ed);
    else
      RENEW(ed);
    if ( enum_ext.find(*ed) )
      foreach(i,ed->members(),Plist<C_UserMemb>)
        BASTA(i);
    else
      foreach(i,ed->members(),Plist<C_UserMemb>)
        RENEW(i);
  }

  // taboo the names of generator entry points
  foreach(d,corepgm.generators,Plist<C_Decl>) {
    BASTA(d);
  }
    
  // manage the names of internal functions
  foreach(fun,corepgm.functions,Plist<C_Decl>)
    RENEW(fun);

  // manage local variables
  foreach(fun2,corepgm.functions,Plist<C_Decl>) {
    mgr.newscope();
    foreach(p,fun2->fun_params(),Plist<C_Decl>)
      RENEW(p);
    foreach(v,fun2->fun_locals(),Plist<C_Decl>)
      RENEW(v);
  }

  // manage local variables in the entry point functions
  foreach(fun3,corepgm.generators,Plist<C_Decl>) {
    mgr.newscope();
    foreach(p,fun3->fun_params(),Plist<C_Decl>)
      RENEW(p);
    foreach(v,fun3->fun_locals(),Plist<C_Decl>)
      RENEW(v);
  }

  // manage the names of struct and union fields
  foreach(ud2,corepgm.usertypes,Plist<C_UserDef>) {
    mgr.newscope();
    foreach(i,ud2->names,Plist<C_UserMemb>)
      RENEW(i);
  }

  foreach(taboo,directives.taboos,Plist<const char>)
    mgr.add_taboo(*taboo);
  mgr.add_taboos(PgenTaboos);
  mgr.resolve() ;
}
    
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define NAME_ACCEPT(a) \
a accept(Numbered::C_DECL); \
a accept(Numbered::C_USERMEMB); \
a accept(Numbered::C_USERDEF); \
a accept(Numbered::C_ENUMDEF);

NameMap::NameMap()
  : multiArray<const char*>("\"UNINITIALIZED")
{
  NAME_ACCEPT(this->)
    }

GenNameMgr::GenNameMgr(NameMap &m)
  : bases(NULL), wantseq(0), map(m)
{
  NAME_ACCEPT(bases.)
    NAME_ACCEPT(wantseq.)
    newscope() ;
}
    
void
GenNameMgr::newscope()
{
  curscope = new Plist<Numbered> ;
  scopes.push_back(curscope);
}

static void
pgen_cooker(char *base)
{
  if ( strncmp("cmix",base,4) == 0 ) {
    base[1] = 'i' ;
    base[2] = 'm' ;
  }
  if ( base[0] == '_' )
    base[0] = 'U' ;
}

void
GenNameMgr::suggest(Numbered *n,char const *proposal)
{
  map[n] = proposal ;
  request_internal(proposal,bases[n],wantseq[n],pgen_cooker);
  curscope->push_back(n);
}

void
GenNameMgr::dictate(Numbered *n,const char *name)
{
  if ( n )
    map[n] = name ;
  taboos.push_back(name);
}

void
GenNameMgr::resolve()
{
  foreach(taboo,taboos,Plist<char const>)
    add_taboo(*taboo);
    
  // manage the names of the first, global, scope
  Plist<Numbered> *globals ;
  globals = scopes.front() ;
  scopes.pop_front() ;
  foreach(n,*globals,Plist<Numbered>) {
    NameBase *nb = bases[*n] ;
    unsigned &seq = wantseq[*n] ;
    do {
      if ( seq < nb->firstseq )
        seq = nb->firstseq ;
      if ( seq < 10000 || nb == list.front() )
        break ;
      nb = list.front() ;
      seq = 1 ;
    } while(1) ;
    nb->firstseq = seq + 1;
  }

  // manage the names of subsequent scopes
  foreach(sc,scopes,Plist<Plist<Numbered> >) {
    foreach(n,**sc,Plist<Numbered>) {
      NameBase *nb = bases[*n] ;
      nb->index = nb->firstseq ;
    }
    foreach(nn,**sc,Plist<Numbered>) {
      NameBase *nb = bases[*nn] ;
      unsigned &seq = wantseq[*nn] ;
      do {
        if ( seq < nb->index )
          seq = nb->index ;
        if ( seq < 10000 || nb == list.front() )
          break ;
        nb = list.front() ;
        seq = 1 ;
      } while(1) ;
      nb->index = seq + 1;
      globals->push_back(*nn);
    }
    delete *sc ;
  }

  // install the names we've found
  char buildbuffer[32];
  foreach(nn,*globals,Plist<Numbered>)
    if ( wantseq[*nn] == 0 && strcmp(bases[*nn]->base,map[*nn]) == 0 )
    ; // no need to add a seqence number for this name
    else {
      sprintf(buildbuffer,"%s%u",bases[*nn]->base,wantseq[*nn]);
      if ( strcmp(buildbuffer,map[*nn]) != 0 )
        map[*nn] = stringDup(buildbuffer);
    }

  delete globals ;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

char NameMgr::split_alpha[27] ;
unsigned NameMgr::split_num ;

void
NameMgr::split_internal(const char *orgname,cooker cook)
{
  if ( orgname == NULL ) {
    strcpy(split_alpha," /*?*/null") ;
    split_num = 0 ;
    return ;
  }
  if ( orgname[0] == '$' ) {
    strcpy(split_alpha,"anonymous") ;
    split_num = 0 ;
    return ;
  }
  bool toolong = strlen(orgname)>26 ;
  if ( toolong ) {
    strncpy(split_alpha,orgname,26);
    split_alpha[26] = '\0' ;
  } else
    strcpy(split_alpha,orgname);
  if ( cook )
    cook(split_alpha);
  char *pc ;
  for ( pc = split_alpha ; *pc ; pc++ )
    ;
  while(isdigit(pc[-1]))
    pc-- ;
  if ( toolong ||
       strlen(pc) > 4 && (pc!=split_alpha+1 || split_alpha[0]!='v') )
    split_num = 0 ;
  else
    split_num = atoi(pc);
  *pc = '\0' ;
}

void
NameMgr::request_internal(const char *orgname,NameBase *&nb,unsigned &seq,
                          cooker cook)
{
  split_internal(orgname,cook);
  seq = split_num ;
  nb = symtab.lookup(split_alpha) ;
  if ( nb == NULL ) {
    nb = new NameBase ;
    nb->base = stringDup(split_alpha);
    nb->index = list.size() ;
    nb->firstseq = 0 ;
    symtab.insert(nb->base,nb);
    list.push_back(nb);
  }
}

NameMgr::NameMgr()
{
  NameBase *nb ;
  unsigned u ;
  request_internal("v",nb,u);
}

NameMgr::~NameMgr()
{
  foreach(nb,list,Plist<NameBase>)
    delete *nb ;
}

void
NameMgr::add_taboo(char const *taboo)
{
  if ( strlen(taboo) > 30 )
    return ; // no generated names will be longer than that
  split_internal(taboo);
  NameBase *nb = symtab.lookup(split_alpha) ;
  if ( nb && nb->firstseq <= split_num )
    nb->firstseq = split_num+1 ;
}

void
NameMgr::add_taboos(char const*const taboos[]) {
  for ( ; *taboos ; taboos++ )
    add_taboo(*taboos);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

ResNameMgr::ResNameMgr()
{
}

static void
res_cooker(char *base)
{
  if ( base[0] == '_' )
    base[0] = 'U' ;
}

char const *
ResNameMgr::request(char const *org)
{
  NameBase *nb ;
  unsigned seqnum ;
  static char const format[] = "cmixRequestName(%u,%u)" ;
  static char buffer[sizeof format+8] ;
  request_internal(org,nb,seqnum,res_cooker);
  sprintf(buffer,format,nb->index,seqnum);
  return buffer ;
}

static char const **nametab_bases ;

static int
nametab_compar(const void *a,const void *b)
{
  return strcmp(nametab_bases[*(unsigned const*)a],
                nametab_bases[*(unsigned const*)b]);
}

void
ResNameMgr::emittable(ostream &ost)
{
  ost << "static struct cmixNameRec cmixNametable[] = {" ;

  unsigned length = list.size() ;
  nametab_bases = new char const*[length] ;
  unsigned *nametab_sorted = new unsigned[length] ;
  Plist<NameBase>::iterator i = list.begin() ;
  for(unsigned cnt = 0 ; cnt < length ; cnt++ ) {
    ost << "\n\t{\"" << i->base << "\"," << i->firstseq << ",0}," ;
    nametab_bases[cnt] = i->base ;
    nametab_sorted[cnt] = cnt ;
    i++ ;
  }
  qsort((void*)nametab_sorted,length,sizeof(unsigned),nametab_compar);

  ost << "\t};\nstatic unsigned const cmixNametableS[] = {" ;
  for(unsigned cnt2 = 0 ; cnt2 < length ; cnt2++ ) {
    if ( cnt2 % 12 == 0 )
      ost << "\n\t" ;
    ost << nametab_sorted[cnt2] << ',' ;
  }
  ost << "};\n" ;

  delete[] nametab_sorted ;
  delete[] nametab_bases ;
}
