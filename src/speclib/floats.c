/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix speclib: functions related to floats 
 *
 * Copyright © 1999-2000. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <math.h>
#include <float.h>
#include <stdio.h>
#define cmixSPECLIB_SOURCE
#include "speclib.h"
#include "code.h"

static void
writeFloat(FILE *fp,int length,double ld)
{
    static int digtable[3] = { FLT_DIG+2, DBL_DIG+2, DBL_DIG+2 } ;
    static char const *typetable[3] = { "f","","L" };
    length -= LiftFloat ;
#ifdef HAVE_ISNAN
    if ( isnan(ld) )
        fprintf(fp,"(0.0%s/0.0%s)",typetable[length],typetable[length]);
    else
#endif
    if ( !(-1.0 <= ld && ld <= 1.0) && DBL_MAX / ld == 0.0 ) /* infinity */
        fprintf(fp,"(%s1.0%s/0.0%s)",ld>0?"":"-",
                typetable[length],typetable[length]);
    else
        fprintf(fp,"%#.*G%s",digtable[length],ld,typetable[length]);
}

Code
cmixLiftFloat(int length,double data)
{
    Code e = (union cmixExpr*) cmixMalloc(sizeof(struct cmixExprLiftFloat)) ;
    length += LiftFloat ;
    if ( length!=LiftDouble && length!=LiftFloat && length!=LiftLongDouble )
        cmixFatal("Misuse of cmixLiftFloat\n");
    e->lift_float.tag = (enum cmixExprTag)length ;
    e->lift_float.data = data ;
    e->lift_float.write = writeFloat ;
    return e ;
}

double cmix_atof(char *str) {
    char *error ;
    double out = strtod(str,&error);
    if ( *error ) {
        fprintf(stderr, "Malformed floating-point argument `%s'.\n",str);
        exit(1);
    }
    return out ;
}
 
