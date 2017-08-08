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
#include <sys/stat.h>
#include "fileops.h"
#include "diagnostic.h"
#include "options.h"
#include "auxilary.h"

bool
CheckFile (const char* name)
{
    struct stat result;
    return (bool) !stat(name, &result);
}

char *CmixOutput::BestBaseSoFar = stringDup("cmix");

int CmixOutput::QualitySoFar = 0 ;

char const *CmixOutput::MakeName(char const *extension) {
    myname = new char[strlen(BestBaseSoFar)+strlen(extension)+1];
    strcpy(myname,BestBaseSoFar);
    strcat(myname,extension);
    return myname ;
}

const char* CmixOutput::ProposeBase ( char const *base, int quality,
                               char const *const suffixes[]) {
    size_t baslen = strlen(base);
    while ( suffixes[0] ) {
        size_t suflen = strlen(suffixes[0]) ;
        if ( suflen == 0 || suflen < baslen &&
             strcmp(base + (baslen-suflen), suffixes[0]) == 0 ) {
            if ( quality > QualitySoFar ) {
                delete[] BestBaseSoFar ;
                BestBaseSoFar = new char[baslen-suflen+1] ;
                strncpy(BestBaseSoFar,base,baslen-suflen) ;
                BestBaseSoFar[baslen-suflen] = '\0' ;
                QualitySoFar = quality ;
                if ( suflen )
                    return BestBaseSoFar ;
            }
            return NULL;
        }
        suffixes++ ;
    }
    return NULL ;
}

//******************************************************************//

void handleBasenameOption(const char *arg) {
    static bool FirstTime = true ;
    if (!FirstTime) {
        Diagnostic(ERROR,Position()) << "only one -o option is allowed" ;
        return ;
    }
    FirstTime = false ;
    
    static char const* const ignoreendings[]
        =  { "-gen.c", "-gen.cc", ".ann", "", NULL } ;

    char const* stripped = CmixOutput::ProposeBase(arg,1000,ignoreendings);
    if ( stripped && stripped[0] )
        Diagnostic(WARNING,Position()) << "assuming you meant just -o "
                                       << stripped ;
}
