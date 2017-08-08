/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Implementation of y/g trees
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdlib.h>
#include "options.h"
#include "ygtree.h"
#include "Pset.h"
#include "Nset.h"
#include "auxilary.h" // for IDcompare
#define debug debugstream_ygtree
#define DOUT if (tracing && debug) debug

#define SMART_NEW_DELETE
#define LEAF_SIZE 6

#define asserti assert
// asserti is used for inner consistency checks on the tree structure,
// while plain assert is used for checking whether client code uses
// the ygtrees correctly.

class yg_node {
  friend class yg_tree ;
  friend class yg_iterator ;
  count_t size ;
  yg_node *up_leaf ; // nil if inner node; this if only leaf

public:
  struct US_inner {
    yg_datum datum ;
    yg_node *up ; // this if root node
    yg_node *left, *right ;
    count_t height ;
  };
private:
  union {
    yg_datum leaf[LEAF_SIZE] ;
    struct US_inner inner ;
  } ;
  bool isinner() const;
  bool isleaf() const;
  count_t height() const;
  void checkinner() const; // also checks size and height calculation is OK
  void checkleaf() const; // also checks size sanity
  yg_node *next() const;
  yg_node *prev() const;
  
  void replug(yg_node *) ; // sets the proper up pointer
  void replace_with(yg_node *) ; // replugs new children; deletes old node
  
  void recalc() ;
  void rotate_left() ;
  void rotate_right() ;
  void avl() ;
  
  yg_node();
  yg_node(yg_node *parent,count_t siz,yg_datum contents[]);
  void freechildren() ;
  ~yg_node() ;
  
#ifdef SMART_NEW_DELETE
  static void* free_list ;
  static yg_node* raw_array ;
  static unsigned raw_length ;
  void *operator new(size_t s);
  void operator delete(void *);
#endif
  
  // debugging routines:
  void dumpname() {
    if ( isinner() )
      cerr << inner.datum.u ;
    else if ( size > 0 )
      cerr << leaf[0].u ;
    else
      cerr << '<' << (void*)this << '>' ;
  }
  void dump() {
    if ( isinner() ) {
      inner.right->dump() ;
    }
    for ( unsigned i = height() ; i<7 ; i++ )
      cerr << "  ";
    if ( isinner() ) {
      cerr << inner.datum.u << " - inner size " << size
           << " height " << height() << " - (" ;
      inner.left->dumpname() ;
      cerr << ',' ;
      inner.right->dumpname() ;
      cerr << ')' ;
    } else {
      for(unsigned i = 0 ; i < size ; i++ )
        cerr << leaf[i].u << ' ' ;
      cerr << '-' ;
    }
    cerr << " at " << (void*) this << endl ;
    if ( isinner() ) {
      inner.left->dump();
    }
  }
  void fullcheck() {
    if ( isleaf() ) {
      checkleaf();
    } else {
      checkinner() ;
      inner.right->fullcheck();
      inner.left->fullcheck();
    }
  }
} ;

void yg_tree::fullcheck() {
    if ( root )
        root->fullcheck();
    else {
        assert(smallsize == 0 || smallsize == 1);
    }
}
    
void yg_tree::dump() {
    if ( root ) {
        cerr << "yg_tree rooted at " << root << endl << endl ;
        root->dump() ;
    } else {
        cerr << "non-rooted yg_tree" << endl ;
    }
}

#ifdef YG_STATISTICS
static yg_tree *all_trees = NULL ;
#endif

// when the difference between a and b is at most 1, this is
// equivalent to max(a,b)+1
static inline count_t
hcombine(count_t a,count_t b)
{
    asserti( a == b || a == b+1 || a+1 == b );
    return (a+b+3)/2 ;
}

//-------------------------------------------------------------

inline bool
yg_node::isinner() const
{
    return up_leaf == NULL ;
}

inline bool
yg_node::isleaf() const
{
    return up_leaf != NULL ;
}

count_t
yg_node::height() const
{
    if ( isinner() )
        return inner.height ;
    else
        return 0 ;
}

