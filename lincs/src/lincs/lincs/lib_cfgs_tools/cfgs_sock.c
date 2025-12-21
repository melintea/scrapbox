
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:19:24 $
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

#include "cfgs/cfgs_config.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>


#include <pthread.h>
#include <string.h>
#include <ctype.h> /*isdigit*/

#include <stdio.h>  /*BUFSIZ*/
#include <asm/param.h> /*MAXHOSTNAMELEN FIXME; not the right header*/

#include "cfgs_sock.h"
#include "cfgs_log.h"


#define SOCKET_ERROR  (-1)

static int rselect( int n, fd_set *rds, fd_set *wds, fd_set *eds, struct timeval *tout ); 

/* FIXME: in Linux, default beaviour is to restart interrupted system 
   calls.  Make sure to set this explicitely.  */

static bool
socket_nonblock( int sock )
{
    int file_flags; 
    int old_errno = errno;
    
    /* next call sets errno 38 ENOSYS on unix sockets */
    if ( (file_flags = fcntl(sock, F_GETFL, 0)) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "socket_nonblock:fnctl(F_GETFL) error\n"); );
        return false;
    }
    if ( fcntl(sock, F_SETFL, (file_flags | O_NONBLOCK)) ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "socket_nonblock:fnctl(F_SETFL) O_NONBLOCK error\n"); );
        return false;
    }
    
    /* FIXME: fcntl/FNDELAY */
    
    errno = old_errno;
    return true;
}


#if 0
static bool
socket_block( int sock )
{
    int file_flags; 
    
    if ( (file_flags = fcntl(sock, F_GETFL, 0)) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "socket_block:fnctl(F_GETFL) error\n"); );
        return false;
    }
    if ( fcntl(sock, F_SETFL, (file_flags & ~O_NONBLOCK)) ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "socket_block:fnctl(F_SETFL) ~O_NONBLOCK error\n"); );
        return false;
    }
    
    return true;
}
#endif 


static bool
prepare_server_socket_bsd( int *sock, int port )
{
    int       i = 1;
    int       ret;
    struct sockaddr_in serv_addr;
    
    *sock = CFGST_INVALID_SOCKET;

    if ( (*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "Can't open socket stream\n"); );
        return false;
    }

    if ( !socket_nonblock(*sock) ) {
        close( *sock );
        *sock = CFGST_INVALID_SOCKET;
        return false;
    }
    
    /* FIXME: SO_LINGER */ 
    
    memset( (char*)&serv_addr, 0, sizeof(serv_addr) );
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons((short)port);

    ret = bind( *sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );
    if ( ret < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "Can't bind local address: %d\n", ret); );
        close( *sock );
        *sock = CFGST_INVALID_SOCKET;
        return false;
    }

    ret = listen( *sock, 5 ); 
    if ( ret < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "listen failed: %d\n", ret); );
        close( *sock );
        *sock = CFGST_INVALID_SOCKET;
        return false;
    }

    /* no SO_REUSEPORT */
    setsockopt( *sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i) );
    
    return true; 
}


static bool
prepare_client_socket_bsd( int *chldsock, bool  (*accept_conn)(int chldsock) )
{
    int       addr_len;
    int       ret; 
    struct sockaddr_in local_addr = {0};

    addr_len = sizeof(struct sockaddr_in);
    ret = getsockname( *chldsock, (struct sockaddr*)&local_addr, &addr_len );
    if ( ret < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "getsockname failed: %d\n", ret); );
        close( *chldsock ), *chldsock = CFGST_INVALID_SOCKET; 
        return false;
    }

    if ( !(*accept_conn)(*chldsock) ) {
        /*FIXME*/
        close( *chldsock ), *chldsock = CFGST_INVALID_SOCKET; 
        return false; 
    }

    if ( !socket_nonblock(*chldsock) ) {
        close( *chldsock ), *chldsock = CFGST_INVALID_SOCKET; 
        return false;
    }
    
    /* FIXME: SO_KEEPALIVE but this goes against the 3 seconds 
       CGFS_SOCK_TOUT timeout period.  Check so_error for ETIMEDOUT */
       
    return true; 
}


