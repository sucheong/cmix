/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Arrays indexed (structurally) by Core C types
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __TYPEARR__
#define __TYPEARR__

#include "corec.h"
#include "array.h"

class void_typearr {
  struct node {
    void* data ;
    node *pointerto, *funreturning, *arrayof ;
    node() ;
    ~node() ;
  };
  Plist<node> all_my_nodes ;
  array<C_EnumDef,node*> for_enums ;
  array<C_UserDef,node*> for_usertypes ;
  Narray<node*> for_basetypes ;
  node *for_notype ;
  node* findnode(C_Type *);
 protected:
  virtual void* v_create(C_Type *)=0 ;
  virtual void v_created(C_Type *,void *) {}  
  void *index(C_Type*) ;
  void_typearr();
 public:
  Plist<C_Type> known_types ;
  virtual ~void_typearr();
};

template <class T>
class typearr : public void_typearr {
 protected:
  virtual T* create(C_Type *)=0 ;
  virtual void created(C_Type *,T *) {}
 private: 
  virtual void* v_create(C_Type *t) { return (void*)create(t); }
  virtual void v_created(C_Type *t, void *data) { created(t,(T*)data); }
 public:
  typearr() {}
  T* operator[](C_Type *t) { return (T*)index(t); }
};
  
#endif
