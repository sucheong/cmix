/* (-*-c++-*-)
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: "Switchboard" intermediate containers for C programs.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __SYNTAX__
#define __SYNTAX__

#include "Plist.h"
#include "cpgm.h"

// Switchboard for base type declarations.
typedef enum { P_Type_Unspecified, P_Int, P_Char, P_Float, P_Double,
               P_Void } Parse_BaseType;
typedef enum { P_Size_Unspecified, P_Short, P_Long } Parse_Size;
typedef enum { P_Sign_Unspecified, P_Signed, P_Unsigned } Parse_Sign;
typedef enum { P_NoStorageClass, P_Typedef, P_Extern, P_Static, P_Auto,
               P_Register } Parse_Storage;

class Parse_Type ;

class Parse_GeneralType
{
protected:
  virtual Type* gettype() = 0;
  friend class Parse_Declarator ;
public:
  Parse_Storage storage;
  constvol qualifiers ;
  Parse_GeneralType() : storage(P_NoStorageClass) {}
  virtual void combine(Parse_Type const&);
};

class Parse_Type : public Parse_GeneralType
{
    virtual Type* gettype();
  public:
    Parse_BaseType type;
    Parse_Size size;
    Parse_Sign sign;

    Parse_Type(void)
      : type(P_Type_Unspecified), size(P_Size_Unspecified),
        sign(P_Sign_Unspecified) {}

    Parse_Type(Parse_BaseType t)
      : type(t), size(P_Size_Unspecified), sign(P_Sign_Unspecified) {}

    Parse_Type(Parse_Size s)
      : type(P_Type_Unspecified), size(s), sign(P_Sign_Unspecified) {}

    Parse_Type(Parse_Sign sgn)
      : type(P_Type_Unspecified), size(P_Size_Unspecified), sign(sgn) {}

    Parse_Type(Parse_Storage s)
      : type(P_Type_Unspecified), size(P_Size_Unspecified),
        sign(P_Sign_Unspecified) { storage = s; }

    virtual void combine(Parse_Type const&);

    BaseTypeTag determine_type(void);

    char* show(void);
};

class Parse_UserType : public Parse_GeneralType
{
    virtual Type* gettype();
  public:
    char const* id;
    UserTag type;

    Parse_UserType(char const* name, UserTag t)
      : id(name), type(t) {}

    char* show(void);
};


class Parse_TypedefType : public Parse_GeneralType
{
    virtual Type* gettype();
    Parse_TypedefType(VarDecl*,int,int=0) {}
public:
    VarDecl* type;
    Parse_TypedefType(VarDecl* t)
        : type(t) {}
};

// Temporary container for postfixes (in declartors and casts)
// ie. char* const* (* a )() [];
//         ^^^^^^^^^^^   ^^^^^^

class Parse_Funtype ;

class Parse_Typemod {
public:
  virtual Type* apply(Type *) =0;
  virtual Parse_Funtype *me_as_function() { return 0; }
  virtual ~Parse_Typemod() {}
};

class Parse_Array : public Parse_Typemod {
  Expr* size;
  virtual Type* apply(Type*);
public:
  Parse_Array(Expr* e) : size(e) {}
};

class Parse_Pointer : public Parse_Typemod {
  virtual Type* apply(Type*);
  constvol qualifiers ;
public:
  Parse_Pointer(constvol cv) : qualifiers(cv) {}
};

class Parse_Funtype : public Parse_Typemod {
  virtual Type* apply(Type*);
  virtual Parse_Funtype* me_as_function() { return this; }
public:
  Position pos ;
  Plist<VarDecl>* params;
  bool varargs ;
  Plist<UserDecl>* usrdecls;

  Parse_Funtype(Plist<VarDecl>* p, bool v, Plist<UserDecl>* u, Position P)
    : pos(P), params(p), varargs(v), usrdecls(u) {}
  virtual ~Parse_Funtype();
};

class Parse_Declarator
{
public:
  char const* id;
  Position pos;
  Plist<Parse_Typemod> typemods;
  Parse_Declarator(char const* n, Position p, Plist<Parse_Typemod>* tms)
    : id(n), pos(p) { prepend_typemods(tms); }
  Parse_Declarator(char const* n, Position p)
    : id(n), pos(p) {}

  Parse_Declarator* prepend_typemods(Plist<Parse_Typemod> *tms)
    { typemods.splice(typemods.begin(),*tms);
      delete tms ;
      return this ;
    }
  ~Parse_Declarator();
  Type* maketype(Parse_GeneralType *);
};

class Parse_MemberId
{
public:
    Parse_Declarator* p_id;
    Expr* bits;  
    Parse_MemberId(Parse_Declarator* pi, Expr* e) : p_id(pi), bits(e) {}
};

#endif