static int
raccept( int s, struct sockaddr *addr, socklen_t *addrlen )
{
    int rc;
    
    do {
        rc = accept( s, addr, addrlen );
    } while ( rc == -1 && errno == EINTR );
    
    return rc;
}


static bool
prepare_client_socket_unix( int *chldsock, bool  (*accept_conn)(int chldsock) )
{
    int       addr_len;
    int       ret; 
    struct sockaddr_un local_addr = {0};

    addr_len = sizeof(struct sockaddr_un);
    ret = getsockname( *chldsock, (struct sockaddr*)&local_addr, &addr_len );
    if ( ret < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "getsockname failed: %d\n", ret); );
        close( *chldsock ), *chldsock = CFGST_INVALID_SOCKET; 
        return false;
    }

    if ( !(*accept_conn)(*chldsock) ) {
        /*FIXME*/
        close( *chldsock ), *chldsock = CFGST_INVALID_SOCKET; 
        return false; 
    }

    if ( !socket_nonblock(*chldsock) ) {
        close( *chldsock ), *chldsock = CFGST_INVALID_SOCKET; 
        return false;
    }
    
    /* FIXME: SO_KEEPALIVE but this goes against the 3 seconds 
       CGFS_SOCK_TOUT timeout period.  Check so_error for ETIMEDOUT */
       
    return true; 
}


static bool
prepare_server_socket_unix( int *sock, const char *path )
{
    int       ret;
    struct sockaddr_un serv_addr;
    
    *sock = CFGST_INVALID_SOCKET;

    if ( (*sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "Can't open socket stream\n"); );
        return false;
    }

    if ( !socket_nonblock(*sock) ) {
        close( *sock );
        *sock = CFGST_INVALID_SOCKET;
        return false;
    }
    
    /* FIXME: SO_LINGER */ 
    
    unlink( path );

    memset( (char*)&serv_addr, 0, sizeof(serv_addr) );
    serv_addr.sun_family      = AF_UNIX;
    strcpy( serv_addr.sun_path, path );

    ret = bind( *sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );
    if ( ret < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "Can't bind local address: %d\n", ret); );
        close( *sock );
        *sock = CFGST_INVALID_SOCKET;
        return false;
    }

    ret = listen( *sock, 5 ); 
    if ( ret < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "listen failed: %d\n", ret); );
        close( *sock );
        *sock = CFGST_INVALID_SOCKET;
        return false;
    }

    return true; 
}