inline void
yg_node::checkinner() const
{
    asserti( isinner() );
    asserti( inner.left->size + inner.right->size + 1 == size );
    asserti( hcombine(inner.left->height(),inner.right->height())
             == inner.height );
}

inline void
yg_node::checkleaf() const
{
    asserti( isleaf() );
    asserti( size <= LEAF_SIZE );
}

yg_node *
yg_node::next() const
{
    if ( isinner() ) {
        yg_node *r = inner.right ;
        while ( r->isinner() )
            r = r->inner.left ;
        return r ;
    } else {
        yg_node const *tail = this ;
        yg_node *head = up_leaf ;
        while(1) {
            assert( head != tail ); // else we were at the last leaf.
            head->checkinner();
            if ( tail == head->inner.left )
                return head ;
            asserti( tail == head->inner.right );
            tail = head ;
            head = head->inner.up ;
        }
    }
}

yg_node *
yg_node::prev() const
{
    if ( isinner() ) {
        yg_node *r = inner.left ;
        while ( r->isinner() )
            r = r->inner.right ;
        return r ;
    } else {
        yg_node const *tail = this ;
        yg_node *head = up_leaf ;
        while(1) {
            assert( head != tail ); // else we were at the last leaf.
            head->checkinner();
            if ( tail == head->inner.right )
                return head ;
            asserti( tail == head->inner.left );
            tail = head ;
            head = head->inner.up ;
        }
    }
}

void
yg_node::recalc()
{
    if ( isinner() ) {
        size = inner.left->size + inner.right->size + 1 ;
        inner.height = hcombine(inner.left->height(),inner.right->height()) ;
    }
}

void
yg_node::replug(yg_node *new_up)
{
    if ( isinner() )
        inner.up = new_up ;
    else
        up_leaf = new_up ;
}

yg_node::yg_node()
{
    up_leaf = this ;
    size = 0 ;
}

yg_node::yg_node(yg_node *parent, count_t siz, yg_datum contents[])
{
    asserti( parent != NULL );
    asserti( siz == 0 || contents != NULL );
    up_leaf = parent ;
    size = siz ;
    for( unsigned k = 0 ; k < siz ; k++ )
        leaf[k] = contents[k] ;
}

inline
yg_node::~yg_node()
{
    freechildren() ;
#ifndef NDEBUG
    size = 12345678 ;
    up_leaf = inner.up = inner.right = inner.left = (yg_node*)0xDEADBEEF ;
#endif
}

void
yg_node::replace_with(yg_node *org)
{
    checkinner();
    yg_node *parent = inner.up ;
    *this = *org ;
    if ( isinner() ) {
        inner.left->replug(this) ;
        inner.right->replug(this) ;
        inner.up = parent ;
    } else
        up_leaf = parent ;
    // pretend org is a leaf so its former children are not also deleted
    org->up_leaf = org ;
    delete org ;
}

void
yg_node::freechildren()
{
    if ( isinner() ) {
        delete inner.left ;
        delete inner.right ;
    }
}

//---------------------------------------------------------------

#ifdef SMART_NEW_DELETE

void *yg_node::free_list = NULL ;
yg_node *yg_node::raw_array ;
unsigned yg_node::raw_length ;

void *
yg_node::operator new(size_t s) {
    asserti(s == sizeof(yg_node));
    if ( free_list != NULL ) {
        void *r = free_list ;
        free_list = *(void**)free_list ;
        return r ;
    }
    if ( raw_length == 0 ) {
        raw_array = (yg_node*)malloc(100*sizeof (yg_node)) ;
        assert(raw_array != NULL);
        raw_length = 100 ;
    }
    raw_length-- ;
    return raw_array++ ;
}

void
yg_node::operator delete(void *p) {
    *(void**)p = free_list ;
    free_list = p ;
}

#endif

//---------------------------------------------------------------

