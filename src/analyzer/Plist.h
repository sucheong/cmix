/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: lists of pointers
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __PLIST__
#define __PLIST__
#include "ygtree.h"
#include "auxilary.h"

template <class T> class Plist ;

//-------------------------------------------------------
// class Plistiter masquerades an yg_iterator as a
// STL-like constant iterator for T* containers
template <class T> class Plistiter {
protected:
  yg_iterator I ;
  friend class Plist<T> ;
  inline Plistiter(yg_iterator i): I(i) {}
public:
  inline Plistiter()			: I() {}
  inline operator bool() const		{ return I.valid(); }
  inline T* operator*() const		{ return (T*)I.get().v; }
  inline T* operator->() const		{ return (T*)I.get().v; }
  inline Plistiter operator++()		{ ++I; return I; }
  inline Plistiter operator--()		{ --I; return I; }
  inline Plistiter operator++(int)	{ yg_iterator J(I); ++I; return J; }
  inline Plistiter operator--(int)	{ yg_iterator J(I); --I; return J; }
  inline bool operator==(const Plistiter &o) const { return I == o.I; }
};

template <class T> class Plistmutable : public Plistiter<T> {
  friend class Plist<T> ;
  inline Plistmutable(yg_iterator i): Plistiter<T>(i) {}
public:
  inline Plistmutable() {}
  inline void operator<<(T* data) {
    assert(data != NULL);
    Plistiter<T>::I.put(yg_v(data));
  }
};

//-------------------------------------------------------
// class Plist creates an interface between the generic
// ygtree and the specialised list-of-pointer-to-T
// used in C-MIX/II''s data structures.

template<class T> class Plist {
  void operator =(const Plist&) {} // must not be called
  Plist(const Plist&) {} // must not be called
  yg_tree Y ;
  typedef T *content ;
  static inline T* get(yg_iterator const &I) { return (T*)I.get().v; }
public:
  typedef Plistmutable<T> mutable_iterator ;
  typedef Plistiter<T> iterator ;
  inline Plist(): Y() {}
  inline Plist(T* data): Y(yg_v((void*)data)) {}

  inline iterator begin() const	{ return Y.begin(); }
  inline iterator end() const		{ return Y.end(); }
  inline mutable_iterator begin()	{ return Y.begin(); }
  inline mutable_iterator end()	{ return Y.end(); }
  inline T* front() const	{ return get(Y.begin()); }
  inline T* back() const	{ yg_iterator I=Y.end(); --I; return get(I); }
  inline void push_front(T* d) {
    assert(d!=NULL);
    yg_iterator I=Y.begin();
    Y.insert(I,yg_v((void*)d));
  }
  inline void push_back(T* d) {
    assert(d!=NULL);
    yg_iterator I=Y.end();
    Y.insert(I,yg_v((void*)d));
  }
  inline void insert(mutable_iterator &i,T* d) {
    Y.insert(i.I,yg_v((void*)d));
  }
  inline void pop_front()	{ yg_iterator I = Y.begin() ; Y.erase(I); }
  inline void pop_back()	{ yg_iterator I = Y.end(); --I; Y.erase(I); }
  inline T* operator[](count_t i) { return get(Y.index(i)); }
  inline void erase(mutable_iterator &i)	    { Y.erase(i.I); }
  inline void splice(mutable_iterator i,Plist &o) { Y.splice(i.I,o.Y); }
  inline void clear(void)     { Y.clear(); }
  inline bool empty() const	{ return Y.size() == 0; }
  inline count_t size() const	{ return Y.size(); }
};

#endif
