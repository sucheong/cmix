#line 1 "./options.org"
/* -*- C++ -*-
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: options processing code
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_REGEX_H
#include <stddef.h> // for redhat machines which otherwise choke on regex.h
#include <regex.h>  // for OSF alphas with buggy GCC installations
#endif
#line 18

#include <ctype.h>
#include "options.h"
#include "directives.h"
#include "auxilary.h"
#ifdef USE_GNU_GETOPT
#include "getopt.h"
#else
#line 25

#include <unistd.h>
#define getopt_long(a,b,c,d,e) getopt(a,b,c)
#endif
#line 28


// only to be set by directives:
bool lift_long_double = false ;
bool lift_uchar_as_char = false ;













bool hardcore_mode = true ;
bool core_mode = false;








bool memo_unconditional_mode = false ;




// bool use_assigned_gotos = false ;
// %OPTION -g --gcc "use GCC extensions to speed up *-gen.c" {






bool only_preprocess_mode = false ;
unsigned quiet_mode = 1 ;




























void ParseDebugOpt(const char*);








static char short_list[]=":VBsSbgqvEo:e:D:I:d:h";

#ifdef USE_GNU_GETOPT
static struct option long_list[] = {
{(char*)"version",no_argument,NULL,'V'},
{(char*)"build-version",no_argument,NULL,'B'},
{(char*)"corec",no_argument,NULL,'s'},
{(char*)"hardcore",no_argument,NULL,'S'},
{(char*)"memoize-gotos",no_argument,NULL,'b'},
{(char*)"quiet",no_argument,NULL,'q'},
{(char*)"silent",no_argument,NULL,'q'},
{(char*)"verbose",no_argument,NULL,'v'},
{(char*)"preprocess",no_argument,NULL,'E'},
{(char*)"basename",required_argument,NULL,'o'},
{(char*)"output",required_argument,NULL,'o'},
{(char*)"directive",required_argument,NULL,'e'},
{(char*)"expression",required_argument,NULL,'e'},
{(char*)"define",required_argument,NULL,'D'},
{(char*)"include-dir",required_argument,NULL,'I'},
{(char*)"debug",optional_argument,NULL,'d'},
{(char*)"help",no_argument,NULL,'h'},
#line 112

};
#endif
#line 114


const char *argv0 ;
void parse_options(int argc, char * const argv[]) {
    argv0 = argv[0];
    bool error = false ;
    if ( argc <= 1 ) {
	banner(cout);
	// The copyright for Roskind's grammar requires us to credit him.
	cout << "Portions Copyright (c) 1989, 1990 James A. Roskind" << endl ;
        goto display_help ;
    }
    int option ;
    while ( (option=getopt_long(argc,argv,short_list,long_list,NULL)) != -1 )
      switch(option) {
#line 34
case 'V': {
  cout << "C-Mix " << CmixRelease << endl ;
  exit(0) ;
}break;
#line 39
case 'B': {
  cout << "C-Mix version " << CmixRelease << endl ;
  cout << "cpp command: `" << CPP << "'" << endl ;
  cout << "shadow headers in `" << CMIX_SHADOW_DIR << "'" << endl ;
  exit(0) ;
}break;
#line 48
case 's': {
  core_mode = true;
  hardcore_mode = false;
}break;
#line 52
case 'S': {
  core_mode = true;
}break;
#line 57
case 'b': {
  memo_unconditional_mode = true ;
}break;
#line 63
case 'g': {
  Diagnostic(WARNING,Position())
    << "-g option has no effect in this version of C-Mix" ;
  // use_assigned_gotos = true ;
}break;
#line 71
case 'q': {
  quiet_mode = 2;
}break;
#line 74
case 'v': {
  if ( !only_preprocess_mode )
    quiet_mode = 0;
}break;
#line 78
case 'E': {
  quiet_mode = 2;
  only_preprocess_mode = true ;
}break;
#line 83
case 'o': {
  handleBasenameOption(optarg);
}break;
#line 87
case 'e': {
  JamInDirective(optarg,Position("directive on command line"));
}break;
#line 91
case 'D': {
  directives.add_cpparg('D',optarg);
}break;
#line 95
case 'I': {
  directives.add_cpparg('I',optarg);
}break;
#line 100
case 'd': {
  ParseDebugOpt(optarg);
}break;
#line 104
case 'h': {
  goto display_help ;
}break;
#line 129

      case 1:
          // used by some incarnations of GNU getopt to signal a non-option
          handleNonOption(optarg);
          break ;
      case ':':
          // some getopts do not recognise ::
          if ( optopt == 'd' ) {
              ParseDebugOpt(NULL);
              break ;
          }
      default:
      case '?':
          error = true ;
          goto display_help ;
      }
    while( optind < argc ) {
        handleNonOption(argv[optind++]);
    }
    return ;
 display_help:
    cout << "Usage: " << argv0 << " [options] ( file.cmx | file.c )*" << endl
         << "Options: " << endl <<
"  -V    - show version number and exit\n"
"  -B    - show compiled-in configutation\n"
"  -s    - show core representation of program\n"
"  -S    - show detailed core representation of program\n"
"  -b    - try hard to share code within each function (slow!)\n"
"  -g    - no-op for backwards compatibility\n"
"  -q    - don't display messages except errors and warnings\n"
"  -v    - display detailed progress messages\n"
"  -E    - only preprocess input files\n"
"  -oxxx - select base name for output files\n"
"  -exxx - give a specializer directive\n"
"  -Dxxx - define a preprocessor symbol\n"
"  -Ixxx - give search path for include files\n"
"  -dxxx - enable debugging output (use -d for help)\n"
"  -h    - display this help message\n"
#line 152

	 << "Report bugs to: cmix-bugreport@diku.dk" << endl ;
    if ( error )
        exit(1);
    else
        exit(0);
}
 
bool DumpCoreC = false;
bool DumpPA = false;
bool DumpBta = false;
bool DumpUST = false;
bool DumpEnd = false;
bool trace_cmx_parser = false;
bool trace_c_parser = false;
bool core_addresses = false;
bool less_inlining = false;

DebugStream::DebugStream() : level(0) {}
DebugStream debugstream_cpgm; 
DebugStream debugstream_corec; 
DebugStream debugstream_outcpgm; 
DebugStream debugstream_outcore; 
DebugStream debugstream_gram; 
DebugStream debugstream_init; 
DebugStream debugstream_check; 
DebugStream debugstream_c2core; 
DebugStream debugstream_pa; 
DebugStream debugstream_locals; 
DebugStream debugstream_cg; 
DebugStream debugstream_dataflow; 
DebugStream debugstream_bta; 
DebugStream debugstream_separate; 
DebugStream debugstream_split; 
DebugStream debugstream_gegen; 
DebugStream debugstream_ygtree; 
//%DEBUGSTREAM symtbl 2

void ParseDebugOpt(const char *opt) {
    static const struct {
        char const* tag ;
        bool *onoff ;
        DebugStream *stream ;
        unsigned maxlevel ;
    } debug_table[] = {
{"dump-corec",&DumpCoreC,NULL,0},
{"dump-pa",&DumpPA,NULL,0},
{"dump-bta",&DumpBta,NULL,0},
{"dump-redirect",&DumpUST,NULL,0},
{"dump-dataflow",&DumpEnd,NULL,0},
{"trace-cmx-parser",&trace_cmx_parser,NULL,0},
{"trace-c-parser",&trace_c_parser,NULL,0},
{"core-pointers",&core_addresses,NULL,0},
{"less-inlining",&less_inlining,NULL,0},
{"cpgm",NULL,&debugstream_cpgm,1},
{"corec",NULL,&debugstream_corec,5},
{"outcpgm",NULL,&debugstream_outcpgm,2},
{"outcore",NULL,&debugstream_outcore,2},
{"gram",NULL,&debugstream_gram,5},
{"init",NULL,&debugstream_init,4},
{"check",NULL,&debugstream_check,5},
{"c2core",NULL,&debugstream_c2core,7},
{"pa",NULL,&debugstream_pa,6},
{"locals",NULL,&debugstream_locals,2},
{"cg",NULL,&debugstream_cg,3},
{"dataflow",NULL,&debugstream_dataflow,3},
{"bta",NULL,&debugstream_bta,4},
{"separate",NULL,&debugstream_separate,1},
{"split",NULL,&debugstream_split,2},
{"gegen",NULL,&debugstream_gegen,1},
{"ygtree",NULL,&debugstream_ygtree,1},
#line 197

    };
    const int dtablen = sizeof debug_table / sizeof debug_table[0] ;
    if ( opt == NULL ) {
        banner(cout);
        cout << "List of debug options:" << endl ;
        for ( int i = 0 ; i < dtablen ; i++ ) {
            cout << "  -d" << debug_table[i].tag ;
            if ( debug_table[i].maxlevel > 1 )
                cout << "[=n]"
                     << "                    0 <= n <= "
                    + strlen(debug_table[i].tag)
                     << debug_table[i].maxlevel ;
            cout << endl ;
        }
        cout << "  -d+ turns all of the above on (DON'T do this!)" << endl ;
        exit(0) ;
    }
    if ( strcmp(opt,"+") == 0 ) {
        for ( int i = 0 ; i < dtablen ; i++ ) {
            if ( debug_table[i].onoff )
                *debug_table[i].onoff = true ;
            if ( debug_table[i].stream )
                debug_table[i].stream->level = debug_table[i].maxlevel ;
        }
        return ;
    }
    for ( int i = 0 ; i < dtablen ; i++ ) {
        int l = strlen(debug_table[i].tag) ;
        if ( strncmp(opt,debug_table[i].tag,l) != 0 )
            continue ;
        if ( debug_table[i].onoff && opt[l] == '\0' ) {
            *debug_table[i].onoff = true ;
            return ;
        }
        assert(debug_table[i].stream != 0);
        if ( opt[l] == '\0' ) {
            debug_table[i].stream->level = debug_table[i].maxlevel ;
            return ;
        }
        if ( opt[l] == '=' && opt[l+1] && isdigit(opt[l+1]) && !opt[l+2] ) {
            unsigned level = opt[l+1]-'0' ;
            if ( level > debug_table[i].maxlevel ) {
                Diagnostic(WARNING,Position())
                    << "there is no level " << opt[l+1] << " for -d"
                    << debug_table[i].tag ;
                level = debug_table[i].maxlevel ;
            }
            debug_table[i].stream->level = level ;
            return ;
        }
    }
    Diagnostic(ERROR,Position())
        << "there is no `" << opt << "' debug option" ;
}
       
        
    
    