inline void
yg_iterator::checkok() const
{
    assert( owner != NULL );
    assert( owner->version == version );
#ifndef NDEBUG
    if ( which == NULL ) {
        asserti( owner->root == NULL );
        asserti( where <= owner->smallsize );
    } else {
        which->checkleaf() ;
        asserti( where <= which->size );
    }
#endif
}

inline bool
yg_iterator::isinner() const
{
    checkok() ;
    return where == ( which ? which->size : owner->smallsize );
}

yg_iterator::yg_iterator(yg_tree const*o, yg_node *whi, unsigned whe)
{
    owner = (yg_tree*)o ;
    version = o->version ;
    which = whi ;
    where = whe ;
}

yg_iterator::yg_iterator()
{
    owner = NULL ;
}

bool
yg_iterator::valid() const
{
    if ( owner == NULL )
        return false ;
    checkok() ;
    if ( which == NULL )
        return where < owner->smallsize ;
    if ( where < which->size )
        return true ;
    yg_node *tail = which ;
    yg_node *head = tail->up_leaf ;
    while (1) {
        if ( head == tail )
            return false ;
        head->checkinner();
        if ( head->inner.right != tail ) {
            asserti( head->inner.left == tail );
            return true ;
        }
        tail = head ;
        head = tail->inner.up ;
    }
}

void
yg_iterator::operator++()
{
    if ( which != NULL && isinner() ) {
        which = which->next()->next() ;
        where = 0 ;
    } else
        where++ ;
}

void
yg_iterator::operator--()
{
    checkok();
    if ( where > 0 )
        where-- ;
    else {
        assert(which!=NULL);
        which = which->prev()->prev() ;
        where = which->size ;
    }
}

bool
yg_iterator::operator==(const yg_iterator &o) const
{
    checkok() ;
    o.checkok() ;
    assert(owner==o.owner);
    return which == o.which && where == o.where ;
}

yg_datum
yg_iterator::get() const
{
    if ( which==NULL ) {
        assert(where == 0);
        assert(owner->smallsize==1);
        return owner->single ;
    } else if ( isinner() )
        return which->next()->inner.datum ;
    else
        return which->leaf[where] ;
}

void yg_iterator::put(yg_datum datum) const
{
    if ( which==NULL ) {
        assert(where == 0);
        assert(owner->smallsize==1);
        owner->single = datum ;
    } else if ( isinner() )
        which->next()->inner.datum = datum ;
    else
        which->leaf[where] = datum ;
}

//-------------------------------------------------------------------

static unsigned long versioncounter = 0 ;

inline void
yg_tree::new_version()
{
    version = versioncounter++ ;
    if ( root != NULL )
        first = last = NULL ;
}

yg_tree::yg_tree()
{
    root = NULL ;
    smallsize = 0 ;
    new_version() ;
    #ifdef YG_STATISTICS
    nextlivetree = all_trees ;
    if ( nextlivetree )
        nextlivetree->prevlivetree = &nextlivetree ;
    prevlivetree = &all_trees ;
    all_trees = this ;
    #endif
    tracing = false ;
}

yg_tree::yg_tree(yg_datum datum)
{
    root = NULL ;
    smallsize = 1 ;
    single = datum ;
    new_version() ;
    #ifdef YG_STATISTICS
    nextlivetree = all_trees ;
    if ( nextlivetree )
        nextlivetree->prevlivetree = &nextlivetree ;
    prevlivetree = &all_trees ;
    all_trees = this ;
    #endif
    tracing = false ;
}

yg_tree::~yg_tree()
{
    clear() ;
    #ifdef YG_STATISTICS
    if ( nextlivetree )
        nextlivetree->prevlivetree = prevlivetree ;
    *prevlivetree = nextlivetree ;
    #endif
    tracing = false ;
    tracing = false ;
}

void
yg_tree::firstlastajour()
{
    asserti( root != NULL );
    if ( first != NULL ) return ;
    first = last = root ;
    while ( first->isinner() )
        first = first->inner.left ;
    while ( last->isinner() )
        last = last->inner.right ;
}

