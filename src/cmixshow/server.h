/* Author:   Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: Generic server interface
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

		
#include <stdio.h>
unsigned server_init(const char *progname);
void server_stop(int notused);
void server_loop(const char *progname,void (*service)(FILE*,FILE*));

		
