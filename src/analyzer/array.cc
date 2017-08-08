/* Authors:  Jens Peter Secher (jpsecher@diku.dk)
 *           Arne Glenstrup (panic@diku.dk)
 *           Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: Implementation of array.h
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "auxilary.h"
#include "array.h"

array_extender::array_extender() {
    low_limit = 0 ;
    high_limit = 0 ;
}

array_extender::~array_extender() {
}

unsigned long
array_extender::trans(unsigned long u) {
    if ( u >= high_limit ) {
        if ( high_limit == 0 ) {
            if ( u > 4 )
                low_limit = u-4 ;
            else
                low_limit = 0 ;
            high_limit = low_limit+8 ;
            alloc_new(8) ;
            for ( unsigned j = 8 ; j-- ; )
                init_data(j);
            ok_new() ;
        } else {
            unsigned long new_high = u + 1 + (u-low_limit) / 3 ;
            assert( new_high > u );
            alloc_new(new_high-low_limit);
            unsigned long k = high_limit - low_limit ;
            for ( unsigned long j = new_high - high_limit ; j-- ; )
                init_data(j+k);
            while(k--)
                move_data(k,k);
            high_limit = new_high ;
            ok_new() ;
        }
    } else if ( u < low_limit ) {
        unsigned long extra = (high_limit-u) / 3 ;
        if ( extra > u )
            extra = u ;
        extra += low_limit - u ;
        unsigned long new_low = low_limit-extra ;
        alloc_new(high_limit-new_low) ;
        unsigned k = high_limit - low_limit ;
        while(k--)
            move_data(k,k+extra);
        while ( extra-- )
            init_data(extra);
        low_limit = new_low ;
        ok_new() ;
    }
    return u - low_limit ;
}

unsigned long
array_extender::ctrans(unsigned long u) const
{
    if ( u < low_limit || u >= high_limit )
        return -1UL ;
    return u-low_limit ;
}

void
array_extender::kill_all() {
    assert ( high_limit >= low_limit );
    high_limit -= low_limit ;
    while ( high_limit-- )
        kill_data(high_limit);
    low_limit = high_limit = 0 ;
}
