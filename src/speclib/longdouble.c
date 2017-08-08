/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix speclib: functions related to long doubles
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
writeLongDouble(FILE *fp,long double ld)
{
#ifdef HAVE_ISNAN
  if ( isnan(ld) )
    fprintf(fp,"(0.0L/0.0L)");
  else
#endif
    if ( !(-1.0 <= ld && ld <= 1.0) && LDBL_MAX / ld == 0.0 ) /* infinity */
      fprintf(fp,"(%s1.0L/0.0L)",ld>0?"":"-");
    else
      fprintf(fp,"%#.*LGL",LDBL_DIG+2,ld);
}

Code
cmixLiftLongDouble(long double data)
{
    Code e = (union cmixExpr*) cmixMalloc(sizeof(struct cmixExprLiftLD)) ;
    e->lift_longdouble.tag = LiftTrueLongDouble ;
    e->lift_longdouble.data = data ;
    e->lift_longdouble.write = writeLongDouble ;
    return e ;
}
