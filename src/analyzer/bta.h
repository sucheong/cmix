/* (-*-c++-*-)
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  Interface between constraint generator (bta.cc)
 *                             constraint engine (btasolve.cc)
 *                             binding-time debugger (outbta.cc, btdebug.cc)
 * History:  Derived from code by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */
  
#ifndef __BTA__
#define __BTA__

#include "Plist.h"
#include "array.h"

class Diagnostic ;
class Output ;
class Outcore ;

// class BTpairlist: parallel lists of BTvariable and BTcause pointers

class BTcause ;
class BTvariable ;
class BTpairlist {
  Plist<BTvariable> v ;
  Plist<BTcause> c ;
public:
  class iterator {
    friend class BTpairlist ;
    Plist<BTvariable>::iterator v ;
    Plist<BTcause>::iterator c ;
    iterator(Plist<BTvariable>::iterator,Plist<BTcause>::iterator) ;
  public:
    void operator++(int);
    operator bool();
    BTvariable *variable() ;
    BTcause *cause() ;
  };
  iterator begin();
  bool empty();
  void push_back(BTvariable *,BTcause *);
  void clear();
};

// class BTobject: something with an explanation

class BTobject : public Numbered {
public:
  BTobject() ;
  virtual ~BTobject() {}
  virtual bool hasName() { return true; }
  virtual bool hasOrgName() { return true; }
  virtual void show(Diagnostic &) = 0;
  void DoShow(Diagnostic &d) {
    if ( hasOrgName() )
      show(d);
  }
  virtual Output *show(Outcore *) = 0;
};

// class BTcause: can write out the explanation for one or more constraints

class BTcause : public BTobject {
  unsigned refcount ;
  friend class BTvariable ;
  static Plist<BTcause> all_causes ;
public:
  BTcause();
  static void remove_unused() ;
};

// class BTvariable: a binding-time variable
// can also be viewed as a node in the analysis graph

class BTvariable : public BTobject {
public:
  enum Status { Untouched, Pending, Dynamic };
private:
  enum Status status ;
  BTpairlist waiting ;
  BTpairlist causes ;
public:
  BTvariable() { status = Untouched; }
  void influence(BTvariable *v,BTcause *c);
  bool isDynamic() { return status != Untouched; }
  BTpairlist::iterator get_causes() { return causes.begin(); }
    
  virtual bool hasOrgName() { return hasName(); }
  virtual void show(Diagnostic &) {} ;

  void flood(Status);
};

class BTanonymousVar : public BTvariable {
  virtual bool hasName() { return false; }
  virtual Output *show(Outcore *);
};

typedef multiArray<BTvariable*> BTmap ;

// class BTequalizer: converts equality constraints to
// dependency constraints in a single step.

class C_Type ;
class BTequalizer {
  array<C_Type,C_Type*> unionfind ;
  void Union(C_Type*,C_Type*);
  C_Type *Find(C_Type*);

  BTmap const &map ;
public:
  BTequalizer(BTmap const &);
  void unify(C_Type*,C_Type*,BTcause *);
};


class C_Pgm ;
void create_btvars(C_Pgm const&,BTmap&);

#endif /* __BTA__ */
