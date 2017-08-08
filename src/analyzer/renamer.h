/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: name management
 *           This file defined the internal 
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __RENAMER__
#define __RENAMER__

#include "symboltable.h"
#include "Pset.h"

//-------------------------------------------------------
//  RENAMER                                    renamer.cc
//
// This module creates a mapping that gives new names to
// every C_Decl, C_UserDef, C_EnumDef and C_UserMemb in
// the program. It also optionally creates a set of
// C_UserDef's that must keep their name in the residual
// program.

class BTresult ;
class C_Pgm ;
class C_UserDef ;
struct NameMap : public multiArray<const char*> {
  NameMap();
};
void MakeUniqueNames(C_Pgm const&,BTresult const&,
                     NameMap&,Pset<C_UserDef>*);

// Gegen uses MakeUniqueNames to create names for the generating
// extension, and the interface below to create a nametable for
// the residual program

class NameMgr {
protected:
    struct NameBase {
        char const *base ;
        unsigned index ;
        unsigned firstseq ;
    };
    
    SymbolTable<NameBase> symtab ;
    Plist<NameBase> list ;
    typedef void (*cooker)(char*) ;
private:
    static char split_alpha[] ;
    static unsigned split_num ;
    void split_internal(char const*,cooker=NULL);
protected:
    void request_internal(char const*,NameBase *&,unsigned &,cooker=NULL);
public:
    NameMgr() ;
    ~NameMgr() ;

    // all taboos must be entered *after* proper names have been
    // requested.
    void add_taboo(char const*);
    void add_taboos(char const*const[]);
};

struct ResNameMgr : public NameMgr {
    ResNameMgr() ;
    char const *request(char const*) ; // returns a pointer to static storage
    void emittable(ostream &);
    unsigned get_size() { return list.size(); }
};

#endif /* __RENAMER__ */
