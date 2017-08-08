/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Fixpoint solver, version 2
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "auxilary.h"
#include "ygtree.h"
static bool Fixpoint_lesseq(yg_datum X,yg_datum Y);
#include "fixiter.h"

FixpointPureVertex::FixpointPureVertex()
    : Numbered(Numbered::FIXPOINTVERTEX)
{
    owner = NULL ;
    is_ajour = false ;
}

bool
FixpointVertex::operator >>(FixpointPureVertex &v)
{
    return successors.insert(&v) ;
}

void
FixpointVertex::Solve(FixpointSolver &fps)
{
  if ( solve(fps) ) {
    // the solution step made a difference. Propagate
    foreach(succ,successors,Pset<FixpointPureVertex>)
      fps += *succ ;
  }
}

FixpointPureVertex::~FixpointPureVertex()
{
}

FixpointVertex::~FixpointVertex()
{
}

FixpointSolver::FixpointSolver()
{
    clock = 0 ;
}

static bool
Fixpoint_lesseq(yg_datum X,yg_datum Y)
{
    return ((FixpointPureVertex*)X.n)->stamp
        <= ((FixpointPureVertex*)Y.n)->stamp ;
}

bool
FixpointSolver::operator += (FixpointPureVertex *fo)
{
    if ( fo->owner == NULL ) {
        fo->owner = this ;
        fo->stamp = 0 ;
    } else {
        assert( fo->owner == this );
        if ( !fo->is_ajour )
            return true; // already in the worklist
    }
    fo->is_ajour = false ;

    yg_datum ygfo = yg_n(fo) ;
    yg_iterator I = worklist.find(ygfo,Fixpoint_lesseq);
    worklist.insert(I,ygfo) ;
    return false; // now in the worklist
}
    
void
FixpointSolver::solve()
{
    while ( worklist.size() != 0 ) {
        yg_iterator I = worklist.begin() ;
        FixpointPureVertex *fo = (FixpointPureVertex*)I.get().n ;
        worklist.erase(I) ;
        
        fo->stamp = ++clock ;
        fo->is_ajour = true ; // meaning it is not in the worklist
        fo->Solve(*this);
    }
}
