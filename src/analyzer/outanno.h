/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Definitions of Core C annotators
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __OUTANNO__
#define __OUTANNO__

#include "outcore.h"
#include "analyses.h"

class OutPa : public CoreAnnotator {
    PAresult const&data ;
    array<ALocSet,Anchor*> exp_cached ;
    array<ALocSet,Anchor*> lval_cached ;
    void declrec(C_Decl &d,Plist<Output> *,Outcore *);
 public:
    OutPa(PAresult const&d, CoreAnnotator *nxt);
    virtual void operator()(C_Decl&,		Plist<Anchor>&,Outcore*);
    virtual void operator()(C_Expr&,bool lval,	Plist<Anchor>&,Outcore*);
}; // implemented in outpa.cc

class OutCg : public CoreAnnotator {
    CGresult const&data ;
    array<ALocSet,Anchor*> called_cached ;
 public:
    OutCg(CGresult const&d, CoreAnnotator *nxt);
    virtual void operator()(C_Decl&,		Plist<Anchor>&,Outcore*);
}; // implemented in outmisc.cc

class OutTrulyLocal : public CoreAnnotator {
    PAresult const&data ;
    Anchor *NotLocal ;
 public:
    OutTrulyLocal(PAresult const&d, CoreAnnotator *nxt);
    virtual void operator()(C_Decl&,		Plist<Anchor>&,Outcore*);
}; // implemented in outmisc.cc

class BTvariable ;
class BTobject ;
class OutBt : public CoreAnnotator {
    BTresult const&data ;
    Anchor *IsSpectime ;
    array<BTvariable,Anchor*> anc_cache ;
    array<BTobject,Output*> name_cache ;
    Plist<BTvariable> pending ;

    Anchor *varanc(BTvariable *);
    Output *findname(BTobject *, Outcore *oc);
    
    void explain(BTvariable *explain_this,Plist<Output> &causestack,
                 Pset<BTvariable> &already_met, Plist<Output> *dest,
                 Outcore *oc);
    void produce(BTvariable*,Outcore*);
    void decorate(BTvariable*,Plist<Anchor>&,Outcore*);
public:
    OutBt(BTresult const&d, CoreAnnotator *nxt);
    virtual void operator()(C_Type&,		Plist<Anchor>&,Outcore*);
    virtual void operator()(C_Decl&,		Plist<Anchor>&,Outcore*);
    virtual void operator()(C_UserMemb&,C_Type&,Plist<Anchor>&,Outcore*);
    virtual void operator()(C_Stmt&,		Plist<Anchor>&,Outcore*);
    virtual void operator()(C_Jump&,		Plist<Anchor>&,Outcore*);
    virtual void operator()(C_BasicBlock&,	Plist<Anchor>&,Outcore*);
    virtual void operator()(C_Expr&,bool lval,	Plist<Anchor>&,Outcore*);
}; // implemented in outbta.cc

class OutDf : public CoreAnnotator {
    DFresult const&data ;
 public:
    OutDf(DFresult const&, CoreAnnotator *nxt);
    virtual void operator()(C_Decl&,		Plist<Anchor>&,Outcore*);
    virtual void operator()(C_BasicBlock&,	Plist<Anchor>&,Outcore*);
    virtual void operator()(C_Stmt&,		Plist<Anchor>&,Outcore*);
    virtual void operator()(C_Jump&,      	Plist<Anchor>&,Outcore*);
}; // implemented in dataflow.cc

class OutSa : public CoreAnnotator {
    SAresult const&data ;
    Anchor *Shareable ;
    Anchor *Unshareable ;
 public:
    OutSa(SAresult const&d, CoreAnnotator *nxt);
    virtual void operator()(C_Decl&,            Plist<Anchor>&,Outcore*);
}; // implemented in outmisc.cc


#endif
