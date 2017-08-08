/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  Binding-time constraint solver
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "auxilary.h"
#include "bta.h"

//////////////////////////////////////////////////////////////////////

BTpairlist::iterator::iterator(Plist<BTvariable>::iterator vv,
                               Plist<BTcause>::iterator cc)
    : v(vv), c(cc)
{
}

void
BTpairlist::iterator::operator++(int)
{
    v++ ;
    c++ ;
}

BTpairlist::iterator::operator bool()
{
    return v ;
}

BTvariable *
BTpairlist::iterator::variable()
{
    return *v ;
}

BTcause *
BTpairlist::iterator::cause()
{
    return *c ;
}

BTpairlist::iterator
BTpairlist::begin()
{
    return iterator(v.begin(),c.begin());
}

bool
BTpairlist::empty()
{
    return v.empty() ;
}

void
BTpairlist::push_back(BTvariable *vv,BTcause *cc)
{
    v.push_back(vv);
    c.push_back(cc);
}

void
BTpairlist::clear()
{
    v.clear();
    c.clear();
}

//////////////////////////////////////////////////////////////////////

BTobject::BTobject()
    : Numbered(Numbered::BTOBJECT)
{
}

//////////////////////////////////////////////////////////////////////

void
BTvariable::influence(BTvariable *v,BTcause *c)
{
    if ( v == this )
        return ;
    waiting.push_back(v,c);
    c->refcount++ ;
}

void
BTvariable::flood(Status s) {
    // This floods the given status to any untouched noded reachable
    // by a breadth-first graph traversal
    
    assert(status==Untouched);
    status = Pending ;
    Plist<BTvariable> pending(this) ;
    while ( !pending.empty() ) {
        BTvariable *current = pending.front() ;
        pending.pop_front() ;
        assert(current->status == Pending);
        current->status = s ;
        foreach(i,current->waiting,BTpairlist)
            switch(i.variable()->status) {
            case Untouched:
                i.variable()->status = Pending ;
                pending.push_back(i.variable());
                // fall through
            case Pending:
                i.variable()->causes.push_back(current,i.cause()) ;
                break ;
            default:
                // already marked. Delete the constraint
                i.cause()->refcount-- ;
                break ;
            }
        current->waiting.clear() ;
    }
}

//////////////////////////////////////////////////////////////////////

Plist<BTcause> BTcause::all_causes ;

BTcause::BTcause()
{
    refcount = 0 ;
    all_causes.push_back(this) ;
}

void
BTcause::remove_unused()
{
    Plist<BTcause>::mutable_iterator i = all_causes.begin() ;
    while ( i )
        if ( i->refcount )
            i++ ;
        else {
            delete (BTcause*)*i ;
            all_causes.erase(i);
        }
}
