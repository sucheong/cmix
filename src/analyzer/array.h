/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Self-extending array
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "auxilary.h"

//#define CONSTRUCT_NOT_COPY

#ifdef CONSTRUCT_NOT_COPY
#include <stddef.h>

struct array_placement {
    void *vp ;
    array_placement(void *v) : vp(v) {}
} ;

inline void *
operator new(size_t,array_placement pl) {
    return pl.vp;
}
#endif

class array_extender {
    virtual void alloc_new(unsigned long)=0 ;
    virtual void move_data(unsigned long,unsigned long)=0 ;
    virtual void init_data(unsigned long)=0 ;
    virtual void kill_data(unsigned long)=0 ;
    virtual void ok_new()=0 ;
    unsigned long low_limit, high_limit ;
public:
    array_extender();
    virtual ~array_extender();
    unsigned long trans(unsigned long);
    unsigned long ctrans(unsigned long) const;
    void kill_all();
};

template <class T> class Narray : private array_extender {
    T *thedata ;
    T *newdata ;
    T startdata ;
    virtual void alloc_new(unsigned long u)
        {
            #ifdef CONSTRUCT_NOT_COPY
            newdata = malloc(u*sizeof(T));
            #else
            newdata = new T[u] ;
            #endif
        }
    virtual void move_data(unsigned long s,unsigned long d)
        {
            #ifdef CONSTRUCT_NOT_COPY
            ::new((void*)(newdata+d)) T(thedata[s]);
            thedata[s].~T() ;
            #else
            newdata[d] = thedata[s] ;
            #endif
        }
    virtual void init_data(unsigned long d)
        {
            #ifdef CONSTRUCT_NOT_COPY
            ::new((void*)(newdata+d)) T(startdata);
            #else
            newdata[d] = startdata ;
            #endif
        }
    virtual void kill_data(unsigned long s)
        {
            #ifdef CONSTRUCT_NOT_COPY
            thedata[s].~T() ;
            #endif
        }
    virtual void ok_new()
        {
            if (thedata) {
                #ifdef CONSTRUCT_NOT_COPY
                free(thedata);
                #else
                delete[] thedata;
                #endif
            }
            thedata=newdata;
        }
public:
    Narray(const T&t): thedata(NULL), startdata(t) {}
    inline T& operator[](unsigned long u) { u=trans(u); return thedata[u]; }
    inline T const& operator[](unsigned long u) const {
        u=ctrans(u);
        if ( u == -1UL )
            return startdata ;
        else
            return thedata[u];
    }
    inline void clear() {
        #ifdef CONSTRUCT_NOT_COPY
        kill_all();
        if (thedata)
            free(thedata);
        #else
        if (thedata)
            delete[] thedata;
        #endif
        thedata = NULL ;
    }
    virtual ~Narray() { clear(); }
};

template <class S,class T> class array {
    Narray<T> A ;
public:
    typedef S index ;
    inline array(const T&t) : A(t) {}
    inline T &operator[](const S &n)
        { return A[n. Numbered_ID]; }
    inline T &operator[](const S *n)
        { return A[n->Numbered_ID]; }
    inline T const &operator[](const S &n) const
        { return A[n. Numbered_ID]; }
    inline T const &operator[](const S *n) const
        { return A[n->Numbered_ID]; }
    inline void clear() { A.clear(); }
};
          
template <class T> class multiArray {
    T startdata ;
    Narray<T> *A[Numbered::NO_SORT] ;
    Narray<T> &find(Numbered::Sort s)
        {
            assert((unsigned)s < Numbered::NO_SORT &&
                   A[s] != NULL);
            return *A[s] ;
        }
    inline Narray<T> const&find(Numbered::Sort s) const
        { return ((multiArray*)this)->find(s); }
public:
    typedef Numbered index ;
    multiArray(const T&t) : startdata(t)
        {
            for(int i = 0 ; i < Numbered::NO_SORT ; i++ )
                A[i] = NULL ;
        }
    inline T &operator[](const Numbered &n)
        { return find(n. Numbered_Sort)[n. Numbered_ID]; }
    inline T &operator[](const Numbered *n)
        { return find(n->Numbered_Sort)[n->Numbered_ID]; }
    inline T const &operator[](const Numbered &n) const
        { return find(n. Numbered_Sort)[n. Numbered_ID]; }
    inline T const &operator[](const Numbered *n) const
        { return find(n->Numbered_Sort)[n->Numbered_ID]; }
    inline void accept(Numbered::Sort s)
        {
            assert( (unsigned)s < Numbered::NO_SORT );
            assert( A[s] == NULL );
            A[s] = new Narray<T>(startdata);
        }
    ~multiArray()
        {
            for(int i = 0 ; i < Numbered::NO_SORT ; i++ )
                if ( A[i] )
                    delete A[i] ;
        }
};

// A safe array always returs a reference to a pointer to an existing object:
// if the entry is uninitialized, create and return a new object.
template <class Arr, class T> class SafePArray {
  Arr* A;
public:
  inline SafePArray() : A(new Arr(NULL)) {}
  // Initialize with an unsafe array.
  inline SafePArray(Arr* init) : A(init) {}
  T*& operator[](const typename Arr::index &n) {
    assert(A);
    T*& ptr = (*A)[n];
    if (ptr == NULL) {
      // Create a new T
      ptr = new T;      
    }
    return ptr;
  }
  inline T*& operator[](const typename Arr::index *n) {
    return operator[](*n);
  }
  inline Arr* getArray() {
    Arr* tmp = A;
    A = NULL;
    return tmp;
  }
};

#endif
