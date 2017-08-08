/* (-*-c++-*-)
 * Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: option flags
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __OPTIONS__
#define __OPTIONS__

#include "diagnostic.h"

extern char const *argv0 ;

extern unsigned quiet_mode ; // 0 = verbose, 1 = standard, 2 = quiet
extern bool core_mode ;
extern bool hardcore_mode ;
extern bool memo_unconditional_mode ;
extern bool use_assigned_gotos ;
extern bool only_preprocess_mode ;
extern bool lift_uchar_as_char ;
extern bool lift_long_double ;

void handleBasenameOption(const char *arg);

void parse_options(int, char *const*);

void banner(ostream&);
void handleNonOption(const char *arg);
    
class DebugStream : public CerrLikeStream {
    unsigned level ;
    friend void ParseDebugOpt(const char*);
public:
    DebugStream();
    operator bool() { return level != 0; }
    bool operator ^(int l) { return level >= (unsigned)l; }
    bool operator %(int l) { return level == (unsigned)l; }
};

extern bool DumpCoreC ;
extern bool DumpPA ;
extern bool DumpBta ;
extern bool DumpUST ;
extern bool DumpEnd ;
extern bool trace_cmx_parser ;
extern bool trace_c_parser ;
extern bool core_addresses ;
extern bool less_inlining ;

extern DebugStream debugstream_cpgm ;
extern DebugStream debugstream_corec ;
extern DebugStream debugstream_outcpgm ;
extern DebugStream debugstream_outcore ;
extern DebugStream debugstream_gram ;
extern DebugStream debugstream_init ;
extern DebugStream debugstream_check ;
extern DebugStream debugstream_c2core ;
extern DebugStream debugstream_pa ;
extern DebugStream debugstream_locals ;
extern DebugStream debugstream_cg ;
extern DebugStream debugstream_dataflow ;
extern DebugStream debugstream_bta ;
extern DebugStream debugstream_separate ;
extern DebugStream debugstream_split ;
extern DebugStream debugstream_gegen ;
extern DebugStream debugstream_ygtree ;
extern DebugStream debugstream_symtbl ;

#endif
