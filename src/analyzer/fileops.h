/* (-*-c++-*-)
 * Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __FILESOP__
#define __FILESOP__

#include <fstream.h>

bool CheckFile (const char*);

//--------------------------------------------------------------
// class CmixOutput
//
// A special type of ofstream that should be used for all disk file output.
// The constructor gets an extension such as "-gen.c" or ".anno" and
// selects a basename based on the names of the input files or any
// explicit command line switches.
//

class CmixOutput : public ofstream {
    static char *BestBaseSoFar ;
    static int QualitySoFar ;
    char *myname ;
    char const *MakeName(char const*) ;
public:
    CmixOutput(char const*extension) : ofstream(MakeName(extension)) {}
    char const *GetFilename(void) { return myname; } 
    static char const *ProposeBase (char const *,int,char const *const []);
    ~CmixOutput() {
        delete[] myname;
    }
} ;

// Use of CmixOutput::ProposeBase:
//  parameter #1 is a string that the proposed basename is a prefix of.
//  parameter #2 is the 'quality' of the name. Names with more quality
//               take precedence over those with less. For names with
//               equal quality, the one proposed first is used. The
//               quality values used at the time of this writing are
//                     0 - the default basename "cmix"
//                   100 - inferred from a blah.c at the command line
//                   200 - inferred from a blah.cmx at the command line
//                   900 - given in a outputbase: directive
//                  1000 - given as -o blah at the command line
//  parameter #3 is a null-terminated array of pointers to strings.
//               If one of these is found at the end of parameter #1, then
//               the text BEFORE that sting is the basename proposal.
//               If parameter #1 is to be considered verbatim, include
//               the empty string among those searched for.
// The return value is the string from parameter #3 that matched, or
// NULL if none did.

#endif
