/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: set of pointers to Numbered things
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __NSET__
#define __NSET__

#include "ygtree.h"

//-------------------------------------------------------
// class Nset is a set of unsigned

class Nset;

class Nsetiter {
  yg_iterator I ;
  friend class Nset ;
  inline Nsetiter(yg_iterator i): I(i) {}
public:
  inline Nsetiter()		: I() {}
  inline operator bool() const { return I.valid() ; }
  inline unsigned operator*() const
        {
            return (unsigned)I.get().n;
        }
  inline Nsetiter operator++()	{ ++I; return I; }
  inline Nsetiter operator--()	{ --I; return I; }
  inline Nsetiter operator++(int)	{ yg_iterator J(I); ++I; return J; }
  inline Nsetiter operator--(int)	{ yg_iterator J(I); --I; return J; }
  inline bool operator==(const Nsetiter &o) const { return I == o.I; }
};

class Nset {
    void operator=(const Nset&) {} // must not be called
    Nset(const Nset&) {} // must not be called
    yg_tree Y ;
public:
    typedef Nsetiter iterator ;
    typedef Nsetiter const_iterator ;
    inline Nset()			: Y() {}
    inline iterator begin() const	{ return Y.begin(); }
    inline iterator end() const		{ return Y.end(); }
    // returns true if the element is OLD
    bool insert(unsigned d);
    inline void erase(iterator &i)	{ Y.erase(i.I); }
    bool contains(unsigned d);
    inline void clear()			{ Y.clear(); }
    inline bool empty() const		{ return Y.size() == 0; }
    inline count_t size() const		{ return Y.size(); }

    // for debugging:
    inline void dump() { Y.dump(); }
    inline void fullcheck() { Y.fullcheck(); }
};

#endif
