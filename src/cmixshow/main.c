/* Author:   Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: Main program
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdio.h>
#include <string.h>
#include "annofront.h"
#include <unistd.h>

struct inpp_hooks const *inpp = &option_b_inpp ;

static void
usage_summary(const char* progname)
{
    fprintf(stderr,"Usage: %s [-bulwhs] filename\n",progname);
    exit(1);
}

int main(int argc, char**argv) {
  char const *filename = "-" ;
  char const *mode = "--" ; /* undocumented option: -- means default */

  /* First, parse options */
  
  switch(argc) {
  case 2:
      if ( argv[1][0] == '-' && argv[1][1] != '\0' )
          mode = argv[1] ;
      else
          filename = argv[1] ;
      break ;
  case 3:
      mode = argv[1] ;
      filename = argv[2] ;
      break ;
  default:
      mode = "-h" ;
      break ;
  }
  if ( mode[0] != '-' || strlen(mode) != 2 )
      usage_summary(argv[0]) ;

  /* select the right mode */

 restart:
  switch(mode[1]) {
  case '-': /* select default mode */
#ifndef NOSERVER_yes
      if ( getenv("BROWSER") != NULL )
          mode = "-s" ;
      else
#endif
          mode = "-b" ;
      goto restart ;
  case 's':
      inpp = &framed_html_inpp ;
      break ;
  case 'w':
      inpp = &single_html_inpp ;
      break ;
  case 'b':
      inpp = &option_b_inpp ;
      break ;
  case 'u':
      inpp = &option_u_inpp ;
      break ;
  case 'l':
      inpp = &option_l_inpp ;
      break ;
  case 'r':
      inpp = &option_r_inpp ;
      break ;
  default:
      usage_summary(argv[0]);
  }

  /* read the input... */

  readthetext(filename);

  /* if the mode is -s, branch off to the server code now */

  if ( mode[1] == 's' ) {
#ifdef NOSERVER_yes
      fprintf(stderr,"Sorry, the -s option was turned off at compile time.\n");
      exit(2);
#else
      webfront(argv[0]) ;
#endif
  } else {
      /* else figure out where the output is going. Start a pager
       * if the output is a terminal and we know the name of one
       */
      FILE *outfile = stdout ;

      if ( isatty(1) ) {
          const char *pager = getenv("PAGER") ;
          if ( pager != NULL && strlen(pager) < 75 ) {
              char pagercopy[80] ;
              strcpy(pagercopy,pager);
              /* ^^  because getenv may return a static buffer */
              outfile = popen(pagercopy,"w");
              if ( outfile == NULL )
                  outfile = stdout ; /* if the popen fails, fall back */
          }
      }

      PrettyPrint(outfile,GetMainTree(),0);
      if ( outfile != stdout )
          pclose(outfile);
  }
  return 0 ;
}