bool 
cfgst_start_server( cfgs_sock_srv *ss )
{
    fd_set    recv_fdset;
    pthread_t      chld_thr;
    pthread_attr_t chld_attr;
    int       ret;
    int       maxfd; 
    const char *path = CFGS_CONFIGD_PATH;
    bool      bret = false; 
    
    lassert( ss->type == CSST_UNIX || ss->type == CSST_INET );

    pthread_attr_init( &chld_attr );
    pthread_attr_setdetachstate( &chld_attr, PTHREAD_CREATE_DETACHED );

    if ( ss->accept_conn == NULL )
        ss->accept_conn = cfgst_accept_conn;
    
    bret = false;
    if ( ss->type == CSST_UNIX ) {
        bret = prepare_server_socket_unix( &ss->srvsock, path/*FIXME ss->*/ );
    } else if ( ss->type == CSST_INET ) {
        bret = prepare_server_socket_bsd( &ss->srvsock, ss->port );
    }
    if (  !ret || !ss->srvsock > 0 ) {
        return false;
    }
    
    LOG( cfgs_log(CFGST_LL_INFO, "Server waiting on : %s\n", path); );
    LOG( cfgs_log(CFGST_LL_INFO, "cfgst_start_server sigpipe=%d, srvsock=%d \n", 
            ss->sigpipe, ss->srvsock); );
    while ( ss->run_flag == true ) {
        FD_ZERO(&recv_fdset);
        FD_SET( ss->srvsock, &recv_fdset );
        if ( ss->sigpipe > 0 ) {
            FD_SET( ss->sigpipe, &recv_fdset );
        }
        maxfd = (ss->sigpipe > ss->srvsock) ? ss->sigpipe : ss->srvsock; 
    
        ret = rselect( maxfd + 1, &recv_fdset, NULL, NULL, NULL ); 
        
        if ( ret < 0 )  {
            LOG( cfgs_log(CFGST_LL_CRITIC, "select failed: %d\n", ret); );
            return false;
        }
        /* interrupted system call */    
        errno   = 0;
        h_errno = 0;

        /* Socket server shutdown has been requested */ 
        if ( ss->sigpipe > 0 && FD_ISSET(ss->sigpipe, &recv_fdset) ) {
            ss->run_flag = false; 
            break; 
        }
        
        /* Serve client */
        if ( FD_ISSET(ss->srvsock, &recv_fdset) ) {
            int       clilen;
            int       chldsock;
            struct sockaddr_in cli_addr; 
    
            clilen = sizeof(cli_addr);
            chldsock = raccept( ss->srvsock, (struct sockaddr*)&cli_addr, &clilen );
            if ( chldsock < 0 )             {
                LOG( cfgs_log(CFGST_LL_CRITIC, "accept failed: %d\n", ret); );
                return false;
            }
        
            bret = false;
            if ( ss->type == CSST_UNIX ) {
                bret = prepare_client_socket_unix( &chldsock, ss->accept_conn );
            } else if ( ss->type == CSST_INET ) {
                bret = prepare_client_socket_bsd( &chldsock, ss->accept_conn );
            }
            if ( !bret ) {
                return false; 
            }
            lassert( chldsock != CFGST_INVALID_SOCKET );
    
#if defined(ONE_SHOT)
            /* no threading */
            (*ss->start_func)( (void*)chldsock );
#else
            /* process incoming request */
            pthread_create( &chld_thr, &chld_attr, ss->start_func, (void*)chldsock );
            LOG( cfgs_log(CFGST_LL_INFO, "Thread %ld serving incoming request \n", (long)chld_thr); );
#endif
        } /*serve client*/
    } /*while*/
    
    close( ss->srvsock ), ss->srvsock = CFGST_INVALID_SOCKET; 
    LOG( cfgs_log(CFGST_LL_INFO, "cfgst_start_server_unix exit \n"); );
    return true;
}


static int
rconnect( int sock, const struct sockaddr *saddr, socklen_t slen )
{
    int rc;
    
    do {
        rc = connect( sock, saddr, slen );
    } while ( rc == -1 && errno == EINTR );
    
    return rc;
}


/*
 *  Works with BSD sockets.  
 */
static int 
cfgst_connect_bsd( const char* host, int port )
{
    int                rsrv = CFGST_INVALID_SOCKET;
    struct sockaddr_in rsrvINETAddress;
    struct sockaddr    *rsrvSockAddrPtr = NULL;
    int                selret, conret;
    struct timeval     tval = {CGFS_SOCK_TOUT, 0}; 
    fd_set             rset, wset;

    lassert( host != NULL );

    rsrvSockAddrPtr = (struct sockaddr*)&rsrvINETAddress;
    memset( (char*)&rsrvINETAddress, 0, sizeof(rsrvINETAddress) );
    rsrvINETAddress.sin_family      = AF_INET;
    rsrvINETAddress.sin_addr.s_addr = cfgst_host_to_ip( host );
    rsrvINETAddress.sin_port        = htons( port );
    
    rsrv = socket( AF_INET, SOCK_STREAM, 0 );
    if ( !socket_nonblock(rsrv) ) {
        close( rsrv );
        return CFGST_INVALID_SOCKET;
    }

    conret = rconnect( rsrv, (struct sockaddr*)rsrvSockAddrPtr, sizeof(rsrvINETAddress) ); 
    if ( conret == 0 )
        goto ok;
    
    if ( conret < 0 && errno != EINPROGRESS ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "conect to %s:%d failed\n", host, port); );
        close( rsrv ); 
        return CFGST_INVALID_SOCKET;
    }
    errno = 0;
    
    /* 
     * Else wait for connection.  Cannot use cfgst_microsleep 
     */
     
    FD_ZERO( &rset );
    FD_SET( rsrv, &rset );
    wset = rset; 
    selret = rselect( rsrv+1, &rset, &wset, NULL, &tval ); 
    if ( selret == 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "Connected to %s:%d timedout\n", host, port); );
        close( rsrv );        /* timeout */
        errno = ETIMEDOUT;
        return CFGST_INVALID_SOCKET;
    }

    if ( FD_ISSET(rsrv, &rset) || FD_ISSET(rsrv, &wset) ) {
        int error=0, optret, len=sizeof(error);
        optret = getsockopt( rsrv, SOL_SOCKET, SO_ERROR, &error, &len ); 
        if ( optret < 0 || error != 0 ) {
            /* Solaris pending error */ 
            errno = error; 
            LOG( cfgs_log(CFGST_LL_INFO, "SOL_SOCKET SO_ERROR=%d\n", error); );
            /*perror( "cfgst_connect" );*/
            close( rsrv );
            return CFGST_INVALID_SOCKET;    
        }
    } else {
        LOG( cfgs_log(CFGST_LL_CRITIC, "select error: sockfd not set\n"); );
        close( rsrv );
        return CFGST_INVALID_SOCKET;
    }
        
