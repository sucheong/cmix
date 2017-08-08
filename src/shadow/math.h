/* $Id: math.h,v 1.2 1999/07/17 14:56:42 makholm Exp $
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix shadow header: math.h
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 */

#ifndef __CMIX
# error This file is only intended to be included from C-Mix/II
#endif

#ifndef __CMIX_MATH__
#define __CMIX_MATH__

#pragma cmix header: <math.h>

extern const double HUGE_VAL ;
#pragma cmix well-known constant: HUGE_VAL ;

/* ISO C 7.5.2 Trigonometric functions */
double acos(double);
double asin(double);
double atan(double);
double atan2(double,double);
double cos(double);
double sin(double);
double tan(double);
#pragma cmix well-known: acos asin atan atan2 cos sin tan
#pragma cmix pure: acos() asin() atan() atan2() cos() sin() tan()

/* ISO C 7.5.3 Hyperbolic functions */
double cosh(double);
double sinh(double);
double tanh(double);
#pragma cmix well-known: cosh sinh tanh
#pragma cmix pure: cosh() sinh() tanh()

/* ISO C 7.5.4 Exponential and logarithmic functions */
double exp(double);
double frexp(double,int*);
double ldexp(double,int);
double log(double);
double log10(double);
double modf(double,double*);
#pragma cmix well-known: exp frexp ldexp log log10 modf
#pragma cmix stateless: exp() frexp() ldexp() log() log10() modf()
/* stateless is ok, because for functions that do not manipulate
 * pointers it is actually the same as pure
 */

/* ISO C 7.5.5 Power functions */
double pow(double,double);
double sqrt(double);
#pragma cmix well-known: pow sqrt
#pragma cmix pure: pow() sqrt()

/* ISO C 7.5.6 Nearest integer, absolute value, and remainder functions */
double ceil(double);
double fabs(double);
double floor(double);
double fmod(double,double);
#pragma cmix well-known: ceil fabs floor fmod
#pragma cmix pure: ceil() fabs() floor() fmod()

#endif
