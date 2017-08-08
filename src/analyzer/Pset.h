/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: sets of pointers to Numbered deriviates
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __PSET__
#define __PSET__

#include "ygtree.h"

template <class T> class Pset ;

//-------------------------------------------------------
// class Psetiter masquerades an yg_iterator as a STL-like
// iterator for Pset<T>.
template <class T> class Psetiter {
  yg_iterator I ;
  friend class Pset<T> ;
  inline Psetiter(yg_iterator i): I(i) {}
public:
  inline Psetiter()		: I() {}
  inline operator bool() const { return I.valid() ; }
  inline T* operator*() const
    {
      (void)sizeof(T); // make sure we know the lattice for T
      return (T*)I.get().n;
    }
  inline T* operator->() const
    {
      (void)sizeof(T); // make sure we know the lattice for T
      return (T*)I.get().n;
    }
  inline Psetiter operator++()	{ ++I; return I; }
  inline Psetiter operator--()	{ --I; return I; }
  inline Psetiter operator++(int)	{ yg_iterator J(I); ++I; return J; }
  inline Psetiter operator--(int)	{ yg_iterator J(I); --I; return J; }
  inline bool operator==(const Psetiter &o) const { return I == o.I; }
};

//-------------------------------------------------------
// class Pset creates an interface between the generic
// ygtree the specialised set-of-pointer-to-T
// (where T is a descendant of Numbered)
// used in C-MIX/IIs data structures.

bool Pset_insert(yg_tree &,Numbered *);
yg_iterator Pset_find(const yg_tree &,Numbered *);
void Pset_erase(yg_tree &,Numbered *);
bool Pset_union(yg_tree &,const yg_tree&); // returns true if A U B = A.
bool Pset_intersect(yg_tree &,const yg_tree&); // returns true if A /\ B = A.
bool Pset_minus(yg_tree &,const yg_tree&); // returns true if A \ B = A.


template<class T> class Pset {
  void operator=(const Pset&) {} // must not be called
  Pset(const Pset&) {} // must not be called
  yg_tree Y ;
  typedef T *content ;
public:
  typedef Psetiter<T> iterator ;
  typedef Psetiter<T> const_iterator ;
  
  inline Pset()			: Y() {}
  inline iterator begin() const	{ return Y.begin(); }
  inline iterator end() const		{ return Y.end(); }
  // returns true if the element is already in the set.
  inline bool insert(T* d)		{ return Pset_insert(Y,d); }
  inline iterator find(T* d) const	{ return Pset_find(Y,d); }
  inline void erase(T* d)		{ Pset_erase(Y,d); }
  inline void erase(iterator &i)	{ Y.erase(i.I); }
  inline void clear()			{ Y.clear(); }
  inline bool Union(Pset const &o) { return Pset_union(Y,o.Y); }
  inline bool Intersect(Pset const &o) { return Pset_intersect(Y,o.Y); }
  inline bool Minus(Pset const &o) { return Pset_minus(Y,o.Y); }
  inline void operator+=(Pset const &o) { Pset_union(Y,o.Y); }
  inline void operator*=(Pset const &o) { Pset_intersect(Y,o.Y); }
  inline void operator-=(Pset const &o) { Pset_minus(Y,o.Y); }
  inline bool empty() const		{ return Y.size() == 0; }
  inline count_t size() const		{ return Y.size(); }

  // for debugging:
  inline void trace(bool b) { Y.tracing = b; }
};

#endif
