/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: Misc. helper routines
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "annofront.h"

char* stringDup(const char* str) {     /* Duplicate a string (dynamically). */
    size_t length ;
    char *result ;
    if ( str == NULL )
        str = "" ;
    length = strlen(str) + 1 ;
    result = safe_malloc(length) ;
    strcpy(result,str) ;
    return result ;
}
   
void* safe_malloc(size_t size) {
    void *allocated = malloc(size) ;
    if (allocated == NULL)
        die("Out of memory");
    return allocated ;
}

void die(const char* format,...) {
    va_list vl ;
    va_start(vl,format) ;
    vfprintf(stderr,format,vl);
    fprintf(stderr,"\n");
    exit(42) ;
}