yg_iterator
yg_tree::begin() const
{
    if ( root == NULL )
        return yg_iterator(this,NULL,0) ;
    ((yg_tree*)this)->firstlastajour() ;
    return yg_iterator(this,first,0) ;
}

yg_iterator
yg_tree::end() const
{
    if ( root == NULL )
        return yg_iterator(this,NULL,smallsize);
    ((yg_tree*)this)->firstlastajour();
    return yg_iterator(this,last,last->size);
}

yg_iterator
yg_tree::index(count_t i) const
{
    if ( root == NULL ) {
        assert(i<=smallsize);
        return yg_iterator(this,NULL,i);
    }
    yg_node *node = root ;
    while ( node->isinner() )
        if ( i > node->inner.left->size ) {
            i -= node->inner.left->size + 1 ;
            node = node->inner.right ;
        } else
            node = node->inner.left ;
    assert(i<=node->size);
    return yg_iterator(this,node,i);
}

yg_iterator
yg_tree::find(yg_datum key,yg_lesseq lesseq) const
{
    if ( root == NULL ) {
        if ( smallsize == 0 || lesseq(key,single) )
            return yg_iterator(this,NULL,0);
        else
            return yg_iterator(this,NULL,1);
    }
    yg_node *node = root ;
    while ( node->isinner() )
        if ( lesseq(key,node->inner.datum) )
            node = node->inner.left ;
        else
            node = node->inner.right ;
    unsigned i = 0 ;
    while ( i < node->size && !lesseq(key, node->leaf[i]) )
        i++ ;
    return yg_iterator(this,node,i) ;
}

count_t
yg_tree::size() const
{
    return root ? root->size : smallsize ;
}

void
yg_tree::erase(yg_iterator &i)
{
    DOUT << "erase " << i.get().n->Numbered_ID << " in " << (void*) this ;
    i.checkok() ;
    assert( i.owner == this );
    if ( root == NULL ) {
        assert( i.where < smallsize );
        smallsize = 0 ;
        DOUT << '.' << endl ;
        return ;
    }
    yg_node *const node = i.which ;
    
    DOUT << "from (" << (void*)i.which << ',' << i.where << ") - size "
         << node->size << endl ;
    
    if ( node->size == 0 ) {
        // Case 1: deleting the inner element after an empty leaf node
        yg_node *const node2 = node->up_leaf ;
        assert( node2 != node ); // or we would be deleting in an empty list
        node2->checkinner();
        
        if ( node == node2->inner.left ) {
            DOUT << "the empty leaf is a left child" << endl ;
            // Case 1a: the empty leaf is a left child
            ++i ;
            if ( i.which == node2->inner.right ) i.which = node2 ;
            node2->replace_with(node2->inner.right) ;
        } else {
            asserti( node == node2->inner.right );
            DOUT << "the empty leaf is a right child" << endl ;
            // Case 1b: the empty leaf is a right child
            // Then the inner node we replace will not quite be the one to
            // delete; overwrite the one we *should* delete with that
            i.put(node2->inner.datum);
            ++i ;
            // and we are now sure that i does NOT point into the left subtree
            node2->replace_with(node2->inner.left) ;
        }
        delete node ;
        make_ajour(node2,i) ;
    } else if ( i.isinner() ) {
        DOUT << "deleting an inner element after a nonempty leaf" << endl ;
        // Case 2: deleting an inner element after a nonempty leaf
        i.where-- ;
        yg_datum tempdatum = i.get() ;
        node->size-- ;
        make_ajour(node,i); // have to do this here or the put will fail
        i.put(tempdatum) ;
        ++i ;
    } else {
        DOUT << "deleting a leaf element" << endl ;
        // Case 3: deleting a leaf element
        node->size-- ;
        for( unsigned k = i.where ; k < node->size ; k++ )
            node->leaf[k] = node->leaf[k+1] ;
        make_ajour(node,i);
    }
    DOUT << "deletion finished" << endl ;
}
    
