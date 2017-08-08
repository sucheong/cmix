/* Author:   Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: old main program, now -s option
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#define _INCLUDE_POSIX_SOURCE
#ifdef GETHOSTNAME_FROM_SYSTEMINFO
  #include <sys/systeminfo.h>
  #define gethostname(buf,len) sysinfo(SI_HOSTNAME,buf,len)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server.h"
#include "http.h"
#include <unistd.h>

char URLbase[90] = "" ;

void createbase(unsigned port) {
  char namebuffer[256] ;
  if ( -1 == gethostname(namebuffer,256) ) {
    perror("couldn't find hostname") ;
    exit(1) ;
  }
  sprintf(URLbase, "http://%s:%u/", namebuffer, port);
}
	

void forkit(char const *progname) {
  int i ;
  const char *browser = getenv("BROWSER") ;
  if ( browser == NULL ) {
    printf("%s\n",URLbase);
    fclose(stdout); /* then,  browser `cmixshow foo.ann`   might work */
    return ;
  }
  i = fork() ;
  if ( i == -1 ) {
    fprintf(stderr,
	    "Couldn't fork! Try pointing a browser at\n"
	    "%s and see if that works.\n",URLbase) ;
  } else
    if ( i == 0 ) {
      execlp(browser,browser,URLbase,(char*)NULL);
      fprintf(stderr,
	      "%s: couldn't exec %s. Try to point a browser\n"
	      "at %s and see if that works.\n",
	      progname,browser,URLbase) ;
      exit(0) ;
    }
}

void
webfront(const char *progname)
{
  createbase( server_init(progname) ) ;
  signal(SIGCHLD,server_stop) ;
  forkit(progname) ;
  server_loop(progname,http_service) ;
}