ok:
    LOG( cfgs_log(CFGST_LL_INFO, "Connected to %s:%d\n", host, port); );
    return rsrv;
}


static int 
cfgst_connect_unix( const char* path )
{
    int                    sockfd = CFGST_INVALID_SOCKET;
    struct sockaddr_un    cliaddr;
    int                 ret;
    
    sockfd = socket( AF_LOCAL, SOCK_STREAM, 0 );
    
    if ( !socket_nonblock(sockfd) ) {
        close( sockfd );
        return CFGST_INVALID_SOCKET;
    }

    memset( &cliaddr, 0, sizeof(cliaddr) );    
    cliaddr.sun_family = AF_LOCAL;
    strcpy( cliaddr.sun_path, path ); //FIXME: overflow

    ret = connect( sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr) );
    if ( ret < 0 ) {
        //FIXME
        close( sockfd );
        return CFGST_INVALID_SOCKET;
    }

    LOG( cfgs_log(CFGST_LL_INFO, "[%d]: connected to %s\n", sockfd, path); );
    return sockfd;
}


int 
cfgst_connect( CSST  type, const char* host, int port )
{
    lassert( type == AF_UNIX || type == AF_INET );
    
    if ( type == CSST_INET ) {
        return cfgst_connect_bsd( host, port );
    } else if ( type == CSST_UNIX ) {
        return cfgst_connect_unix( host/*path*/ );
    } else {
        //FIXME: set errno
        return CFGST_INVALID_SOCKET;
    }
}


int  
cfgst_disconnect( int sock )
{
    /* 2: send and receives disallowed */
    int ret = shutdown( sock, 2 );
    
    if ( ret != 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "Error shuting down socket %d\n", sock); );
    }
    
    return ret;
}


int 
cfgst_fullsend( int sock, const char *buf, int buflen, int flags )
{
    int sent = 0, lastsend = 0;
    int no_tout = 1;

    lassert( buf );
    
    while ( sent < buflen ) {
        errno = 0;
        lastsend  = cfgst_rsend( sock,
                          buf + sent*sizeof(char),
                          buflen - sent,
                          flags );
        if ( lastsend < 0 ) {
            /* no need for timeout check, when remote side closes the 
               socket, an error will be returned */
            /* FIXME: get rid of no_tout */
            if ( no_tout && (errno == EAGAIN || errno == EINTR) ) {
                continue;
            } else {
                LOG( cfgs_log(CFGST_LL_CRITIC, "cfgst_fullsend error\n"); );
                return sent; 
            }
        } else if ( lastsend == 0 ) {
            return sent;
        } else {
            sent += lastsend;
        }
    }

    return sent;
}


