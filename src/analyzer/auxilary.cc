/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 *           Peter Holst Andersen
 * Content:  C-Mix system: Miscellaneous functions
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdio.h>
#include <iostream.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include "auxilary.h"
#include "diagnostic.h"

#if OVERLOAD_NEW_DELETE

// Debugging versions of storage allocation operators

static const int extra_words = sizeof(long double)/sizeof(unsigned) ;
// the size, in words, of an allocation header. We hope that something
// that is properly aligned for a long double will be properly aligned
// for anything, so that we won't lose alignment by adding this number
// of words at the front of each block

static const unsigned NEWMAGIC    = 0xFEED0000u ;
static const unsigned DELETEMAGIC = 0xDEAD0000u ;
// see below for BLOCKMAGIC
// When memory is allocated, it is filled with a sequential stamp that
// starts at NEWMAGIC - when it is deallocated it is filled with a
// sequential stamp that starts at DELETEMAGIC. On our development
// system these stamps point outside of the allocated memory when
// interpreted as addresses, so this immediately gives us a segfault
// if the code tries to use uninitialized pointers or to read pointer
// values from a block after it has been deallocated. The fact that
// the stamps are different for each new and delete allows one to
// conditionally breakpoint on the exact operation when rerunning
// the program.

static const unsigned BLOCKMAGIC  = 0xA10C0000u ;
static const unsigned BLOCKLIMIT = 100000 ;
// BLOCKLIMIT is the maximum number of words that can be allocated in
// a single new. The actual number of allocated words is added to
// BLOCKMAGIC to form a block header.
//   When a deletion is attempted this computation is done backwards,
// and if the thing we attempted to deallocate was not in fact an
// allocation we'll probably end up with a ridiculously huge number
// of words here. Thus false deletes can be detected at run time.
//   The reason why a limit exists is that if we allowed *any*
// number of words to be allocated at one time there would be no
// way to know that a block header of, say, 0xDEAD0004 wasn't
// simply the start of a block that happended to span 1033961476
// words.

void*
operator new(size_t u)
{
    assert(extra_words*sizeof(unsigned) == sizeof(long double));
    size_t words = ( u + (sizeof(unsigned) - 1) ) / sizeof(unsigned)
                   + extra_words ;
    assert(words < BLOCKLIMIT);
    unsigned *block = (unsigned*)malloc(words*sizeof(unsigned)) ;
    assert(block != NULL);
    static unsigned stamp = NEWMAGIC ;
    *block = BLOCKMAGIC + words ;
    while(words>1) block[--words] = stamp ;
    stamp++ ;
    return block + extra_words ;
}
void*
operator new[](size_t u)
{
  return operator new(u);
}

void
operator delete(void*vp)
{
    if ( vp == NULL )
        return ;
    static unsigned stamp = DELETEMAGIC ;
    unsigned *block = (unsigned*)vp - extra_words ;
    size_t words = *block - BLOCKMAGIC ;
    assert(words < BLOCKLIMIT);
    while(words--)
        block[words] = stamp ;
    free(block);
    stamp++ ;
}
void
operator delete[](void*vp)
{
    ::operator delete(vp);
}

#elif 0

void*
operator new(size_t u)
{
    return malloc(u);
}
void
operator delete(void*vp)
{
    free(vp);
}
void
operator delete[](void*vp)
{
    free(vp);
}

#endif


// Our own assert handler. The built-in one seems to confuse gdb.

void
assert_error(char const*file, unsigned line, char const*expr)
{
    Diagnostic d(INTERNAL,Position(file,line)) ;
    d << "assertation failed:" ;
    d.addline() << expr ;
}

// Make a dynamically allocated copy of a string.
char* stringDup(const char* str)
{
    if (str == NULL) return NULL;
    int len = strlen(str);
    char* newStr = new char[len+1];
    assert(newStr!=NULL);
    strcpy(newStr,str);
    return newStr;
}

char* long2a(long i)
{
    // Make a literate string out of the number.
    long len = (i==0 ? 1 : (long)log10((double)i) + 1);
    char *literal = new char[len + 1];
    sprintf(literal, "%ld", i);
    return literal;
}

static unsigned long the_numbers[Numbered::NO_SORT] = {0} ;

static unsigned long
numinit(Numbered::Sort s)
{
    assert((unsigned)s < Numbered::NO_SORT);
    return ++the_numbers[s] ;
}
    
Numbered::Numbered(Sort s)
    : Numbered_ID(numinit(s)), Numbered_Sort(s)
{
}

Numbered::Numbered(const Numbered &o)
    : Numbered_ID(numinit(o.Numbered_Sort)), Numbered_Sort(o.Numbered_Sort)
{
}

#ifndef HAVE_LTOA
char *ltoa(long x)
{
    static char str[30];
    char *p = str;
    long y = x;

    if (y < 0) {
	*p++ = '-';
	y = -y;
    }

    do {
	p++;
	y = y / 10;
    } while (y > 0);

    *p = '\0';
    do {
	*--p = '0' + (x % 10);
	x = x / 10;
    } while (x > 0);

    return str;
}
#endif
