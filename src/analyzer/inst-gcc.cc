#line 1 "./inst-gcc.org"
/* -*-fundamental-*-
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Template instantiations, lists and own stuff
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "Plist.h"
#include "Pset.h"
#include "liststack.h"
#include "symboltable.h"
#include "array.h"
#include "cpgm.h"       // for Numbered deriviations
#include "corec.h"	// for Numbered deriviations
#include "fixiter.h"	// for Numbered deriviations
#include "directives.h" // for GeneratorDirective::Param
#include "bta.h"        // for BType::Cause
#include "commonout.h"  // for outputInterList
#include "renamer.h"    // for NameMgr::NameBase
#include "pa.h"         // for (indirect) Numbered deriviations
#include "typearr.h"
class PredConstraint;


class   Anchor; template class Plist< Anchor >;
class   AnyAtom; template class Plist< AnyAtom >;
class   C_BasicBlock; template class Plist< C_BasicBlock >;
class   C_Decl; template class Plist< C_Decl >;
class   C_Expr; template class Plist< C_Expr >;
class   C_Stmt; template class Plist< C_Stmt >;
class   C_Type; template class Plist< C_Type >;
class   C_UserDecl; template class Plist< C_UserDecl >;
class   Expr; template class Plist< Expr >;
class   FunDef; template class Plist< FunDef >;
class   GeneratorDirective; template class Plist< GeneratorDirective >;
 template class Plist< GeneratorDirective::Param >;
class   InUseConstraint; template class Plist< InUseConstraint >;
class   Init; template class Plist< Init >;
class   MemberDecl; template class Plist< MemberDecl >;
class   OType; template class Plist< OType >;
class   Output; template class Plist< Output >;
class   Parse_Postfix; template class Plist< Parse_Postfix >;
class   SourceDirective; template class Plist< SourceDirective >;
class   SepNode; template class Plist< SepNode >;
class   Stmt; template class Plist< Stmt >;
class   TextAttribute; template class Plist< TextAttribute >;
class   Type; template class Plist< Type >;
class   UserDecl; template class Plist< UserDecl >;
class   VarDecl; template class Plist< VarDecl >;
class   WriteConstraint; template class Plist< WriteConstraint >;
 template class Plist< char const >;


class   Numbered; template class Pset< Numbered >;
class   C_Decl; template class Pset< C_Decl >;
class   C_UserDef; template class Pset< C_UserDef >;
class   C_EnumDef; template class Pset< C_EnumDef >;
class   PAconstraint; template class Pset< PAconstraint >;


class   UserDecl; template class ListStack< UserDecl >;
class   MemberDecl; template class ListStack< MemberDecl >;
class   VarDecl; template class ListStack< VarDecl >;


class   ObjectDecl; template class Scope< ObjectDecl >;
class   MemberDecl; template class Scope< MemberDecl >;
class   UserDecl; template class Scope< UserDecl >;


class   Indirection; template class SymbolTable< Indirection >;
class   ObjectDecl; template class SymbolTable< ObjectDecl >;
class   UserAnno; template class SymbolTable< UserAnno >;
 template class SymbolTable< const char /* or whatever, used for mere precence tables */ >;

class ALocSet;
class InvALocSet;
class SetConstraint;
class MultiPointsTo;

 template class Narray< ALocSet * /* used for several analysis results */ >;
 template class Narray< PA_Set * /* used in the PA */ >;
 template class Narray< void_typearr::node * /* used for pools in the PA */ >;
 template class Narray< MultiPointsTo * >;
 template class Narray< SetConstraint */* used for dataflow analysis result */ >;
 template class Narray< InvALocSet * /* used for dataflow analysis */ >;
 template class Narray< BTvariable * /* used for BTA results */ >;
 template class Narray< const char * /* name tables */ >;
 template class Narray< Pset<PAconstraint>* /* used in pa.cc */ >;
 template class Narray< C_Type * /* used in bta.cc */ >;
 template class Narray< Plist<C_Type>* /* used in separate.cc */ >;
 template class Narray< SepNode * /* used in separate.cc */ >;
 template class Narray< unsigned /* used by gegen */ >;
 template class Narray< Anchor * /* used by outcore */ >;
 template class Narray< Output * /* used by outbta.cc */ >;
 template class Narray< Plist<Output>* /* used in output.cc */ >;
 template class Narray< PredConstraint * /* used in transpred.cc */ >;
 template class Narray< NameMgr::NameBase * /* used in renamer.cc */ >;


class   PA_Set; template class typearr< PA_Set >;
class   C_Decl; template class typearr< C_Decl >;
