/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Quasi-persistent stackable list.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __ListStack__
#define __ListStack__

#include <stdlib.h>
#include "auxilary.h"
#include "Plist.h"

template <class T>
class ListStack
{
    int  height;
    int  maxsize;
    Plist<T>** elements;
public:
    ListStack(void) : height(0), maxsize(1)
        {
	    elements = new Plist<T>*[1];
            elements[height] = new Plist<T>;
        }

    void enter(void)
    {
	height++;
        // Check whether there is is room for a new element.
	if (height >= maxsize) {
	    // Make a new stack that is twice as big as the old.
            int newSize = 2*maxsize;
	    Plist<T>** new_stack = new Plist<T>*[newSize];
	    int i;
            // Copy the old elements to the new stack.
	    for (i = 0; i < maxsize; i++) new_stack[i] = elements[i];
            // Initialize the rest.
	    for (; i < newSize; i++) new_stack[i] = NULL;
            // Clean up.
	    delete elements;
	    maxsize = newSize;
	    elements = new_stack;
	}
        // Make an empty list.
	elements[height] = new Plist<T>();
    }

    void leave(void)
    {
	assert(height > 0);
        assert(elements[height]!=NULL);
        delete elements[height];
        elements[height] = NULL;
	height--;
    }

    void push_front(T* e) { elements[height]->push_front(e); }
    void push_back(T* e) { elements[height]->push_back(e); }
    void push_back_global (T* e) { elements[0]->push_back(e); }
    Plist<T>* get_list(void)
        {
            Plist<T>* tmp = elements[height];
            elements[height] = new Plist<T>();
            return tmp;
        }
    int size(void) { return elements[height]->size(); }
        
};

#endif
