/* Author:   Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix annotation browser: Server engine for Berkeley sockets
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "server.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

static int oursocket ;

unsigned server_init(const char *progname) {
    struct sockaddr_in ouraddr = {0};
    int addrlen ;
    
    oursocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP) ;
    if ( -1 == oursocket ) {
        perror("hmm...");
        fprintf(stderr,"%s: socket() failed\n",progname) ;
        exit(EXIT_FAILURE) ;
    }
    
    ouraddr.sin_family = AF_INET ;
    ouraddr.sin_port = 0 ;
    ouraddr.sin_addr.s_addr = INADDR_ANY ;
    if ( -1 == bind(oursocket,(struct sockaddr*)&ouraddr,sizeof ouraddr) ) {
        perror("hmm...") ;
        fprintf(stderr,"%s: bind() failed\n",progname) ;
        close(oursocket);
        exit(EXIT_FAILURE);
    }
    
    listen(oursocket,10) ;
    
    addrlen = sizeof ouraddr ;
    if ( -1 == getsockname(oursocket,(struct sockaddr*)&ouraddr,&addrlen) ) {
        perror("hmm...") ;
        fprintf(stderr,"%s: getsockname() failed\n",progname) ;
        close(oursocket);
        exit(EXIT_FAILURE);
    }	
    
    return ntohs(ouraddr.sin_port) ;
}

static volatile sig_atomic_t loop_go = 1 ;
static volatile sig_atomic_t served_anything = 0 ;

void server_stop(int notused) {
    if ( served_anything ) loop_go = 0 ;
}

void server_loop(const char *progname,void (*service)(FILE*,FILE*)) {
    int fd ;
    struct sockaddr_in sin ;
    FILE *fi, *fo ;
    int addrlen ;
    
    while (loop_go) {
        addrlen = sizeof sin ;
        fd = accept(oursocket,(struct sockaddr*)&sin,&addrlen) ;
        if ( -1 == fd ) {
            if ( errno == EINTR )
                continue ; /* probably it was the netscape that died... */
            else
                return ; /* panic and bail out */
        }
        served_anything = 1 ;
        if ( NULL == (fi=fdopen(fd,"rb")) ) {
            fprintf(stderr,
                    "%s: got connection but fdopen() failed!\n",progname);
            close(fd);
        } else if ( NULL == (fo=fdopen(fd,"wb")) ) {
            fprintf(stderr,
                    "%s: got connection but fdopen() failed!\n",progname);
            fclose(fi) ;
        } else {
            service(fi,fo) ;
            fflush(fo) ;
            fclose(fo) ;
            fclose(fi) ;
        }
    }
}
		