void
yg_tree::insert(yg_iterator &i,yg_datum datum)
{
    DOUT << "insert " << datum.n->Numbered_ID << " in " << (void*) this ;
    i.checkok() ;
    assert( i.owner == this );
    if ( root == NULL ) {
        if ( smallsize == 0 ) {
            smallsize = 1 ;
            single = datum ;
            i.where = 1 ;
            DOUT << '.' << endl ;
            return ;
        }
        i.which = root = new yg_node();
        root->size = 1 ;
        root->leaf[0] = single ;
        first = last = root ;
    }
    yg_node *const node = i.which ;
    DOUT << " at (" << (void*)node << ',' << i.where << ") - size "
         << node->size ;
    if ( node->size < LEAF_SIZE ) {
        DOUT << " - enlarging" << endl ;
        // can enlarge leaf node
        for( unsigned k = node->size ; k > i.where ; k-- )
            node->leaf[k] = node->leaf[k-1] ;
        node->leaf[i.where] = datum ;
        node->size++ ;
        i.where++ ;
        make_ajour(node,i) ;
    } else {
        DOUT << " - splitting ";
        // we need to split the node
        yg_datum elements[LEAF_SIZE+1] ;
        for( unsigned k = 0 ; k < i.where ; k++ )
            elements[k] = node->leaf[k] ;
        elements[i.where] = datum ;
        for( unsigned kk = i.where+1 ; kk <= LEAF_SIZE ; kk++ )
            elements[kk] = node->leaf[kk-1] ;
        
        // const unsigned split = versioncounter % (LEAF_SIZE+1) ;
        unsigned split = LEAF_SIZE/2 ;
        if ( i.where == 0 ) {
            firstlastajour() ;
            if ( i.which == first )
                split = 0 ;
        } else if ( i.where == LEAF_SIZE ) {
            firstlastajour() ;
            if ( i.which == last )
                split = LEAF_SIZE ;
        }
        DOUT << split << endl ;
        
        yg_node *newleft = new yg_node(node,split,elements) ;
        DOUT << "insert: new left = " << (void*)newleft << endl ;
        yg_node *newright = new yg_node(node,LEAF_SIZE-split,elements+split+1);
        DOUT << "insert: new right = " << (void*)newright << endl ;
        node->inner.up = node->up_leaf ;
        node->up_leaf = NULL ;
        node->inner.left = newleft ;
        node->inner.right = newright ;
        node->inner.datum = elements[split] ;
        
        // set i to point after the newly-inserted element
        i.where++ ;
        if ( i.where <= newleft->size ) {
        i.which = newleft ;
        } else {
            i.where -= newleft->size + 1 ;
            i.which = newright ;
        }
        make_ajour(node,i) ;
    }
    DOUT << "insert ok" << endl ;
}

void
yg_tree::clear()
{
    if ( root != NULL )
        delete root ;
    root = NULL ;
    smallsize = 0 ;
    new_version() ;
}

void
yg_tree::splice(yg_iterator &i,yg_tree &o)
{
    for ( yg_iterator j = o.begin() ; !( j == o.end() ) ; ++j )
        insert(i,j.get()) ;
    o.clear() ;
}

//--------------------------------------------------------------
// make_ajour not only keeps the size fields ajour, it also keeps
// the tree balanced with the AVL rotation algorithm.
//
// Since the rotations only affect inner nodes, the balancing
// operation does not in principle affect the validity of
// iterators. However, the insert and delete at the leaf level
// that precedes the call to make_ajour may have invalidated
// other iterators; so we might as well assign make_ajour the
// duty of invalidating all other iterators than the given one.

#define checknode(n) if(n->isinner()) n->checkinner(); else n->checkleaf();

void
yg_node::rotate_left()
{
    asserti( isinner() );
    asserti( inner.right->isinner() );
    yg_node *const node2 = inner.right ;
    yg_node *const tree1 = inner.left ;
    yg_node *const tree2 = node2->inner.left ;
    yg_node *const tree3 = node2->inner.right ;
    
    yg_datum temp = inner.datum ;
    inner.datum = node2->inner.datum ;
    node2->inner.datum = temp ;
    
    node2->inner.left = tree1 ; tree1->replug(node2) ;
    node2->inner.right = tree2 ; // already plugged, just changing side
    inner.left = node2 ; // already plugged, just changing side
    inner.right = tree3 ; tree3->replug(this) ;
    
    node2->recalc() ;
    // The rotate operation itself should *not* recalc; the first
    // of a double rotation leaves an improperly balanced tree.
    // Rather the caller takes responsibility to eventually recalc.
}
  
