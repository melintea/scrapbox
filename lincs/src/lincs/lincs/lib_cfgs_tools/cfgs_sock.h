
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

#ifndef CSSOCK_H
#define CSSOCK_H

#ifdef __cplusplus
extern "C" {
#endif


#include <sys/socket.h>
#include <netinet/in.h> /*in_addr_t*/


#define CFGST_INVALID_SOCKET  (-1)


typedef enum {
    CSST_UNIX = AF_UNIX,
    CSST_INET = AF_INET,
} CSST;

bool cfgst_accept_conn( int clisock );

/**
 *  Starts a stream AF_INET/AF_UNIX server on port and serve clients by starting 
 *  a start_func thread, as long as run_flag is true.  Will store the socket 
 *  in srvsock.  Newly created thread is detached.  Socket is non-blocking.  
 *
 *  @param accept_conn is a user supplied function to check on the newly
 *  created client socket 'clisock' if to accept connection or not.  If 
 *  NULL, cfgst_accept_conn is used by default.  
 *
 *  FIXME: not tested on unix sockets.  
 */
typedef struct _cfgs_sock_srv {
    CSST  type;
    int   port; 
    void* (*start_func)(void*); 
    bool  run_flag; 
    int   srvsock; 
    bool  (*accept_conn)(int clisock);
    int   sigpipe; /**< File descriptor that offers an alternate way 
            to signal the socket server that it has to finish.  
            Useful if signals are blocked and the server is stuck in 
            select.  */
} cfgs_sock_srv; 

bool cfgst_start_server( cfgs_sock_srv *ss );

/** Socket is non-blocking.  Connection attempt will timeout after 
    CGFS_SOCK_TOUT.  @param host is the path to the server socket if
    @param type is CSST_UNIX */
int  cfgst_connect( CSST  type, const char* host, int port );

/** 
 * Returns 0 on success, returns -1 on error and errno is set.  
 * See shutdown man page.  
 */
int  cfgst_disconnect( int sock );

/** 
 * For @param flags see the man (2) send flags.  Same for @return values. 
 * If returned value is less than @param buflen, one should check errno.  
 */
int  cfgst_fullsend( int sock, const char *buf, int buflen, int flags );
/** For @param flags see the man recv (2) flags */
int  cfgst_fullrecv( int sock, char *buf, int buflen, int flags );

/** Restarts if interrupted. See man read (2) */
int  cfgst_rread( int fd, void *buf, int len ); 
/** Restarts if interrupted. See man write (2) */
int  cfgst_rwrite( int fd, const void *buf, int len ); 
/** Restarts if interrupted. See man send (2) */
int  cfgst_rsend( int sock, const char *buf, int buflen, int flags );
/** Restarts if interrupted. See man recv (2) */
int  cfgst_rrecv( int sock, char *buf, int buflen, int flags );

in_addr_t cfgst_host_to_ip( const char* name );

#define CFGST_SE       unsigned char
#define CFGST_SE_READ   (0x01)
#define CFGST_SE_WRITE  (0x02)
#define CFGST_SE_EXCEPT (0x04)
/** 
 * Sleep microseconds. Returns same codes as select(2), that is 0 on 
 * timeout, sock on event, -1 on error.  @param sock can be 
 * CFGST_INVALID_SOCKET.  You may want to clear errno after.  
 */
int cfgst_microsleep( int sock, CFGST_SE se, int microsec );


#ifdef __cplusplus
}
#endif

#endif /*CSSOCK_H*/
 
