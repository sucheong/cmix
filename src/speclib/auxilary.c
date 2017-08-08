/*	File:	  auxilary.c
 *      Author:	  Lars Ole Andersen (lars@diku.dk)
 *                Peter Holst Andersen (txix@diku.dk)
 *                Henning Makholm (makholm@diku.dk)
 *      Content:  C-Mix speclib: miscellaneous helper functions
 * 
 *      Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 *      Redistribution and modification are allowed under certain
 *      terms; see the file COPYING.cmix for details.
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strings.h>
#define cmixSPECLIB_SOURCE
#include "speclib.h"

int cmixSpeclibAbortSignal = SIGKILL ;

void cmixFatal(char *err)
{
    fflush(NULL);
    fprintf(stderr, "C-Mix fatal error: %s\n", err);
    cmixPrintMemusage();
    raise(cmixSpeclibAbortSignal);
    raise(SIGKILL);
}

void *
cmixMalloc(size_t siz) {
  void *ret = malloc(siz);
  if ( ret == NULL )
    cmixFatal("out of memory");
  return ret ;
}

char cmix_atoc(char *str)
{
    if (strlen(str) > 1)
	fprintf(stderr, "Warning: Only one character expected.\n");
    else if (strlen(str) < 1) {
	fprintf(stderr, "Warning: Zero length string.\n");
	return 0;
    }
    return str[0];
}

long cmix_atoi(char *str) {
    char *error ;
    long out = strtol(str,&error,0);
    if ( *error ) {
        fprintf(stderr, "Malformed integer argument `%s'.\n",str);
        exit(1);
    }
    return out ;
}