int 
cfgst_fullrecv( int sock, char *buf, int buflen, int flags )
{
    int recvsofar = 0, lastrecv = 0;
    int no_tout = 1;

    lassert( buf );
    
    while ( recvsofar < buflen ) {
        errno = 0;
        lastrecv  = cfgst_rrecv( sock,
                          buf + recvsofar*sizeof(char),
                          buflen - recvsofar,
                          flags );
        LOG( cfgs_log(CFGST_LL_INFO, "cfgst_fullrecv %d\n", lastrecv); );
        
        /*socket closed*/
        if ( lastrecv == 0 )  {
            return recvsofar;
        } else if ( lastrecv < 0 ) {
            /* No EWOULDBLOCK, socket has been set non-blocking.  */
            if ( (errno == EAGAIN || errno == EINTR) && no_tout > 0 ) {
                LOG( cfgs_log(CFGST_LL_CRITIC, "cfgst_fullrecv" ); );
                errno = 0;
                no_tout = cfgst_microsleep( sock, CFGST_SE_READ, CGFS_SOCK_TOUT*1000000 ); 
TEST_ERROR                
                if ( no_tout < 0 )
                    return false; 
                continue;
            } else {
                return lastrecv; /*<0*/
            }
        } else {
            no_tout = 1;
            recvsofar += lastrecv;
        }
    }

    return recvsofar;
}


in_addr_t 
cfgst_host_to_ip( const char* name )
{
    struct hostent* hostStruct = NULL;
    struct in_addr* hostNode   = NULL;
    in_addr_t       ret = (in_addr_t)0;

    lassert( name != NULL );

    /* FIXME: inet_aton */
    ret = inet_addr( name );
    if ( ret != (in_addr_t)-1 ) 
        return ret;

    hostStruct = gethostbyname( name );
    if ( NULL == hostStruct )
        return( (in_addr_t)0 ); //not found

    /* extract IP */
    hostNode = (struct in_addr*) hostStruct->h_addr;
    LOG( cfgs_log(CFGST_LL_INFO, "%s IP: %s\n", name, inet_ntoa(*hostNode) ); );
    return( hostNode->s_addr );
}


/*
 * Restart select if call interrupted. 
 */
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
cfgst_microsleep( int sock, CFGST_SE se, int microsec )
{
    struct timeval tv;
    fd_set         rfs, wfs, efs; 
    int            ret;
    
    LOG( cfgs_log(CFGST_LL_CRITIC, "cfgst_microsleep(%d) \n", sock); );
    
    tv.tv_sec  = 0;
    tv.tv_usec = microsec;
    
    if ( sock == CFGST_INVALID_SOCKET ) {
TEST_ERROR    
        ret = rselect( 0, NULL, NULL, NULL, &tv );
TEST_ERROR        
    } else {
        FD_ZERO( &rfs ); if ( se & CFGST_SE_READ )   FD_SET( sock, &rfs );
        FD_ZERO( &wfs ); if ( se & CFGST_SE_WRITE )  FD_SET( sock, &wfs );
        FD_ZERO( &efs ); if ( se & CFGST_SE_EXCEPT ) FD_SET( sock, &efs );
TEST_ERROR        
        ret = rselect( sock+1, &rfs, &wfs, &efs, &tv );
TEST_ERROR        
    }
    
    LOG( cfgs_log(CFGST_LL_CRITIC, "cfgst_microsleep(%d) ret %d \n", sock, ret); );
    return ret;
}



static bool
is_our_domain( const char *host )
{
    char *p1, *p2;
    char lh[ MAXHOSTNAMELEN ] = {0};
TEST_ERROR    
    /* no  domain */
    if ( (p1=strchr(host, '.')) == NULL )
        return true;
    
    gethostname( lh, sizeof(lh) );
    if ( (p2=strchr(lh, '.')) == NULL )
        return true;
    
    if ( strcasecmp(p1, p2) == 0 )
        return true;
    
    /* FIXME: who sets this localdomain string ? */
    if ( strcasecmp(p1, ".localdomain") == 0 )
        return true;
    
    return false;
}

/* IP specific.  Do not allow options */
static bool
check_ip_options( int sock )
{
    int             ipproto;
    struct protoent *ip;
    unsigned char   optbuf[BUFSIZ/3];
    int             optsz = sizeof( optbuf );
TEST_ERROR    
    if ( (ip=getprotobyname("ip")) != NULL )
        ipproto = ip->p_proto;
    else
        ipproto = IPPROTO_IP;
    
    if (  getsockopt(sock, ipproto, IP_OPTIONS, (char*)optbuf, &optsz) == 0 
       && optsz != 0 ) {
       /* turn off options */
       if ( setsockopt(sock, ipproto, IP_OPTIONS, NULL, optsz ) != 0 )
           return false;
    }
    
    return true;
}


