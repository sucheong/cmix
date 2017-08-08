/* (this is -*- c++ -*- code)
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: symboltable used while parsing. 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __SYMBOLTABLE__
#define __SYMBOLTABLE__

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "auxilary.h"
#include "diagnostic.h"

#define SYMBOLTABLESIZE 29      

template <class T>
class SymbolTable
{
    // A Bucket is a list of items.
    struct Bucket
    {
	const char *id;
	T *elem;
	Bucket *prev, *next;	// Double linked list

      public:
	Bucket(const char *i, T *e, Bucket *n)
	{ id = i; elem = e; next = n; if (n) n->prev = this; prev = NULL; }

	~Bucket(void) {
            delete next;
        }

	Bucket *copy(void)
	{
	    return new Bucket(id, elem, next ? next->copy() : (Bucket *)NULL);
	}
    };
      
    Bucket *table[SYMBOLTABLESIZE];

    int hash(const char *id)
    { 
	int i = 0;
	  
	while (*id) i += *id++;
	return i % SYMBOLTABLESIZE;
    }

  public:
    SymbolTable(void)
    {
	for (int n = 0; n < SYMBOLTABLESIZE; n++)
	    table[n] = NULL;
    }
    SymbolTable(Bucket **tbl)
    {
	for (int i = 0; i < SYMBOLTABLESIZE; i++)
	    table[i] = tbl[i] ? tbl[i]->copy() : (Bucket *)NULL;
    }
    ~SymbolTable(void)
    {
	for (int i = 0; i < SYMBOLTABLESIZE; i++)
            if (table[i])
                delete table[i];
    }

    void clear(void)
    {
	for (int i = 0; i < SYMBOLTABLESIZE; i++) {
            if (table[i])
                delete table[i];
            table[i] = NULL;
        }
    }

    T *lookup(const char *id)
    {
	for (Bucket *b = table[hash(id)]; b; b = b->next)
	    if (!strcmp(b->id, id))
		return b->elem;
	return NULL;
    }

    void insert(const char *id, T *elem)
    {
	assert(elem != NULL);
    
	int n = hash(id);
	table[n] = new Bucket(id, elem, table[n]);
    }

    void remove(const char *id)
    {
	assert(id != NULL);

	int n = hash(id);
	for (Bucket *b = table[n]; b; b = b->next) {
	    if (!strcmp(b->id, id)) {
		// Remove the bucket from the doubly linked list.
		if (b->next != NULL)
		    b->next->prev = b->prev;
		if (b->prev != NULL)
		    b->prev->next = b->next;
		else
		    table[n] = b->next;
		// Delete the bucket.
		b->next = NULL;
		b->prev = NULL;
		delete b;
		return;
	    }
	}
	
        Diagnostic(FATAL,Position()) << "Couldn't remove " << id
                                     << " from symbol table" ;
    }

    SymbolTable<T> *copy(void) { return new SymbolTable(table); }
};


template <class T> class Scope
{
    int scope;
    SymbolTable<T>** symtables;
    int table_size;
    T* lastAdded;

  public:
    Scope(void)
    {
	scope = -1;
	symtables = NULL;
        lastAdded = NULL;
	table_size = 0;
	enter_scope();
    }

    ~Scope(void)
    {
        if ( symtables ) {
            for (int i = 0; i < table_size; i++)
                delete symtables[i];
            delete symtables;
        }
    }

    void restart(void)
    {
        assert(scope == 0);
        for (int i = 0; i < table_size; i++) {
            if (symtables[i]) delete symtables[i];
            symtables[i] = NULL;
        }
	symtables[0] = new SymbolTable<T>;
    }        

    void enter_scope(void)
    {
	scope++;
        // Check whether there is is room for a new table.
	if (scope >= table_size) {
	    // Make a new set of symboltables that is twice as big as the old
            int newSize = (table_size==0 ? 1 : 2*table_size);
	    SymbolTable<T> **new_tbls = new SymbolTable<T> * [newSize];
	    int i;
            // Copy the old tables to the new one.
	    for (i = 0; i < table_size; i++)
		new_tbls[i] = symtables[i];
            // Initialize the rest.
	    for (; i < newSize; i++)
		new_tbls[i] = NULL;
	    table_size = newSize;
	    if (symtables) delete symtables;
	    symtables = new_tbls;
	}
	symtables[scope] = new SymbolTable<T>;
    }

    void leave_scope(void)
    {
	assert(scope > 0);
        assert(symtables[scope]!=NULL);
        delete symtables[scope];
        symtables[scope] = NULL;
	scope--;
    }

    inline T *lookup(const char *id) { return lookup(id, scope); }

    T *lookup(const char *id, int i)
    {
	T *elem;
	for(; i >= 0; i--) {
	    elem = symtables[i]->lookup(id);
	    if (elem)
		return elem;
	}
	return NULL;
    }

    T *lookup_intermediate(int level, const char *id) {
        return symtables[level]->lookup(id);
    }
    

    inline T *lookup_local(const char *id)
        { return symtables[scope]->lookup(id); }
    inline T *lookup_global(const char *id)
        { return symtables[0]->lookup(id); }
    inline void insert(char const *id, T *elem)
        { lastAdded=elem; insert(id, elem, scope); }
    inline void insert(char const *id, T *elem, int i)
        {  lastAdded=elem; symtables[i]->insert(id, elem); }

    inline void remove(const char *id) { remove(id, scope); }
    inline void remove(const char *id, int i) { symtables[i]->remove(id); }

    inline int get_level(void) { return scope; }
    inline T* last_added() { return lastAdded; }
    
    inline SymbolTable<T> *get_table() { return symtables[scope]->copy(); }
};


#endif /* __SYMBOLTABLE__ */
