/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: 
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "typearr.h"

void_typearr::node::node()
  : data(NULL), pointerto(NULL), funreturning(NULL), arrayof(NULL) {}

void_typearr::node::~node()
{
}

void_typearr::node*
void_typearr::findnode(C_Type *t)
{
  C_Type *t2 = t ;
  node **nodepp = NULL ;

  // first find out which tree to search in
  while(1)
    if ( t2 == NULL ) {
      nodepp = &for_notype ;
      break ;
    } else {
      switch ( t2->tag ) {
      case Pointer:
      case FunPtr:
        t2 = t2->ptr_next() ;
        continue ;
      case Array:
        t2 = t2->array_next() ;
        continue ;
      case Function:
        t2 = t2->fun_ret() ;
        continue ;
      case Enum:
        nodepp = &for_enums[t2->enum_def()] ;
        break ;
      case StructUnion:
        nodepp = &for_usertypes[t2->user_def()] ;
        break ;
      case Abstract:
      case Primitive:
        nodepp = &for_basetypes[t2->hash()] ;
        break ;
      }
      break ;
    }
  assert(nodepp!=NULL) ;

  // then search down the tree
  while(1) {
    node *nodep = *nodepp ;
    if ( nodep == NULL ) {
      nodep = new node ;
      *nodepp = nodep ;
      all_my_nodes.push_back(nodep);
    }
    if ( t == t2 )
      return(nodep);
    nodepp = NULL ;
    switch(t->tag) {
    case Pointer:
    case FunPtr:
      nodepp = &nodep->pointerto ;
      t = t->ptr_next() ;
      break ;
    case Array:
      nodepp = &nodep->arrayof ;
      t = t->array_next() ;
      break ;
    case Function:
      nodepp = &nodep->funreturning ;
      t = t->fun_ret() ;
      break ;
    default:
      assert(0);
    }
  }
};

void *
void_typearr::index(C_Type *t)
{
  node *n = findnode(t);
  if ( n->data == NULL ) {
    known_types.push_back(t);
    n->data = v_create(t);
    v_created(t,n->data);
  }
  return n->data ;
}

void_typearr::void_typearr()
  : for_enums(NULL), for_usertypes(NULL), for_basetypes(NULL), for_notype(NULL)
{}

void_typearr::~void_typearr()
{
  foreach(i,all_my_nodes,Plist<node>)
    delete *i ;
}
