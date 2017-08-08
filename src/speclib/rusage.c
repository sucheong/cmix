/*	File:	  rusage.c
 *      Author:	  Peter Holst Andersen (txix@diku.dk)
 *      Content:  C-Mix Cogen library: Resource usage
 *
 *      Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 *      Redistribution and modification are allowed under certain
 *      terms; see the file COPYING.cmix for details.
 */
  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define cmixSPECLIB_SOURCE
#include "speclib.h"

extern void *_end, *end, *_etext, *etext, *_edata, *edata;

static void print_bytes(FILE *fp, unsigned long x);

static long max_sbrk;
static unsigned long stats[STAT_LAST];
static unsigned long bytes_saved, bytes_allocated;

void
cmixCheckMemusage(void)
{
    long foo;

    foo = (long)sbrk(0);
    if (foo > max_sbrk)
	max_sbrk = foo;
}

void
cmixPrintMemusage(void)
{
    unsigned long foo ;
    
    char *str[] = { "NULL:", "Check stack:", "Stack:", "Check heap:", "Heap:",
		    "Check globl:", "Globals:", "Not found:" };
    int index[] = { STAT_CMP_NULL,
		    STAT_CMP_CHECK_STACK, STAT_CMP_STACK,
		    STAT_CMP_CHECK_HEAP, STAT_CMP_HEAP,
		    STAT_CMP_CHECK_GLOBALS, STAT_CMP_GLOBALS,
		    STAT_CMP_NOT_FOUND,
		    -1
		  };
    int index2[] = { STAT_COPY_NULL,
		     STAT_COPY_CHECK_STACK, STAT_COPY_STACK,
		     STAT_COPY_CHECK_HEAP, STAT_COPY_HEAP,
		     STAT_COPY_CHECK_GLOBALS, STAT_COPY_GLOBALS,
		     STAT_COPY_NOT_FOUND,
		     -1
		   };
    unsigned i;
  
    fprintf(stderr, "Maximum memusage (approx): ");
#ifdef ETEXT_LINKED_IN
    print_bytes(stderr, max_sbrk - (long)&edata);
#else
    fprintf(stderr, "<cannot be detected on this system>\n");
#endif
    fprintf(stderr, "Number of compare state (matches): %lu (%lu)\n",
	    stats[0], stats[1]);
    fprintf(stderr, "Number of compare large (matches): %lu (%lu)\n",
	    stats[2], stats[4]);
    fprintf(stderr,
	    "Average length of lists in obj_table: %6.3f\n",
	    stats[2] > 0
	    ? (double) stats[3] / (double) stats[2]
	    : 0.0);
    fprintf(stderr, "Memory allocated for copied objects: ");
    print_bytes(stderr, bytes_allocated);
    fprintf(stderr, "Storage saved due to sharing:        ");
    print_bytes(stderr, bytes_saved);
    
    foo = stats[STAT_CMP_NULL] + stats[STAT_CMP_STACK] +
          stats[STAT_CMP_HEAP] + stats[STAT_CMP_NOT_FOUND];

    fprintf(stderr, "cmix_cmpReferencedObject\n");
    fprintf(stderr, "  total calls: %11lu\n", foo);

    for (i = 0; index[i] != -1; i++)
	fprintf(stderr, "  %-12s %11lu (%2.0f %%)\n",
		str[i], stats[index[i]], 
		100.0 * (double)stats[index[i]] / (double)foo);

    foo = stats[STAT_COPY_NULL] + stats[STAT_COPY_STACK] +
	  stats[STAT_COPY_HEAP] + stats[STAT_COPY_NOT_FOUND];

    fprintf(stderr, "cmix_copyReferencedObject\n");
    fprintf(stderr, "  total calls: %11lu\n", foo);

    for (i = 0; index2[i] != -1; i++)
	fprintf(stderr, "  %-12s %11lu (%2.0f %%)\n",
		str[i], stats[index2[i]], 
		100.0 * (double)stats[index2[i]] / (double)foo);
}

void
cmixCollectStatistic(int kind, int arg1)
{
    stats[kind]++;
    if (kind == STAT_SHARED_FOUND)
	bytes_saved += arg1;
    else if (kind == STAT_ALLOCATED)
	bytes_allocated += arg1;
}

static void
print_bytes(FILE *fp, unsigned long x)
{
    double y;

    y = x;
    fprintf(fp, "%ld", x);
    if (y > 1024.0*1024.0)
	fprintf(fp, " (%4.1f MB)", y / 1024.0 / 1024.0);
    else if (y > 0.0)
	fprintf(fp, " (%4.1f KB)", y / 1024.0);
    fprintf(fp, "\n");
}
