
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:24:51 $
 *
 *  Test tool: send content of the specified file to the server/client. 
 *  Works with Unix sockets.
 */
/*
# 
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


#include "../lincs/cfgs/cfgs_hostcfg.h"
#include "../lincs/cfgs/cfgs_config.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h> /*in_addr_t*/
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>



#if defined (TCLIENT)
#  define PROGNAME   "dcli"
#elif defined (TSERVER)
#  define PROGNAME   "dsrv"
#endif
const char progname[] = PROGNAME;


#ifndef CFGS_CONFIGD_PATH
#  define CFGS_CONFIGD_PATH    "/tmp/cfgs_configd.sock"
#endif
#ifndef CFGS_CONFIGD_PORT
#  define CFGS_CONFIGD_PORT    (9000)
#endif
#ifndef CFGST_INVALID_SOCKET
#  define CFGST_INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#  define SOCKET_ERROR         (-1)
#endif


static void 
usage( void )
{
    fprintf( stderr, 
#if defined (TCLIENT)
"Test client.  Send file content to the server. \n"
#elif defined (TSERVER)
"Test \"server\".  Send file content to the client. \n"
#endif
"Usage: %s <file name> \n"
"\n", 
    progname );
}

static void 
printerr( const char *s )
{
    if ( errno != 0 )
        fprintf( stderr, "%s: [%d]: %s\n", s, errno, strerror(errno) );
}


static int 
sconnect( const char* path )
{
    int                    sockfd = CFGST_INVALID_SOCKET;
    struct sockaddr_un    cliaddr;
    int                 ret;
    
    sockfd = socket( AF_LOCAL, SOCK_STREAM, 0 );
    
    memset( &cliaddr, 0, sizeof(cliaddr) );    
    cliaddr.sun_family = AF_LOCAL;
    strcpy( cliaddr.sun_path, path ); //FIXME: overflow

    ret = connect( sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr) );
    if ( ret < 0 ) {
        printf( "Could not connect to %s\n", path ); printerr( "connect" );
        close( sockfd );
        return CFGST_INVALID_SOCKET;
    }

    printf( "[%d]: connected to %s\n", sockfd, path );
    return sockfd;
}


static int  
sdisconnect( int sock )
{
    /* 2: send and receives disallowed */
    int ret = shutdown( sock, 2 );
    
    if ( ret != 0 ) { 
        printf( "Error shuting down socket %d\n", sock); printerr("shutdown");
    }
    
    return ret;
}


static int 
rsend( int sock, const char *buf, int len, int flags )
{
    int ret;
    
    do {
        ret = send( sock, buf, len, flags );
    } while ( ret == -1 && errno == EINTR );
    return ret;
}


static int 
fullsend( int sock, const char *buf, int buflen, int flags )
{
    int sendsofar = 0, lastsend = 0;

    while ( sendsofar < buflen ) {
        lastsend  = rsend( sock,
                          buf + sendsofar*sizeof(char),
                          buflen - sendsofar,
                          flags );
        if ( lastsend == SOCKET_ERROR ) {
            if ( errno == EINTR ) {
                continue;
            } else {
                printf( "cfgst_fullsend error\n"); printerr("send");
                return SOCKET_ERROR;
            }
        } else {
            sendsofar += lastsend;
        }
    }

    return sendsofar;
}


static int  
rrecv( int sock, char *buf, int len, int flags )
{
    int ret;
    
    do {
        ret = recv( sock, buf, len, flags );
    } while ( ret == -1 && errno == EINTR );
    return ret;
}


static int 
fullrecv( int sock, char *buf, int buflen, int flags )
{
    int recvsofar = 0, lastrecv = 0;

    while ( recvsofar < buflen )
    {
        lastrecv  = rrecv( sock,
                          buf + recvsofar*sizeof(char),
                          buflen - recvsofar,
                          flags );
        /*socket closed*/
        if ( lastrecv == 0 )  {
            return recvsofar;
        } else if ( lastrecv == SOCKET_ERROR ) {
            /* FIXME: if EWOULDBLOCK, what to do? Also fullsend.  */
            if ( errno == EINTR ) {
                continue;
            } else {
                printerr("recv");
                return SOCKET_ERROR;
            }
        } else {
            recvsofar += lastrecv;
        }
    }

    return recvsofar;
}



static off_t
fsize( const char *file )
{
    struct stat s;
    off_t       ret;
   
    assert( file != NULL );
        
    ret = stat( file, &s );
    if ( ret==0 && S_ISREG(s.st_mode) ) 
        return s.st_size;
    
    return 0;
}


/* NULL term */
static char*
load_file( const char *fname )
{
    off_t   flen = fsize( fname );
    char    *txt = NULL;
    int     f;
    
    if ( flen <= 0 )
        return NULL;
        
    f = open( fname, O_RDONLY );
    if ( f == -1 )
        return NULL;
        
    txt = calloc( flen+1, 1 );
    assert( txt );
    
    read( f, txt, flen );
    
    close( f );
    return txt;
}


