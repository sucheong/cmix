/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <string.h>
#include "syntax.h"
#include "auxilary.h"
#include "diagnostic.h"

extern LexerTracker ccchere ;

///// SHOW FUNCTIONS /////

char *exshow(Parse_Storage sc)
{
    switch (sc) {
      case P_NoStorageClass:	return "";
      case P_Typedef:	return "typedef";
      case P_Extern:	return "extern";
      case P_Static:	return "static";
      case P_Auto:	return "auto";
      case P_Register:	return "register";
      default:		Diagnostic(INTERNAL,ccchere)
                            << "exshow: unknown storage class";
                        return NULL;
    }
}

char *exshow(Parse_BaseType bt)
{
    switch (bt) {
      case P_Type_Unspecified:	return "";
      case P_Int:		return "int";
      case P_Char:		return "char";
      case P_Float:		return "float";
      case P_Double:		return "double";
      case P_Void:		return "void";
      default:			Diagnostic(INTERNAL,ccchere)
                                   << "exshow: unknown parse base-type";
                                return NULL;
    }
}

char *exshow(Parse_Size size)
{
    switch (size) {
      case P_Size_Unspecified:	return "";
      case P_Short:		return "short";
      case P_Long:		return "long";
      default:			Diagnostic(INTERNAL,ccchere)
                                   << "exshow: unknown size";
                                return NULL;
    }
}

char *exshow(Parse_Sign sign)
{
    switch (sign) {
      case P_Sign_Unspecified:	return "";
      case P_Signed:		return "signed";
      case P_Unsigned:		return "unsigned";
      default:			Diagnostic(INTERNAL,ccchere)
                                  << "exshow: unknown sign";
                                return NULL;
    }
}

char*
Parse_Type :: show(void)
{
    static char buffer[256];
    char* tmp;
    buffer[0] = '\0';

    tmp = exshow(storage);
    if (tmp[0] != '\0') {
        strcat(buffer,tmp);
        strcat(buffer," ");
    }
    if (qualifiers.cst)
      strcat(buffer,"const ");
    if (qualifiers.vol)
      strcat(buffer,"volatile ");
    tmp = exshow(sign);
    if (tmp[0] != '\0') {
        strcat(buffer,tmp);
        strcat(buffer," ");
    }
    tmp = exshow(size);
    if (tmp[0] != '\0') {
        strcat(buffer,tmp);
        strcat(buffer," ");
    }
    tmp = exshow(type);
    if (tmp[0] != '\0') {
        strcat(buffer,tmp);
        strcat(buffer," ");
    }

    return strdup(buffer);
};

////// TYPE CONBINATION AND DERTERMINATION ///////


void
Parse_GeneralType :: combine(Parse_Type const &other)
{
    // The Parse_Type can only be a modifier/qualifier:
    assert(other.type == P_Type_Unspecified);
    assert(other.size == P_Size_Unspecified);
    assert(other.sign == P_Sign_Unspecified);

    if (qualifiers.cst && other.qualifiers.cst)
        Diagnostic(WARNING,ccchere) << "duplicate `const'";
    if (qualifiers.vol && other.qualifiers.vol)
        Diagnostic(WARNING,ccchere) << "duplicate `volatile'";
    qualifiers += other.qualifiers ;

    // Resolve storage class.
    if (other.storage == P_NoStorageClass)
        ;
    else if (storage == P_NoStorageClass)
        storage = other.storage;
    else if (storage == other.storage)
        Diagnostic(WARNING,ccchere) << "duplicate " << exshow(storage) ;
    else
        Diagnostic(ERROR,ccchere) << "multiple storage classes";
};

Type*
Parse_Pointer::apply(Type* t)
{
  t = new PtrType(t);
  t->cv = qualifiers ;
  return t;
}

Type*
Parse_Array::apply(Type *t)
{
  return new ArrayType(size,t);
}

Type*
Parse_Funtype::apply(Type* t)
{
  Plist<Type> *ptypes = new Plist<Type> ;
  foreach(i,*params,Plist<VarDecl>)
    ptypes->push_back(i->type);
  return new FunType(ptypes,varargs,t,pos);
}

Parse_Funtype::~Parse_Funtype() {
  if( params ) {
    foreach(i,*params,Plist<VarDecl>)
      delete *i ;
    delete params ;
  }
  if( usrdecls )
    delete usrdecls ;
} 

Type*
Parse_Declarator::maketype(Parse_GeneralType* gt)
{
  Type* t = gt->gettype() ;
  foreach(it,typemods,Plist<Parse_Typemod>)
    t = it->apply(t);
  return t ;
}

Parse_Declarator::~Parse_Declarator()
{
  foreach(i,typemods,Plist<Parse_Typemod>)
    delete *i ;
}