void
yg_node::rotate_right()
{
    asserti( isinner() );
    asserti( inner.left->isinner() );
    yg_node *const node2 = inner.left ;
    yg_node *const tree1 = inner.right ;
    yg_node *const tree2 = node2->inner.right ;
    yg_node *const tree3 = node2->inner.left ;
    
    yg_datum temp = inner.datum ;
    inner.datum = node2->inner.datum ;
    node2->inner.datum = temp ;
    
    node2->inner.right = tree1 ; tree1->replug(node2) ;
    node2->inner.left = tree2 ; // already plugged, just changing side
    inner.right = node2 ; // already plugged, just changing side
    inner.left = tree3 ; tree3->replug(this) ;
    
    node2->recalc() ;
    // The rotate operation itself should *not* recalc; the first
    // of a double rotation leaves an improperly balanced tree.
    // Rather the caller takes responsibility to eventually recalc.
}

inline void
yg_node::avl()
{
    yg_node *const left = inner.left ;
    yg_node *const right = inner.right ;
    if ( left->height() + 1 < right->height() ) {
        // right subtree too tall
        if ( right->inner.right->height() != right->height() - 1 )
            right->rotate_right() ;
        rotate_left() ;
    } else if ( right->height() + 1 < left->height() ) {
        // left subtree too tall
        if ( left->inner.left->height() != left->height() - 1 )
            left->rotate_left() ;
        rotate_right() ;
    }
    recalc() ;
}

void
yg_tree::make_ajour(yg_node *n, yg_iterator &i)
{
    DOUT << "make_ajour at " << (void*)n << endl;
    for ( yg_node *now = NULL ; now != n ; ) {
        now = n ;
        if ( now->isleaf() )
            n = now->up_leaf ;
        else {
            now->avl() ;
            n = now->inner.up ;
        }
    }

    new_version() ;
    i.version = version ;
}


// **********************************************************************
// **********************************************************************

static bool
Pset_lesseq(yg_datum x,yg_datum y)
{
    if ( x.n->Numbered_ID == y.n->Numbered_ID )
        return x.n->Numbered_Sort <= y.n->Numbered_Sort ;
    else
        return x.n->Numbered_ID <= y.n->Numbered_ID ;
}

bool
Pset_insert(yg_tree &T,Numbered *d)
{
    yg_iterator I = T.find(yg_n(d),Pset_lesseq) ;
    if ( I.valid() && I.get().n == d )
        return true ;
    else {
        if ( T.tracing && debug )
            debug << "Pset::" ;
        T.insert(I,yg_n(d)) ;
        return false ;
    };
}

yg_iterator
Pset_find(const yg_tree &T,Numbered *d)
{
    yg_iterator I = T.find(yg_n(d),Pset_lesseq) ;
    if ( I.valid() && I.get().n == d )
        return I ;
    else
        return T.end() ;
}

void
Pset_erase(yg_tree &T,Numbered *d)
{
    yg_iterator I = T.find(yg_n(d),Pset_lesseq) ;
    assert( I.get().n == d );
    if ( T.tracing && debug )
        debug << "Pset::" ;
    T.erase(I) ;
}  