static int
send_file( const char *fname, int sock )
{
    int        sent = 0;
    char       *txt = load_file( fname );
    off_t      flen = fsize( fname );
    int        ret = EXIT_SUCCESS;
    int        rlen; /*read bytes*/

    if ( !txt )
        return EXIT_FAILURE;
    
    sent = fullsend( sock, txt, flen, 0 );
    if ( sent != flen || errno != 0 ) {
        printerr("fullsend"); 
        ret = EXIT_FAILURE;
    }
        
    /* recv answer, print it to stdout.  No answer is expected from client if
       running as a server.   */
    do {
        char       recv_buf[ 1024 ];
        
        memset( recv_buf, 0, 1024 );
        rlen = fullrecv( sock, recv_buf, 1023, 0 );
        /* server: expect a ret==-1 || receive the request */
        if ( rlen == SOCKET_ERROR ) {
            ret = EXIT_FAILURE;
        } else {
            /* assuming all text.  NULL terminated */
            fprintf( stdout, "%s", recv_buf );
        }
    } while ( rlen > 0 );
    
    free( txt );
    /* No err ret at this point. Let the test script decide 
     return ret; */
    return EXIT_SUCCESS;
}

 
static int
client( const char *fname )
{
    int        sock;
    int        ret = EXIT_SUCCESS;
    
    errno = 0; 
    sock = sconnect( CFGS_CONFIGD_PATH ); 
    if ( sock == CFGST_INVALID_SOCKET || errno != 0 ) {
        printerr("sconnect");
        return EXIT_FAILURE;
    }
    
    ret = send_file( fname, sock );
    
    sdisconnect( sock );
    return ret;
}


static int 
rselect( int n, fd_set *rds, fd_set *wds, fd_set *eds, struct timeval *tout )
{
    int rc;
    
    do {
        rc = select( n, rds, wds, eds, tout );
    } while ( rc == -1 && errno == EINTR );
    
    return rc;
}


int 
server( const char *fname )
{
    int       clilen;
    int       srvsock=CFGST_INVALID_SOCKET, chldsock=CFGST_INVALID_SOCKET;
    fd_set    recv_fdset;
    int       addr_len;
    int       ret;
    struct sockaddr_un   cli_addr, serv_addr, local_addr;
    
    FD_ZERO(&recv_fdset);

    if ( (srvsock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
        printerr("Can't open socket stream\n"); 
        return EXIT_FAILURE;
    }

    unlink( CFGS_CONFIGD_PATH );

    memset( (char*)&serv_addr, 0, sizeof(serv_addr) );
    serv_addr.sun_family      = AF_UNIX;
    strcpy( serv_addr.sun_path, CFGS_CONFIGD_PATH );

    ret = bind( srvsock, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );
    if ( ret < 0 ) {
        printerr("bind");
        return EXIT_FAILURE;
    }

    ret = listen( srvsock, 5 ); 
    if ( ret < 0 ) {
        printerr("listen");
        return EXIT_FAILURE;
    }

    FD_SET( srvsock, &recv_fdset );
    
    {
        int i = 1;
        /* no SO_REUSEPORT */
        setsockopt( srvsock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i) );
    }

    printf( "Server waiting on socket: %s\n", CFGS_CONFIGD_PATH); 
    for( ;; ) {
        ret = rselect( srvsock + 1, &recv_fdset, NULL, NULL, NULL );
        if ( ret < 0 )  {
            if ( errno == EINTR ) {
                printf( "    Interrupted. Continuing...\n" );
                continue;
            }

            printerr("select");
            return EXIT_FAILURE;
        } else if ( ret == 0 ) {
            assert( 0 ); /*should never happen*/
        } else 
            break;        
    }

    if ( FD_ISSET(srvsock, &recv_fdset) ) {
        clilen = sizeof(cli_addr);
        chldsock = accept( srvsock, (struct sockaddr*)&cli_addr, &clilen );
        if ( chldsock < 0 )             {
            printerr("accept");
            return EXIT_FAILURE;
        }

        addr_len = sizeof(struct sockaddr_in);
        ret = getsockname( chldsock, (struct sockaddr*)&local_addr, &addr_len );
        if ( ret < 0 ) {
            printerr("getsockname");
            return EXIT_FAILURE;
        }

        send_file( fname, chldsock );
    } else {
        printerr("fdset"); 
    }
    
    close( srvsock );
    return EXIT_SUCCESS;
}


int
main( int argc, char **argv, char **envp )
{
    int        ret = EXIT_SUCCESS;
    const char *fname = NULL;
    
    usage();

    if ( argc == 2 ) {
        fname = argv[1];
    } else
        return EXIT_FAILURE;

#if defined (TCLIENT)
    ret = client( fname );    
#elif defined (TSERVER)
    ret = server( fname );    
#else
error Must define one of the previous defines. 
#endif
    
    return ret;
}