// Combine this Parse_Type with another Parse_Type. If it succeeds,
// "this" reflects the combination.
void
Parse_Type :: combine(Parse_Type const& other)
{
    if (qualifiers.cst && other.qualifiers.cst)
        Diagnostic(WARNING,ccchere) << "duplicate `const'";
    if (qualifiers.vol && other.qualifiers.vol)
        Diagnostic(WARNING,ccchere) << "duplicate `volatile'";
    qualifiers += other.qualifiers;

    // Combine types.
    // If one of them has an unspecified type, the result is the type
    // that is specified.
    if (type == P_Type_Unspecified) 
        type = other.type;
    else if (other.type != P_Type_Unspecified) {
        if (type == other.type)
            Diagnostic(WARNING,ccchere) << "duplicate " << exshow(type) ;
        else
            Diagnostic(ERROR,ccchere) <<
                "two different base types in declaration" ;
    }

    // Combine sizes.
    if (size == P_Size_Unspecified)
        size = other.size;
    else if (other.size != P_Size_Unspecified) {
        if (size == P_Short && other.size == P_Short) 
            Diagnostic(WARNING,ccchere) << "duplicate `short'";
	if (size == P_Long && other.size == P_Long) {
	    Diagnostic di(WARNING,ccchere) ;
            di << "duplicate `long', assuming type is `long'";
            di.addline() << "(we don't handle long long yet)";
        }
	else
            Diagnostic(ERROR,ccchere) << "long and short specified together";
    }
    
    // Combine signs (signed, unsigned)
    if (sign == P_Sign_Unspecified)
        sign = other.sign;
    else if (other.sign != P_Sign_Unspecified) {
        if (sign == other.sign)
            Diagnostic(WARNING,ccchere) << "duplicate " << exshow(sign) ;
        else
            Diagnostic(ERROR,ccchere) << "signed and unsigned given together";
    }

    // Resolve storage class.
    if (other.storage == P_NoStorageClass)
        ;
    else if (storage == P_NoStorageClass)
        storage = other.storage;
    else if (storage == other.storage)
        Diagnostic(WARNING,ccchere) << "duplicate " << exshow(storage);
    else
        Diagnostic(ERROR,ccchere) << "multiple storage classes";
}


// Determine real type from a Parse_Type.
BaseTypeTag
Parse_Type :: determine_type()
{
    switch (type) {
      case P_Type_Unspecified: // Assume int.
      case P_Int:
	switch (size) {
	  case P_Size_Unspecified:
	    switch (sign) {
	      case P_Sign_Unspecified: // Assume signed.
	      case P_Signed:
		return Int;
	      case P_Unsigned:
		return UInt;
	      default:
		Diagnostic(INTERNAL,ccchere) << "determine_type: unknown sign";
	    }
	  case P_Short:
	    switch (sign) {
	      case P_Sign_Unspecified:
	      case P_Signed:
		return ShortInt;
	      case P_Unsigned:
		return UShortInt;
	      default:
		Diagnostic(INTERNAL,ccchere) << "determine_type: unknown sign";
	    }
	  case P_Long:
	    switch (sign) {
	      case P_Sign_Unspecified:
	      case P_Signed:
		return LongInt;
	      case P_Unsigned:
		return ULongInt;
	      default:
		Diagnostic(INTERNAL,ccchere) << "determine_type: unknown sign";
	    }
	  default:
	    Diagnostic(INTERNAL,ccchere) << "determine_type: unknown size";
	}
      case P_Char:
	if (size != P_Size_Unspecified)
	    Diagnostic(ERROR,ccchere)
                << "long or short specified for char type";
	if (sign == P_Unsigned)
	    return UChar;
	else if (sign == P_Signed)
            return SChar;
        else
	    return Char;
      case P_Float:
	if (size != P_Size_Unspecified)
	    Diagnostic(ERROR,ccchere)
                << "long or short specified for float type";
	if (sign != P_Sign_Unspecified)
	    Diagnostic(ERROR,ccchere)
                << "sign specified for float type";
	return Float;
      case P_Double:
	if (sign != P_Sign_Unspecified)
	    Diagnostic(ERROR,ccchere) << "sign specified for double type";
	switch (size) {
	  case P_Size_Unspecified:
	    return Double;
	  case P_Long:
	    return LongDouble;
	  case P_Short:
	    Diagnostic(ERROR,ccchere) << "short specified for double type";
	  default:
	    Diagnostic(INTERNAL,ccchere) << "determine_type: unknown size";
	}
      case P_Void:
	if (size != P_Size_Unspecified)
	    Diagnostic(ERROR,ccchere)
                << "long or short specified for void type";
	if (sign != P_Sign_Unspecified)
	    Diagnostic(ERROR,ccchere) << "sign specified for void type";
	return Void;
      default:
	Diagnostic(INTERNAL,ccchere) << "determine_type: unknown type tag";
	return Void;
    }
}
    
