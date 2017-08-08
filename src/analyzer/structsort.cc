/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: 
 * History:  Derived from theory by Peter Holst Andersen and Lars Ole Andersen.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdlib.h>
#include "analyses.h"

static int mycompare(const void *a,const void *b) {
    C_UserDef* &A = *(C_UserDef **)a ;
    C_UserDef* &B = *(C_UserDef **)b ;
    if ( A->defseqnum < B->defseqnum )
        return -1 ;
    if ( A->defseqnum > B->defseqnum )
        return 1 ;
    return 0 ;
}

void sortStructDecls(C_Pgm &corepgm) {
    unsigned size = corepgm.usertypes.size() ;
    if ( size < 2 )
        return ;
    C_UserDef **array = new C_UserDef*[size] ;
    unsigned num=0 ;
    foreach(i,corepgm.usertypes,Plist<C_UserDef>)
        array[num++] = *i ;
    corepgm.usertypes.clear() ;
    qsort(array,size,sizeof(C_UserDef*),mycompare);
    for ( num = 0 ; num < size ; num++ )
        corepgm.usertypes.push_back(array[num]);
    delete[] array ;
}
    
