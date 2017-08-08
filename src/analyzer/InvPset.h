#ifndef __INVPSET__
#define __INVPSET__

#include "Pset.h"
#include "auxilary.h"

// The must-write analysis is started up with all must-write sets being full,
// ie containing all objects in the program. To efficiently represent such
// sets, a special version (InvPSet) of Pset is used. It inherits from
// class Numbered so that it can be put into arrays, etc.
template<class T> class InvPset
{
  // Inverse is true if the set is inversed.
  bool inverse;
 protected:
  Pset<T> set;
 public:
  // The default constructor makes an empty set.
  InvPset() : inverse(false) {}
  // This constructor can make a full set.
  bool insert(T* x) {
    if (inverse) {
      Psetiter<T> I = set.find(x);
      if (I) {
        set.erase(I);
        return false;
      }
      else
        return true;
    }
    else {
      return set.insert(x);
    }
  };
  void erase(T x) {
    if (inverse) {
      set.insert(x);
    }
    else {
      set.erase(x);
    }
  };
  void clear() {
    inverse = false;
    set.clear();
  };
  void fill() {
    inverse = true;
    set.clear();
  };
  bool Union(InvPset const& other) {
    if (inverse) {
      if (other.inverse) {
	return set.Intersect(other.set);
      }
      else {
	return set.Minus(other.set);
      }
    }
    else {
      if (other.inverse) {
	// Save old set.
	Pset<T> old_set;
	old_set += set;
	// Make a new inverse set.
	inverse = true;
	set.clear();
	set += other.set;
	set -= old_set;
	// The universe is implicit (unknown) so we must assume that a
	// change has taken place:
	return false; 
      }
      else {
	return set.Union(other.set);
      }
    }
  };
  bool Intersect(InvPset const& other) {
    if (inverse) {
      if (other.inverse) {
	return set.Union(other.set);
      }
      else {
	// Save old set.
	Pset<T> old_set;
	old_set += set;
	// Make a new set.
	inverse = false;
	set.clear();
	set += other.set;
	set -= old_set;
	return false;
      }
    }
    else {
      if (other.inverse) {
	return set.Minus(other.set);
      }
      else {
	return set.Intersect(other.set);
      }
    }
        
  };
  bool Minus(InvPset const& other) {
    if (inverse) {
      if (other.inverse) {
	// Save old set.
	Pset<T> old_set;
	old_set += set;
	// Make a new set.
	inverse = false;
	set.clear();
	set += other.set;
	set -= old_set;
	return false;
      }
      else {
	return set.Union(other.set);
      }
    }
    else {
      if (other.inverse) {
	return set.Intersect(other.set);
      }
      else {
	return set.Minus(other.set);
      }
    }
  }
  //inline void operator+=(InvPset const& i) { Union(i); }
  //inline void operator*=(InvPset const& i) { Intersect(i); }
  //inline void operator-=(InvPset const& i) { Minus(i); }
  inline bool empty() const { return (inverse) ? false : set.empty(); }
  inline bool full() const { return (inverse) ? set.empty() : false; }
  inline bool isInversed() const { return inverse; }
  inline Pset<T> const& theSet() const { return set; }
};

#endif
