/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Literal strings used by all output functions.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __STRINGS__
#define __STRINGS__

// TYPES
extern const char* str_void;
extern const char* str_char;
extern const char* str_schar;
extern const char* str_uchar;
extern const char* str_short;
extern const char* str_ushort;
extern const char* str_int;
extern const char* str_uint;
extern const char* str_long;
extern const char* str_ulong;
extern const char* str_float;
extern const char* str_double;
extern const char* str_ldouble;

// STRUCTURES
extern const char* str_struct;
extern const char* str_enum;
extern const char* str_union;

// OPERATORS
extern const char* str_addr;
extern const char* str_plus;
extern const char* str_minus;
extern const char* str_neg;
extern const char* str_bang;
extern const char* str_mul;
extern const char* str_div;
extern const char* str_mod;
extern const char* str_lshift;
extern const char* str_rshift;
extern const char* str_less;
extern const char* str_greater;
extern const char* str_lesseq;
extern const char* str_greatereq;
extern const char* str_equal;
extern const char* str_notequal;
extern const char* str_bxor;
extern const char* str_bor;
extern const char* str_and;
extern const char* str_or;

// DELIMITERS
extern const char* str_blank;
extern const char* str_is;
extern const char* str_dot;
extern const char* str_lparen;
extern const char* str_rparen;
extern const char* str_comma;
extern const char* str_semi;
extern const char* str_lbrace;
extern const char* str_rbrace;
extern const char* str_lbracket;
extern const char* str_rbracket;
extern const char* str_colon;
extern const char* str_rarrow;
extern const char* str_question;
extern const char* str_tilde;

// MODIFIERS
extern const char* str_const;
extern const char* str_vol;
extern const char* str_static;
extern const char* str_extern;

// KEYWORDS
extern const char* str_if;
extern const char* str_else;
extern const char* str_case;
extern const char* str_switch;
extern const char* str_for;
extern const char* str_do;
extern const char* str_while;
extern const char* str_goto;
extern const char* str_return;
extern const char* str_malloc;
extern const char* str_calloc;
extern const char* str_length;
extern const char* str_sizeof;
extern const char* str_free;

extern const char* str_default;
extern const char* str_break;
extern const char* str_continue;
extern const char* str_typedef;
extern const char* str_anything_but;

#endif
