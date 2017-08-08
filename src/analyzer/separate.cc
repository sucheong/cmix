/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Separate struct definitions according to BTs
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "options.h"
#include "analyses.h"
#include "fixiter.h"
#include "array.h"

#define debug debugstream_separate

// local type traversing functions, defined at the bottom of the file
static Plist<C_Type> *find_parts(C_Type*);
static bool local_match(C_Type*,C_Type*,BTresult const&);

struct SepNode ;

struct SepData {
  array<C_Type,Plist<C_Type>*> instance2parts ;
  array<C_UserDef,SepNode*> def2node ;
  Plist<SepNode> defs ;

  SepData() : instance2parts(NULL), def2node(NULL) {}
} ;

struct SepNode : public FixpointVertex {
  SepData &glob ;
  C_UserDef *const def ;
  Plist<C_UserDef> canonical ;
  SepNode(C_UserDef*, SepData &);
  void add(C_Type *);
  void setcanonical();
  unsigned normalize() ;
  virtual bool solve(FixpointSolver &) ;
} ;

SepNode::SepNode(C_UserDef *orig, SepData &g)
  : glob(g), def(orig->copy())
{
  glob.def2node[def] = this ;
  glob.defs.push_back(this);
}

void
SepNode::add(C_Type *t)
{
  assert( t->tag == StructUnion );
  def->instances.push_back(t);
  t->user_def(def);
}

void
SepNode::setcanonical()
{
  assert( !def->instances.empty() );
  Plist<C_Type> *parts = glob.instance2parts[def->instances.front()] ;
  assert( parts != NULL );
  foreach(i,*parts,Plist<C_Type>) {
    C_UserDef *user_def = i->user_def() ;
    canonical.push_back(user_def) ;
    *glob.def2node[user_def] >> *this ;
  }
}

static bool
match(Plist<C_Type> *ts, Plist<C_UserDef> &us) {
  assert( ts != NULL );
  assert( ts->size() == us.size() );
  Plist<C_Type>::iterator ti = ts->begin() ;
  Plist<C_UserDef>::iterator ui = us.begin() ;
  for ( unsigned n = ts->size() ; n > 0 ; n-- ) {
    if ( ti->user_def() != *ui )
      return false ;
  }
  return true ;
}

unsigned
SepNode::normalize()
{
  // normalize() returns the (zero-based) index of the
  // first canonical slave. If there are no canonical
  // instances at all, the first slave is *declared* to
  // be canonical, and the dependencies are updated.
  Plist<C_Type> &instances = def->instances ;
  unsigned noncan_met = 0 ;
  foreach(slave,instances,Plist<C_Type>) {
    if ( match(glob.instance2parts[*slave],canonical) )
      return noncan_met ;
    noncan_met++ ;
  }
  // if we fall out of the loop there were no canonical
  // nodes at all. We then need do redefine the canonicality
  // concept - it cannot be allowed to move everything away
  // from the definition lest the fixpoint iteration will
  // loop if recursive types exist.
  canonical.clear() ;
  setcanonical() ;
  return 0 ;
}

bool
SepNode::solve(FixpointSolver &s)
{
  SepNode *thenew = NULL ;
  Plist<C_Type> &instances = def->instances ;
  Plist<C_Type>::mutable_iterator instance = instances.begin() ;
  unsigned j = normalize() ;
  for ( unsigned i = 0 ; instance ; i++ ) {
    // when i < j we know it is noncanonical
    // when i = j we know it is canonical
    // when i > j we have to check
    if ( i==j || i>j && match(glob.instance2parts[*instance],canonical) )
      instance++ ;
    else {
      if ( thenew == NULL )
        thenew = new SepNode(def,glob) ;
      thenew->add(*instance);
      instances.erase(instance);
    }
  }
  if ( thenew != NULL ) {
    thenew->setcanonical() ;
    s += thenew ;
    return true ;
  } else
    return false ;
}

static void
make_nodes(C_UserDef *orig,SepData &glob,BTresult const&bt)
{
  if ( debug )
    debug << "Collecting SepNodes from " << (void*)orig << " (ID "
          << orig->Numbered_ID << ")" << endl ;
  // we move elements out of the old definition until there are
  // no ones left
  while ( !orig->instances.empty() ) {
    SepNode *node = new SepNode(orig,glob) ;
    if ( debug )
      debug << "  Making new SepNode at " << (void *)node << endl ;
    Plist<C_Type> &instances = orig->instances ;
    Plist<C_Type>::mutable_iterator i = instances.begin() ;
        
    C_Type *lead = *i ;
    goto loop_start ; // yes, I use a goto and I'm proud of it!
        
    while( i ) {
      if ( local_match(lead,*i,bt) ) {
      loop_start:
        if ( debug )
          debug << "    Adding instance " << (void*)*i << " (ID "
                << i->Numbered_ID << ") to node "
                << (void*)node << endl ;
        node->add(*i);
        glob.instance2parts[*i] = find_parts(*i) ;
        instances.erase(i) ;
      } else
        i++ ;
    }
  }
  delete orig ;
}

