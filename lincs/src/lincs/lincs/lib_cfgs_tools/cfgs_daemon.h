
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *
 */
/*
# 
# Copyright (c) 2003 Aurelian Melinte. 
# This file is part of LinCS/tiger.  
# 
# LinCS/tiger is distributed under the terms of the GNU Lesser General Public
# License version 2 or any later version.  See the file COPYING.LIB for copying 
# permission or http://www.gnu.org. 
#                                                                            
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  
# IMPLIED, without even the implied warranty of MERCHANTABILITY or FITNESS 
# FOR A PARTICULAR PURPOSE.  ANY USE IS AT YOUR OWN RISK. 
#                                                                            
# Permission to modify the code and to distribute modified code is granted, 
# provided the above notices are retained, and a notice that the code was 
# modified is included with the above copyright notice. 
# 
 */ 

#ifndef CSDAEMON_H
#define CSDAEMON_H

#ifdef __cplusplus
extern "C" {
#endif


#include <signal.h>

#ifndef _NSIG
#  define  _NSIG  NSIG
#endif

/** Like main */
typedef   int  daemon_func( int argc, char **argv, char **envp );
typedef   void sighandler_func( int );


typedef enum {
    CFGS_DH_SET_HANDLERS = 0, /**< Install handlers.  Default. */
    CFGS_DH_THREAD,           /**< Spawn a thread that will dispatch 
                                   received signals */
    CFGS_DH_SIGPIPE,          /**< See cfgs_sigpipe_start */
} CFGS_DH;


typedef struct _cfgs_daemon cfgs_daemon;
struct _cfgs_daemon {
    daemon_func *func;
    int  argc;
    char **argv;
    char **envp;
    int  sigpipe;             /**< See cfgs_sigpipe_start */
    bool single_instance;     /**< One daemon instance only */
    CFGS_DH install_handlers; /**< By default install handlers specified below. */ 
    sighandler_func *sighandlers[ _NSIG + 1 ];
};


/** ret ERROR_SUCCESS or ERROR_FAILURE or errno */
int cfgs_run_daemon( cfgs_daemon *d );


/** 
  Support tools: 
  
  A pipe of signals.  More details in 'Writing Reliable AIX Daemons'
  by Eric Agar.  Sample use: 

<pre>
int main(int argc, char **argv)
{
    int sigs[] = {SIGUSR1, SIGTERM};
    int rc;
    fd_set read_fds;
    int sock_fd, sigpipe_fd, max_fd;

    
    sigpipe_fd = cfgs_sigpipe_start(sigs, sizeof sigs / sizeof sigs[0]);
    if (sigpipe_fd == -1) {
        *** Unexpected error
    }

    *** Initialize the client request socket, and set up sock_fd:
    while ( true ) {
        FD_ZERO( &read_fds );
        FD_SET( sock_fd, &read_fds );
        FD_SET( sigpipe_fd, &read_fds );
        max_fd = (sock_fd > sigpipe_fd) ? sock_fd : sigpipe_fd;

        do {
            rc = select( max_fd+1, &read_fds, NULL, NULL, NULL );
        } while ( rc == -1 && errno == EINTR );

        if (rc == -1) {
            *** Unexpected error
            break;
        }

        if ( FD_ISSET(sigpipe_fd, &read_fds) ) {
            if ( cfgs_sigpipe_caught(SIGUSR1) ) {
                *** Process SIGUSR1 request
            }
            if ( cfgs_sigpipe_caught(SIGTERM) ) {
                Process SIGTERM request
            }
        }

        if ( FD_ISSET(sock_fd, &read_fds) ) {
            *** Process client request with any routines needed
        }
    }

    (void)cfgs_sigpipe_stop(); 

    *** Call signal safe and/or unsafe routines to clean up 
    return 0;
}
</pre>

  Note that there can be only one per application 

*/ 
int  cfgs_sigpipe_start( int *sigs, int num_sigs );
int  cfgs_sigpipe_caught( int sig );
int  cfgs_sigpipe_stop( void );



#ifdef __cplusplus
}
#endif

#endif /*CSDAEMON_H*/