/* Do some checks, who is at the other end.  
   Not reentrant because of gethostbyname/addr */
/* FIXME: there is something fishy here: check on localhost.localdomain? */
static bool
accept_conn_inet( int sock )
{
    struct sockaddr_in  cli_addr;
    socklen_t           cli_len = sizeof( struct sockaddr_in );
    int                 un = 1;
    struct hostent      hp, *php;
    bool                hostok = false;
TEST_ERROR    
    if ( getpeername(sock, &cli_addr, &cli_len) < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "getpeername\n"); ); 
        return false;
    }
    
    /*FIXME: should we set SO_KEEPALIVE ? See cfgs_sock note too */
    if ( setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &un, sizeof(un)) < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "setsockopt(%d, SO_KEEPALIVE)\n", sock); ); 
        return false;
    }
    
    /* look up client name */
    cli_addr.sin_port = ntohs( (u_short)cli_addr.sin_port );
    php = gethostbyaddr( (const char *)&cli_addr.sin_addr, 
              sizeof(struct in_addr), cli_addr.sin_family );
    if ( php == NULL ) {
        /* could not find name, use dotted addr */
        php = &hp;
        php->h_name = inet_ntoa( cli_addr.sin_addr ); /*FIXME: inet_aton*/
        hostok = true;
    } else if ( is_our_domain(php->h_name) ) {
        /* check that remote host really is in our domain */
        char rhost[ 2*MAXHOSTNAMELEN + 1 ] = {0};
        
        strncpy( rhost, php->h_name, sizeof(rhost)-1 );
        if ( (php=gethostbyname(rhost)) != NULL ) {
            for ( ; php->h_addr_list[0]; php->h_addr_list++ ) {
                if ( 0 == strncmp(php->h_addr_list[0], (caddr_t)&cli_addr.sin_addr, 
                              sizeof(cli_addr.sin_addr) ) ) {
                    hostok = true;
                    break;
                }
            }
        }
    } else {
        hostok = true;
    }
    
    if ( !hostok )
        return false;
    
    /*
    if ( cli_addr.sin_port >= IPPORT_RESERVED ) {
        return false;
    }
    */
    
    /* Do not allow IP options */
    if ( !check_ip_options(sock) ) 
        return false;
    
    /*
    if ( getpeername(0, &cli_addr, cli_len) < 0 ) {
        return false;
    }
    */
    
    return true;
}


static bool
accept_conn_unix( int sock )
{
    /*FIXME*/
    return true;
}


/*
 *  FIXME: check:
 *    if AF_UNIX, get credentials
 *    if AF_INET, get remote addr & name, check against allowed list
 *      check that ntohs(port) < IPPORT_RESERVED
 *      reset sock options (do not allow + log)
 *
 */
bool 
cfgst_accept_conn( int sock )
{
    struct sockaddr_in  cli_addr;
    socklen_t           cli_len = sizeof( struct sockaddr_in );
TEST_ERROR    
    if ( getpeername(sock, &cli_addr, &cli_len) < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "getpeername\n"); ); 
        return false;
    }
    
    if ( cli_addr.sin_family == AF_INET ) {
        return accept_conn_inet( sock );
    } else if ( cli_addr.sin_family == AF_UNIX ) {
        return accept_conn_unix( sock );
    } else {
        lassert( false );
        return false;
    }

    return false;    
}


int
cfgst_rread( int fd, void *buf, int len )
{
    int ret;
    
    do {
        ret = read( fd, buf, len );
    } while ( ret == -1 && errno == EINTR );
    return ret;
}


int  
cfgst_rwrite( int fd, const void *buf, int len )
{
    int ret;
    
    do {
        ret = write( fd, buf, len );
    } while ( ret == -1 && errno == EINTR );
    return ret;
}


int 
cfgst_rsend( int sock, const char *buf, int len, int flags )
{
    int ret;
    
    do {
        ret = send( sock, buf, len, flags );
    } while ( ret == -1 && errno == EINTR );
    return ret;
}


int  
cfgst_rrecv( int sock, char *buf, int len, int flags )
{
    int ret;
    
    do {
        ret = recv( sock, buf, len, flags );
    } while ( ret == -1 && errno == EINTR );
    return ret;
}