void
Separate(C_Pgm &pgm,BTresult const&bt)
{
  SepData glob ;
  FixpointSolver solver ;    

  // First get all of the userdefs into the constraint system.
  // In this phase we pre-split on the binding times of
  // immediately occuring types, down to the first layer
  // of StructUnions.
  foreach(o,pgm.usertypes,Plist<C_UserDef>)
    make_nodes(*o,glob,bt) ;
  pgm.usertypes.clear() ;
    
  // Now all existing C_UserDefs are wrapped in a SepNode
  // and the glob.def2node mapping has been fully set up.
  // Thus we can compute the relations between the initial
  // constraint nodes.
  foreach(i,glob.defs,Plist<SepNode>) {
    i->setcanonical() ;
    if ( i->def->instances.size() > 1 )
      solver += *i ;
  }

  if ( debug ) debug << "Solving..." << endl ;
  solver.solve() ;
  if ( debug ) debug << "Solved" << endl ;
    
  // Finished. Move the separated userdefs back on the main list
  // and delete the instance2parts mapping
  foreach(ii,glob.defs,Plist<SepNode>) {
    if (debug)
      debug << "Finished a new definition " << (void*)ii->def << "(ID "
            << ii->def->Numbered_ID << ")" << endl ;
    assert( !ii->def->instances.empty() );
    pgm.usertypes.push_back(ii->def);
    foreach(j,ii->def->instances,Plist<C_Type>) {
      if (debug)
        debug << "  Removing partslist for " << (void*)*j << "(ID "
              << j->Numbered_ID << ")" << endl ;
      Plist<C_Type> *&partslist = glob.instance2parts[*j] ;
      assert( partslist != NULL );
      delete partslist ;
      partslist = NULL ;
    }
  }
}


/***************************************************/
/***************************************************/
/***************************************************/


static void
find_parts(Plist<C_Type> &ts, Plist<C_Type> *parts)
{
  foreach(i, ts, Plist<C_Type>) {
    C_Type *t = *i ;
    while(1)
      switch(t->tag) {
      case Array:
        t = t->array_next() ;
        break ;
      case FunPtr:
      case Pointer:
        t = t->ptr_next() ;
        break ;
      case Function:
        find_parts(t->fun_params(),parts);
        t = t->fun_ret() ;
        break ;
      case StructUnion:
        parts->push_back(t) ;
        goto break2;
      default:
        goto break2;
      }
  break2:
    ;
  }
}
 
static Plist<C_Type> *
find_parts(C_Type *t)
{
  Plist<C_Type> *parts = new Plist<C_Type> ;
  assert( t->tag == StructUnion );
  find_parts( t->user_types(), parts );
  return parts ;
}

static bool
local_match(const Plist<C_Type> &ts, const Plist<C_Type> &us,BTresult const&bt)
{
  assert( ts.size() == us.size() );
  double_foreach(ti,ui,ts,us,Plist<C_Type>) {
    C_Type *t = *ti ;
    C_Type *u = *ui ;
    while(1) {
      assert(t->tag == u->tag);
      if ( bt.Dynamic(t) != bt.Dynamic(u) )
        return false ;
      switch(t->tag) {
      case Array:
        t = t->array_next() ;
        u = u->array_next() ;
        break ;
      case FunPtr:
      case Pointer:
        t = t->ptr_next() ;
        u = u->ptr_next() ;
        break ;
      case Function:
        if ( !local_match(t->fun_params(),u->fun_params(),bt) )
          return false ;
        t = t->fun_ret() ;
        u = u->fun_ret() ;
        break ;
      default:
        goto break2 ;
      }
    }
  break2:
    ;
  }
  return true ;
}
    

static bool
local_match(C_Type *t,C_Type *u,BTresult const&bt)
{
  if ( bt.Dynamic(t) != bt.Dynamic(u) )
    return false ;
  return local_match(t->user_types(), u->user_types(), bt) ;
}