// returns true if A U B = A.
bool
Pset_union(yg_tree &A,const yg_tree &B)
{
    bool changed = false;
    if ( &A == &B )
        return !changed;
    // add those elements in B that do not appear in A, to A
    yg_iterator ia = A.begin() ;
    yg_iterator ib = B.begin() ;
    while ( ia.valid() && ib.valid() ) {
        if ( ia.get().n == ib.get().n ) {
            // this element is in A and B
            ++ia, ++ib ;
        } else if ( Pset_lesseq(ia.get(),ib.get()) ) {
            // the front of A is not in B
            ++ia;
        } else {
            // the front of B is not in A
            A.insert(ia,ib.get()) ;
            ++ib;
            changed = true;
        }
    }
    // if there are elements left in B after stepping through
    // all of A, the rest must be added
    while ( ib.valid() ) {
        A.insert(ia,ib.get());
        ++ib ;
        changed = true;
    }
    return !changed;
}
              
// returns true if A /\ B = A.
bool
Pset_intersect(yg_tree &A,const yg_tree &B)
{
    bool changed = false;
    if ( &A == &B )
        return !changed;
    // remove all elements from A that do not appear in B
    yg_iterator ia = A.begin() ;
    yg_iterator ib = B.begin() ;
    while ( ia.valid() && ib.valid() ) {
        if ( ia.get().n == ib.get().n ) {
            // this element is in A and B
            ++ia, ++ib ;
        } else if ( Pset_lesseq(ia.get(),ib.get()) ) {
            // the front of A is not in B
            changed = true;
            A.erase(ia);
        } else {
            // the front of B is not in A
            ++ib;
        }
    }
    // if there are elements left in A after stepping through
    // all of B, the rest must be erased
    while ( ia.valid() ) {
        changed = true;
        A.erase(ia);
    }
    return !changed;
}
    
// returns true if A \ B = A.
bool
Pset_minus(yg_tree &A,const yg_tree &B)
{
    bool changed = false;
    if ( &A == &B ) {
        A.clear() ;
        return !changed;
    }
    // remove all elements from A that appear in B
    yg_iterator ia = A.begin() ;
    yg_iterator ib = B.begin() ;
    while ( ia.valid() && ib.valid() ) {
        if ( ia.get().n == ib.get().n ) {
            // this element is in A and B
            A.erase(ia);
            ++ib ;
            changed = true;
        } else if ( Pset_lesseq(ia.get(),ib.get()) ) {
            // the front of A is not in B
            ++ia;
        } else {
            // the front of B is not in A
            ++ib;
        }
    }
    return !changed;
}

// **********************************************************************
// **********************************************************************

static bool
Nset_lesseq(yg_datum x,yg_datum y)
{
    return x.u <= y.u ;
}

bool
Nset::insert(unsigned d)
{
    yg_iterator I = Y.find(yg_u(d),Nset_lesseq) ;
    if ( I.valid() && I.get().u == d )
        return false ;
    else {
        Y.insert(I,yg_u(d)) ;
        return true ;
    };
}

bool
Nset::contains(unsigned d)
{
    yg_iterator I = Y.find(yg_u(d),Nset_lesseq) ;
    return I.valid() && I.get().u == d ;
}

#ifdef YG_STATISTICS

void ygtree_statistics() {
    unsigned counts[20] ;
    unsigned norootcounts[20] ;
    unsigned total = 0 ;
    for ( int i = 0 ; i < 20 ; i++ )
        norootcounts[i] = counts[i] = 0 ;
    for ( yg_tree *dive = all_trees ; dive ; dive = dive->nextlivetree ) {
        unsigned s = dive->size() ;
        if ( s >= 10 )
            s = 9 + s/10 ;
        if ( s >= 20 )
            s = 19 ;
        counts[s]++ ;
        if ( dive->root == NULL )
            norootcounts[s]++;
        total++ ;
    }
    cerr << "ygtree statistics (" << total << " trees in total):" << endl ;
    for ( int i = 0 ; i < 20 ; i++ ) {
        cerr << counts[i] << " trees with " ;
        if ( i < 10 )
            cerr << i ;
        else if ( i < 19 )
            cerr << (i-9)*10 << '-' << (i-9)*10+9 ;
        else
            cerr << "more" ;
        cerr << " elements" ;
        if ( norootcounts[i] )
            cerr << " (" << norootcounts[i] << " with no nodes)" ;
        cerr << endl ;
    }
}

#endif
