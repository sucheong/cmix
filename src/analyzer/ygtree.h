/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: y/g trees.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

/* These trees are used to implement list and sets.
 *
 * They are binary trees where inner nodes differ from leaf
 * nodes. Inner nodes contain a single datum; leaf nodes contain
 * between 0 and LEAF_SIZE data. The tree above the leaf nodes
 * is an AVL tree.
 *
 * Special optimzations are performed for trees that have
 * never had more than one datum in them.
 *
 * An iterator always points into a leaf node; if it points beyond
 * the last datum in that leaf node, the (single) inner node that
 * falls between this and the next leaf is meant.
 *
 * When an insert or delete is performed all iterators into the
 * list except the one used to specify the operation are invalidated.
 *
 * Iterator validity is managed by assigning a sequence number to
 * each chronological version of the list; the iterators keep
 * backup copies of this.
 */

#ifndef _YG_TREE_
#define _YG_TREE_

#ifndef __CMIX_CONFIGURE__
#error Include <cmixconf.h> before ygtree.h
#endif

//#define YG_STATISTICS

class Numbered ;
union yg_datum {
    unsigned u ;
    void* v ;
    Numbered* n ;
} ;

inline yg_datum yg_u(unsigned u)  { yg_datum d; d.u = u; return d; }
inline yg_datum yg_v(void *v)     { yg_datum d; d.v = v; return d; }
inline yg_datum yg_n(Numbered *n) { yg_datum d; d.n = n; return d; }

typedef bool (*yg_lesseq)(yg_datum,yg_datum);

typedef unsigned count_t ;

class yg_node ;

class yg_tree ;

class yg_iterator {
    friend class yg_tree ;
    yg_tree *owner ;
    unsigned long version ;
    yg_node *which ;
    unsigned where ;
    
    void checkok() const ;
    bool isinner() const ; // also checks
    
    yg_iterator(yg_tree const*,yg_node*,unsigned);
public:
    yg_iterator(); // initializes to unuseable iterator
    // default copy and assignment operators are OK
    bool valid() const ;
    void operator++() ;
    void operator--() ;
    bool operator==(const yg_iterator&) const;
    yg_datum get() const;
    void put(yg_datum) const;
} ;

class yg_tree {
    friend class yg_iterator ;
    unsigned long version ;
    yg_node *root ;
    
    void new_version() ;
    void make_ajour(yg_node *,yg_iterator &) ;
    // exactly the mentioned iterator stays valid

    union {
        yg_node *first ;
        unsigned smallsize ;
    } ;
    union {
        yg_node *last ;
        yg_datum single ;
    } ;
    void firstlastajour() ;
    
    #ifdef YG_STATISTICS
    yg_tree *nextlivetree, **prevlivetree ;
    friend void ygtree_statistics() ;
    #endif
public:
    yg_tree();
    yg_tree(yg_datum);
    ~yg_tree();
    
    yg_iterator begin() const;
    yg_iterator end() const;
    yg_iterator index(count_t) const;
    yg_iterator find(yg_datum,yg_lesseq) const;
    count_t size() const ;
    void erase(yg_iterator&);
    void clear();
    void insert(yg_iterator&,yg_datum);
    void splice(yg_iterator&,yg_tree &);

    // for debugging
    bool tracing ;
    void dump() ;
    void fullcheck() ;
} ;

#endif
