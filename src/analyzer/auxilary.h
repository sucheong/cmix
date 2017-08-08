/* (-*-c++-*-)
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 * Content:  C-Mix system: aux declarations.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 *                    Jens Peter Secher
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __aux__
#define __aux__

#ifndef __CMIX_CONFIGURE__
#error Include <cmixconf.h> before auxilary.h
#endif
#include <iostream.h>

extern char CmixRelease[] ; // in release.cc
extern char CPP[] ;             // in varstrings.cc or bindist.cc
extern char CMIX_SHADOW_DIR[] ; // in varstrings.cc or bindist.cc

char* stringDup(const char* str);       // Duplicate a string (dynamically).
char* long2a(long);                     // Make a string out of a long.
        
#define foreach(i,lst,type) \
        for(type::iterator i((lst).begin()); i ; i++)

#define double_foreach(i,j,ilst,jlst,type) \
        for(type::iterator i((ilst).begin()), j((jlst).begin()); \
            i && j; i++, j++)

void assert_error(char const*file, unsigned line, char const*expr);
#define assert(e) do{if(!(e))assert_error(__FILE__,__LINE__,#e);}while(0)

//----------------------
// class Numbered
// This class is used as an abstract base class for classes T that need
// to be put in a set or used as domain for a map. Each object has a
// unique identifier that can be used as an index in arrays. It is
// also used for comparison in sets.
//
// The sort argument to the constructor should be different for each
// Numbered deriviate. Each sort has its own series of unique identifiers.

class Numbered {
public:
  enum Sort {
    ALOCSET,
    FCNSET,
    INVALOCSET,
    BTOBJECT,
    C_DECL,
    C_EXPR,
    C_JUMP,
    C_STMT,
    C_BASICBLOCK,
    C_ENUMDEF,
    C_USERDEF,
    C_USERMEMB,
    OTYPE,
    FIXPOINTVERTEX,
    C_TYPE,
    PA_SET,
    NO_SORT
    // NO_SORT *must* be the last constant, its value equal to the
    // number of actually used constants.
  };
private:
  Numbered(); // must not be called
protected:
  Numbered(Sort);
public:
  const unsigned long Numbered_ID ;
  const Sort Numbered_Sort ;
  Numbered(const Numbered&);
};
  
#if !defined(NULL)
#define NULL 0 // sigh, ((void*)0) makes g++ complain at places
#endif

#ifndef HAVE_LTOA
char *ltoa(long int l);			// Convert a long to a string
#endif

#endif /* __aux__ */
