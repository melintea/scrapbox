
/*
 *  $Revision: 1.8 $
 *  $Date: 2004/03/31 21:20:42 $
 *
 *  Config daemon. 
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
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <fnmatch.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <sys/uio.h>  /*iovec*/

#include "cfgs_daemon.h"
#include "cfgs_log.h"
#include "cfgs_tags.h"
#include "cfgs_dlist.h"
#include "cfgs_sock.h"
#include "cfgs_backend.h"
#include "cfgs_protocol.h"
#include "cfgs_mutex.h"


#define PROGNAME   "cfgs_configd"
const char progname[] = PROGNAME;


static char          *m_argv0 = NULL;
static cfgs_daemon   m_daemon = {0};
static cfgs_sock_srv m_http_local_srv = {0};
/* Signal to the socket server that it is time to exit */
static int           m_pipe_stop_srv[ 2 ] = {-1, -1};


#define PIPE_OUT(p)  p[1]
#define PIPE_IN(p)   p[0]


/* FIXME: scandir ? */
/* 
 * Backends to be loaded at startup.  
 */
static const char *m_modules[] = {
    CFGS_BOOTSTRAP_BACKEND,
    NULL
};
/* List of backends loaded.  */
static cfgs_backend    *m_backends  = NULL;
static pthread_mutex_t m_backends_mutex  = PTHREAD_MUTEX_INITIALIZER;

/*
 *  Mechanism to control load of the server.  This really should be
 *  in the socket server code but then it will require tracking active
 *  threads.  
 */
static pthread_mutex_t m_conn_mutex = PTHREAD_MUTEX_INITIALIZER;
/* <= CGFS_MAX_CONNEXIONS */
static int             m_connexions = 0;

/*
 *  Notifications.  
 */
static cfgs_notif      *m_notif_requests   = NULL;
static pthread_mutex_t m_notif_list_mutex  = PTHREAD_MUTEX_INITIALIZER;
static int             m_notif_queue[2]    = {-1, -1};
static pthread_mutex_t m_notif_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t       m_notif_thr;



/* No logging, log has a mutex in.  */
/* Loose enforcement of CGFS_MAX_CONNEXIONS.  Since we check maximum
   before locking, it might happen we reject connexions when counter
   under maximum or accept conns when counter over max.  */
static bool
inc_conn_num( void )
{
    int ret;
    
    lassert( m_connexions >= 0 );
    
    if ( m_connexions > CGFS_MAX_CONNEXIONS ) 
        return false;
    
    ret = cfgs_mutex_lock( &m_conn_mutex );
    lassert( ret == 0 );
    if ( ret != 0 ) 
        return false;
    
    m_connexions++;
    
    ret = cfgs_mutex_unlock( &m_conn_mutex );
    lassert( ret == 0 );
    if ( ret != 0 ) 
        return false;
    
    return true;
}


static bool
dec_conn_num( void )
{
    int ret;
    
    ret = cfgs_mutex_lock( &m_conn_mutex );
    lassert( ret == 0 );
    if ( ret != 0 ) 
        return false;
    
    m_connexions--;
    
    ret = cfgs_mutex_unlock( &m_conn_mutex );
    lassert( ret == 0 );
    if ( ret != 0 ) 
        return false;
    
    lassert( m_connexions >= 0 );
    
    return true; 
}


static bool
close_pipe( int p[2] )
{
    bool rc = true; 
    
    if ( close(p[1]) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "close_pipe: close(1) \n"); );
        rc = false;
    }
    p[1] = -1;

    if ( close(p[0]) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "close_pipe: close(0) \n"); );
        rc = false;
    }
    p[0] = -1;
    
    return rc; 
}


#define PNB_OUT  0x01
#define PNB_IN   0x02
/*FIXME: there is a in/out inconsistency somewhere */ 
static bool
open_pipe( int p[2], unsigned int flags/*which end is non-blocking*/ )
{
    int file_flags;

    if ( pipe(p) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "open_pipe: pipe \n"); );
        (void)close_pipe( p );
        return false;
    }

    if ( flags & PNB_OUT ) {
        if ( (file_flags = fcntl(p[0], F_GETFL, 0)) == -1 ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "open_pipe: fcntl(F_GETFL) \n"); );
            (void)close_pipe( p );
            return false;
        }

        if ( fcntl(p[0], F_SETFL, (file_flags | O_NONBLOCK)) == -1 ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "open_pipe: fcntl(F_SETFL) \n"); );
            (void)close_pipe( p );
            return false;
        }
    }

    if ( flags & PNB_IN ) {
        if ( (file_flags = fcntl(p[1], F_GETFL, 0)) == -1 ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "open_pipe: fcntl(F_GETFL) \n"); );
            (void)close_pipe( p );
            return false;
        }

        if ( fcntl(p[1], F_SETFL, (file_flags | O_NONBLOCK)) == -1 ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "open_pipe: fcntl(F_SETFL) \n"); );
            (void)close_pipe( p );
            return false;
        }
    }
    
    return true;
}


static void
sighup_handler( int signum )
{
    int old_errno = errno; 

    /*FIXME: avoid logging from handler */
    LOG( cfgs_log(CFGST_LL_INFO, "csconfigd:sighup_handler(%d)\n", signum); );
    TEST_ERROR
    
    errno = old_errno;
}


static int stop_notifications_mechanism( void ); 

static void
sigterm_handler( int signum )
{
    m_http_local_srv.run_flag = false;
    cfgst_rwrite( PIPE_OUT(m_pipe_stop_srv), " ", 1 );
    /*close_pipe(); Closing it will block the daemon*/

    /* to unblock the notifications thread */ 
    cfgst_rwrite( PIPE_OUT(m_notif_queue), " ", 1 );
    stop_notifications_mechanism(); 
    
    LOG( cfgs_log(CFGST_LL_CRITIC, 
            "[%s] version %s: Exit on signal %d\n", m_argv0, VERSION, signum ); ); 
    syslog( LOG_NOTICE, "[%s] version %s: Exit on signal %d\n", m_argv0, VERSION, signum ); 
    /* FIXME: exit not signal safe but calls atexit funcs */ 
    _exit( EXIT_SUCCESS ); 
}


/* FIXME: do not log in a signal handler */
static void
sigpipe_handler( int signum )
{
    int old_errno = errno; 
    
    LOG( cfgs_log(CFGST_LL_CRITIC, "csconfigd:sigpipe_handler(%d)\n", signum); );
    /* FIXME: clear (h_)errno selectively */
    TEST_ERROR
    
    //if ( m_srv_sock != CFGST_INVALID_SOCKET ) {
    //    close( m_srv_sock );   
    //    m_srv_sock = CFGST_INVALID_SOCKET;
    //}
    
    errno = old_errno;
}


/*------------------------------------------------------------------*/

static bool
lock_backends( void )
{
    int ret;
    
    ret = cfgs_mutex_lock( &m_backends_mutex );
    lassert( ret == 0 );
    if ( ret != 0 ) 
        return false;
    
    return true;
}


static bool
unlock_backends( void )
{
    int ret;
    
    ret = cfgs_mutex_unlock( &m_backends_mutex );
    lassert( ret == 0 );
    if ( ret != 0 ) 
        return false;
    
    return true; 
}

/*------------------------------------------------------------------*/
/*
 * Notifications mechanism.  
 */

/*
 * Read/write into m_notif_queue records with the following format: 
 *     start_rec
 *     total_length of what follows
 *     valname lenght
 *     valname
 *     layer length
 *     layer
 *     end_rec
 * This should actually be xml formatted to keep things consistent.  
 *
 * Writer threads communicate with one reader thread through a pipe.  
 */
static const char start_rec[] = "<%start%>"; 
static const char end_rec[]   = "<%end%>"; 


/*
 * Note that queuing the valname change notif is unreliable:
 *   -pipe[1] is non-blocking
 *   -write should deal with EAGAIN (EINTR should not occur)
 */

/* FIXME: format the message in one buffer and write at once - atomic */
static int 
queue_change_notif( int fd, const char *valname, const char *layer )
{
    unsigned short vlen, llen, tlen;
    int            ret;
    
    lassert( fd && valname && layer );
    
    vlen = strlen( valname ) + 1;
    llen = strlen( layer ) + 1;
    tlen = 
         /*+ strlen(start_rec)+1*/
         /*+ sizeof(short) /*tlen*/
         + sizeof(short) /*vlen*/
         + vlen
         + sizeof(short) /*llen*/
         + llen
         + strlen(end_rec)
         ;
    
    if (  (ret=cfgst_rwrite(PIPE_OUT(m_notif_queue), start_rec, strlen(start_rec))) 
       != strlen(start_rec) ) {
        return -1; 
    }
    if (  (ret=cfgst_rwrite(PIPE_OUT(m_notif_queue), &tlen, sizeof(short))) != sizeof(short) ) { 
        return -1; 
    }
    if (  (ret=cfgst_rwrite(PIPE_OUT(m_notif_queue), &vlen, sizeof(short))) != sizeof(short) ) { 
        return -1; 
    }
    if (  (ret=cfgst_rwrite(PIPE_OUT(m_notif_queue), valname, vlen)) != (vlen) ) { 
        return -1; 
    }
    if (  (ret=cfgst_rwrite(PIPE_OUT(m_notif_queue), &llen, sizeof(short))) != sizeof(short) ) { 
        return -1; 
    }
    if (  (ret=cfgst_rwrite(PIPE_OUT(m_notif_queue), layer, llen)) != (llen) ) {
        return -1; 
    }
    if (  (ret=cfgst_rwrite(PIPE_OUT(m_notif_queue), end_rec, strlen(end_rec))) 
       != strlen(end_rec) ) {
        return -1; 
    }
    
    LOG( cfgs_log(CFGST_LL_INFO, "queue_change_notif %s | %s\n", layer, valname); ); 
    return 0;
}


static int
full_read( int fd, void *buf, int len )
{
    int ret = cfgst_rread( fd, buf, len );
    LOG( cfgs_log(CFGST_LL_CRITIC, "full_read(%d, %p, %d) %d\n", fd, buf, len, ret); ); 
    return ret;
}


static inline int
read_tag( int fd, const char *tag )
{
    char c  = '\0';
    const char *p = tag; 
    
    lassert( tag );
    LOG( cfgs_log(CFGST_LL_INFO, "read_tag(%d, %s)...\n", fd, tag); ); 
    
    while ( c != *p ) {
        if ( -1 == full_read(fd, &c, 1) ) { 
            return -1;
        }
    }
    
    for ( p++; *p; p++ ) {
        if ( -1 == full_read(fd, &c, 1) ) { 
            return -1;
        }
        if ( c != *p ) {
            return 0;
        }
    }
    
    return 1;
}


static int 
read_change_notif( int fd, char *val_buf, char *layer_buf )
{
    unsigned short vlen=0, llen=0, tlen=0;
    
    lassert( fd && val_buf && layer_buf );
    
    if ( 1 != read_tag(fd, start_rec) ) {
        return -1;
    }
    
    if ( sizeof(short) != full_read(fd, &tlen, sizeof(short)) ) {
        return -1;
    }
    
    if ( sizeof(short) != full_read(fd, &vlen, sizeof(short)) ) {
        return -1;
    }
    if ( vlen > FILENAME_MAX ) {
        return -1;
    }
    if ( vlen != full_read(fd, val_buf, vlen) ) {
        return -1;
    }
    
    if ( sizeof(short) != full_read(fd, &llen, sizeof(short)) ) {
        return -1;
    }
    if ( llen > FILENAME_MAX ) {
        return -1;
    }
    if ( llen != full_read(fd, layer_buf, llen) ) {
        return -1;
    }
    
    if ( 1 != read_tag(fd, end_rec) ) {
        return -1;
    }
    
    LOG( cfgs_log(CFGST_LL_INFO, "read_change_notif %s | %s\n", layer_buf, val_buf); ); 
    return 0;
}


static int 
send_notifications( const char *valname, const char *layer )
{
    int        nb_notifs = 0;
    cfgs_notif *pn;
    int        ret;
    
    LOG( cfgs_log(CFGST_LL_INFO, "send_notifications \n"); ); 
    for ( pn=m_notif_requests; pn; pn=pn->next ) {
        ret = fnmatch( pn->valname, valname, 0 ); 
        LOG( cfgs_log(CFGST_LL_INFO, 
                "send_notifications %d fnmatch'%s' <-> '%s' \n", 
                ret, valname, pn->valname); ); 
        if ( 0 == ret ) {
            /* FIXME: local/remote notif */ 
            lassert( pn->pid > 0 && pn->signal > 0 );
            errno = 0;
            ret = kill( pn->pid, pn->signal );
            LOG( cfgs_log(CFGST_LL_INFO, "send_notifications(%d) to %d, signal %d\n", 
                    ret, pn->pid, pn->signal); ); 
            if ( ret == 0 )
                nb_notifs++; 
        }
    }
    
    return nb_notifs;
}


static void *
notif_dispatcher( void *arg )
{
    char valname[ FILENAME_MAX+1 ] = {0};
    char layer[ FILENAME_MAX+1 ]   = {0};
    
    LOG( cfgs_log(CFGST_LL_INFO, 
            "notif_dispatcher started as thread %ld\n", pthread_self()); );

    while ( m_http_local_srv.run_flag && PIPE_IN(m_notif_queue) > 0 ) { 
        int ret;
    
        memset( valname, '\0', FILENAME_MAX+1 );
        memset( layer,   '\0', FILENAME_MAX+1 );
    
        /* There is only one reader thread */
        ret  = read_change_notif( PIPE_IN(m_notif_queue), valname, layer );
    
        if ( ret == 0 ) {
            ret = cfgs_mutex_lock( &m_notif_list_mutex );
            lassert( ret == 0 );
        
            send_notifications( valname, layer );
        
            ret = cfgs_mutex_unlock( &m_notif_list_mutex );
            lassert( ret == 0 );
        }
        
        /*sleep( 1 ); */
    }
    return NULL;
}


static int
start_notifications_mechanism( void )
{
    pthread_attr_t chld_attr;
    
    REGISTER_MUTEX( &m_notif_list_mutex,  CFGS_MO_NOTIF_LIST );
    REGISTER_MUTEX( &m_notif_queue_mutex, CFGS_MO_NOTIF_QUEUE );
    
    if ( !open_pipe(m_notif_queue, PNB_IN) ) {
        return EXIT_FAILURE;
    }
    
    /* Start notification dispatching thread */
    if ( 0 != pthread_attr_init(&chld_attr) ) {
        return EXIT_FAILURE;
    }
    if ( 0 != pthread_attr_setdetachstate(&chld_attr, PTHREAD_CREATE_DETACHED) ) {
        return EXIT_FAILURE;
    }
    if ( 0 != pthread_create(&m_notif_thr, &chld_attr, notif_dispatcher, NULL) ) {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


static int
stop_notifications_mechanism( void )
{
    return EXIT_SUCCESS;
}


static void 
queue_notification( const char *valname, const char *layer )
{
    int ret;
    
    ret = cfgs_mutex_lock( &m_notif_queue_mutex );
    lassert( ret == 0 );
    
    /* There is a specialized thread that sends notifications - it examines 
       messages with changed values and sends notifications if appropriate. 
       This function only queues a message that valname has changed.  */
    LOG( cfgs_log(CFGST_LL_INFO, "queue_notification %s : %s\n", 
            SAFE(layer), SAFE(valname)); ); 
        
    queue_change_notif( PIPE_OUT(m_notif_queue), valname, layer );
    
    ret = cfgs_mutex_unlock( &m_notif_queue_mutex );
    lassert( ret == 0 );
}

static int
add_notif( cfgs_notif *notif )
{
    int ret;
    
    lassert( notif );
    if ( !notif )
        return EXIT_FAILURE;
    
    LOG( cfgs_log(CFGST_LL_INFO, "add_notif %s\n", SAFE(notif->valname)); ); 
    
    ret = cfgs_mutex_lock( &m_notif_list_mutex );
    lassert( ret == 0 );
    if ( ret != 0 ) 
        return ret;
    
    m_notif_requests = (cfgs_notif*)cfgs_dlist_add_tail( (cfgs_dlist*)m_notif_requests, 
                               (cfgs_dlist*)notif );
    
    ret = cfgs_mutex_unlock( &m_notif_list_mutex );
    lassert( ret == 0 );
    
    if ( ret != 0 || !m_notif_requests ) 
        return ret;
    
    return EXIT_SUCCESS;
}

/*------------------------------------------------------------------*/

/*
 * Callback functions.  Check also tag_callback
 */
typedef void* callback_func( cfgsp_data* );
#define X(a,b)  static void * b##_rq_handler( cfgsp_data* );
CFGS_API_EXPORTS
#undef X
#define X(a,b)  b##_rq_handler,
callback_func *m_handlers[] = {
    CFGS_API_EXPORTS
};
#undef X
 

/*-------- navigation ----------------------------------------------*/

static void*
cfgs_getsubvals_rq_handler( cfgsp_data *data )
{
    cfgs_backend   *bk  = m_backends;
    cfgs_str       *sk = NULL; /*subkeys*/
    const char     *v;
    const char     *l;

    lassert( data && data->attribs );
    if ( !data || !data->attribs ) 
        return (void*)-1;
    
	v = cfgs_tag_attr( data->attribs, CFGS_EA_NAME ); 
	l = cfgs_tag_attr( data->attribs, CFGS_EA_LAYER ); 
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_getsubvals_rq_handler %s %s\n", 
            SAFE(v), SAFE(l)); );
	if ( !v || !l )
        return (void*)-1;

    while ( bk ) {
TEST_ERROR    
        sk = (*bk->cfgs_getsubvals)( data->sess, v, l );
        /* only one backend stores the value */
	    if ( sk )
	        break; 
TEST_ERROR        
        bk = bk->next;
    }
    
    return (void*)sk; /*FIXME: make sure sk is freed downstream!*/
}

static void*
cfgs_getsublayers_rq_handler( cfgsp_data *data )
{
    int            gret = 0;
    return (void*)gret;
}

/*------------------------------------------------------------------*/

static void* 
cfgs_getinfos_rq_handler( cfgsp_data *data )
{
    cfgs_backend   *bk  = m_backends;
    cfgs_str       *sk = cfgs_str_new( "" );

    lassert( data && data->attribs );
    if ( !data || !data->attribs || !sk ) 
        return (void*)-1;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_getinfos_rq_handler\n"); );

    while ( bk ) {
TEST_ERROR    
        cfgs_str *pv = (*bk->cfgs_getinfos)( data->sess );
        if ( pv ) cfgs_str_cat( sk, pv->name );
        cfgs_str_free( pv );
TEST_ERROR        
        bk = bk->next;
    }
    
    cfgs_str_cat( sk, 
            "; ;Daemon " PROGNAME
	        ";    CFGS_CONFIGD_PATH:    " CFGS_CONFIGD_PATH
	        /* ";    CFGS_CONFIGD_PORT:    " CFGS_CONFIGD_PORT
	        ";    CGFS_MAX_CONNEXIONS:  " CGFS_MAX_CONNEXIONS
	        ";    CGFS_SOCK_TOUT:       " CGFS_SOCK_TOUT " sec. " */
        );
	
    return (void*)sk; /*FIXME: make sure sk is freed downstream!*/
}


static void*
cfgs_setval_rq_handler( cfgsp_data *data )
{
    int            gret = 0, pret;
    cfgs_backend   *bk  = m_backends;
    cfgs_entry     *val = cfgs_entry_new();
    cfgs_pair      *p;

    lassert( data && data->attribs );
    if ( !data || !data->attribs || !val ) 
        return (void*)-1;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_setval_rq_handler %s\n", 
            SAFE(cfgs_tag_attr(data->attribs, CFGS_EA_NAME))); );

    val->value_type = CFGS_VT_UCPTR;
    for ( p=data->attribs->attr; p; p=p->next ) {
        if ( !cfgs_entry_add_attr(val, p->first, p->second) ) {
            cfgs_entry_free( val );
            return (void*)-1;
        }
    }
    
    /*
     * FIXME: look in cache to find backend to poll for name, else poll all 
     * backends, then cache it.  cs_get/set/rm/ val
     */
    while ( bk ) {
TEST_ERROR    
        pret = (*bk->cfgs_setval)( data->sess, val );
        /* only one backend stores the value */
        if ( pret < 0 ) {
            gret = -1;
            continue;
        } else {
            if ( pret ) {
                const char *valname = cfgs_entry_attr( val, CFGS_EA_NAME ); 
                const char *layer   = cfgs_entry_attr( val, CFGS_EA_LAYER ); 
                lassert( valname && layer );
                queue_notification( valname, layer );
            }
            gret += pret;
            break; 
        }
TEST_ERROR        
        bk = bk->next;
    }
    
    cfgs_entry_free( val );
    return (void*)gret;
}

static void*
cfgs_rmval_rq_handler( cfgsp_data *data )
{
    int            gret = 0, pret;
    cfgs_backend   *bk  = m_backends;
    const char     *valname, *layer; 

    lassert( data && data->attribs );
    if ( !data || !data->attribs ) 
        return (void*)-1;
    
    valname = cfgs_tag_attr( data->attribs, CFGS_EA_NAME );
    layer   = cfgs_tag_attr( data->attribs, CFGS_EA_LAYER );
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_rmval_rq_handler %s\n", SAFE(valname)); );
    if ( !valname || !layer )
        return (void*)-1;
    

    /*
     * FIXME: look in cache to find backend to poll for name, else poll all 
     * backends, then cache it.  cs_get/set/rm/ val
     */
    while ( bk ) {
TEST_ERROR    
        pret = (*bk->cfgs_rmval)( data->sess, valname, layer );
        if ( pret < 0 ) {
            gret = -1;
            break;
        }
        gret += pret;
        
        if ( pret ) {
            lassert( valname && layer );
            queue_notification( valname, layer );
        }
TEST_ERROR        
        bk = bk->next;
    }
    
    return (void*)gret;
}

static void*
cfgs_getval_rq_handler( cfgsp_data *data )
{
    cfgs_entry     *pv = NULL;
    cfgs_backend   *bk = m_backends;
    const char     *valname, *layer; 

    lassert( data && data->attribs );
    if ( !data || !data->attribs ) 
        return NULL;
    
    valname = cfgs_tag_attr( data->attribs, CFGS_EA_NAME );
    layer   = cfgs_tag_attr( data->attribs, CFGS_EA_LAYER );
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_getval_rq_handler %s\n", SAFE(valname)); );
    if ( !valname || !layer )
        return NULL;
    

    /*
     * FIXME: look in cache to find backend to poll for name, else poll all 
     * backends, then cache it.  cs_get/set/rm/ val
     */
    while ( bk && !pv ) {
TEST_ERROR    
        pv = (*bk->cfgs_getval)( data->sess, valname, layer );
TEST_ERROR        
        bk = bk->next;
    }
    
    return (void*)pv;
}

static void*
cfgs_register_notif_rq_handler( cfgsp_data *data )
{
    int        gret = 0;
    cfgs_notif *nn, *n; 
    
    lassert( data && data->attribs );
    if ( !data || !data->attribs ) {
        return NULL;
    }
    
    /* FIXME: should use cfgs_notifs_from_tags but 
       add_notif is not prepared to deal with lists */ 
    nn = cfgs_notif_from_tag( data->attribs );
    if ( !nn ) {
        return NULL;
    }
    
    for ( n=nn; n; n=n->next ) {
        int ret;
        
        ret = add_notif( n );
        if ( ret < 0 ) {
            /*FIXME: set error*/
            break; 
        }
        
        gret += 1;
    }
    
    /* Do NOT free, it is now appended to m_notif_requests */
    /*cfgs_notif_free( nn );*/
    return (void*)gret;
}


/** @see CFGSP_CALLBACK definition */
static void* 
tag_callback( cfgsp_data *data )
{
    if ( !data->attribs )
        return NULL;
TEST_ERROR
    return (*m_handlers[data->idx])( data );
}

/*------------------------------------------------------------------*/

/** Report err to client */
static inline void 
report_error( int sock, CFGS_ERR err )
{
    /*FIXME*/
}


static inline void
EXIT_HPR( int csock, int code )
{
    close( csock ); /*shutdown(,2)*/ 
    lassert_no_mutex(); 
    LOG( cfgs_log(CFGST_LL_INFO, "*** thread done ***\n"); ); 
    pthread_exit( (void*)code ); 
}


/*
 * See Stevens - Unix Network Programming
 */
#define    CONTROL_LEN    (sizeof(struct cmsghdr) + sizeof(struct ucred))

static ssize_t
recv_cred( int sock, void *ptr, size_t nbytes, struct ucred *cred )
{
    struct msghdr    msg;
    struct iovec    iov[1];
    char            control[ CONTROL_LEN + 20 ];
    int                n, ret;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    msg.msg_flags = 0;
    
    ret = setsockopt( sock, SOL_SOCKET, SO_PASSCRED, &n, sizeof(int) );
    if ( ret ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "recv_cred:setsockopt: %d", ret); );
        return ret;
    }

    if ( (n = recvmsg(sock, &msg, 0)) < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "recv_cred:recvmsg: %d", n); );
        return( n );
	}

    cred->uid = CFGS_UID_NOBODY;    /* indicates no credentials returned */
    cred->gid = CFGS_GID_NOBODY;
    if ( cred && msg.msg_controllen > 0 ) {
        struct cmsghdr    *cmptr = (struct cmsghdr *) control;

        if ( cmptr->cmsg_len != sizeof(struct cmsghdr) + sizeof(struct ucred) ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "recv_cred:cmptr->cmsg_len: %d", cmptr->cmsg_len); );
        }
        if ( cmptr->cmsg_level != SOL_SOCKET ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "recv_cred:cmptr->cmsg_level: %d", cmptr->cmsg_level); );
        }
        if ( cmptr->cmsg_type != SCM_CREDENTIALS ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "recv_cred:cmptr->cmsg_type: %d", cmptr->cmsg_type); );
        }
        memcpy( cred, CMSG_DATA(cmptr), sizeof(struct ucred) );
    }

    return( n );
}


static bool
authenticate_remote_user( int csock, cfgs_session *sess )
{
    if ( m_http_local_srv.type == CSST_UNIX ) {
        struct ucred client;
        struct ucred *sess_creds;
        char         buf[ 100 ] ={0};
        int          n;

        /* Note that calling recv_cred with a request for 0 bytes we cannot tell
	       the EOF.  Anyway, it will be detected later.  */
        if ( ( n = recv_cred(csock, buf, 0/*sizeof(buf)*/, &client)) < 0 ) {
	        return false;
	    }

        lassert( buf[0] == 0 ); 
        
        cfgs_session_set_creds( sess, &client );
	    sess_creds = cfgs_session_get_creds( sess );
        LOG( cfgs_log(CFGST_LL_INFO, 
	        "authenticate_remote_user: uid=%d, gid=%d, pid=%d, ", 
		    sess_creds->uid, sess_creds->gid, sess_creds->pid); );
	    return true;
    }
    
    return true;
}


static void* 
http_process_request( void* arg )
{
    int          csock = (int)arg;
    cfgs_session *sess; 
    cfgsp_data   cb_data;
    bool         keep_alive = true;
    
    lassert( arg && csock >= 0 );
    
    errno = h_errno = 0;

    LOG( cfgs_log(CFGST_LL_CRITIC, 
        "csconfigd http_process_request on socket %d, thread %ld \n", 
        csock, pthread_self()); );
    
    if ( (sess = cfgs_session_new()) == NULL ) {
        EXIT_HPR( csock, ENOMEM );
    }
TEST_ERROR    
    if ( !authenticate_remote_user(csock, sess) ) {
        /* This seems to fail sometimes.  Keep going, user will be anonymous. */
	    /*FIXME: why*/
    }
TEST_ERROR    
    /* FIXME: This connection accounting should be done by the server itself
       but complicates decrementing the connection counter - server must
       keep track of threads. */
    if ( !inc_conn_num() ) {
        report_error( csock, CFGSP_MAX_CONN ); 
        EXIT_HPR( csock, 0 );
    }
    
    cb_data.sess = sess;
    cb_data.idx  = INVALID_CFGS_FUNC_INDEX;
TEST_ERROR    
    while ( keep_alive ) {
        if ( !lock_backends() ) {
            break;
        }
        
        /* Reset Keep-alive if errors */
        keep_alive = cfgsp_process_rq( csock, CFGSP_HOST_PROTO_HTTP, 
                         tag_callback, &cb_data ); 
TEST_ERROR /*errno 11 EAGAIN detected */    

        if ( !unlock_backends() ) {
            break;
        }
    } /*while*/
TEST_ERROR
    if ( !dec_conn_num() ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "http_process_request:dec_conn_num"); );
    }    
    EXIT_HPR( csock, 0 );
    return NULL; /* shut up compiler */
}


static int
start_server( void )
{
    REGISTER_MUTEX( &m_conn_mutex,   CFGS_MO_CONN );
    
    m_http_local_srv.type       = CSST_UNIX;
    m_http_local_srv.port       = CFGS_CONFIGD_PORT;
    m_http_local_srv.start_func = http_process_request;
    if ( !open_pipe(m_pipe_stop_srv, PNB_IN|PNB_OUT) ) {
        return EXIT_FAILURE;
    }
    m_http_local_srv.sigpipe    = PIPE_IN(m_pipe_stop_srv); 
    m_http_local_srv.run_flag   = true;
    if ( !cfgst_start_server(&m_http_local_srv) ) {
        m_http_local_srv.run_flag = false;
        LOG( cfgs_log(CFGST_LL_CRITIC, "Could not start server on port %d.\n", 
                CFGS_CONFIGD_PORT ); );
        return EXIT_FAILURE;
    }
    
    m_http_local_srv.run_flag = false;
    return EXIT_SUCCESS; 
}


int
csconfigd( int argc, char **argv, char **envp )
{
    LOG( cfgs_log(CFGST_LL_INFO, "csconfigd " VERSION " running.\n"); );
    
    if ( !cfgsb_init() ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgsb_init failed !.\n"); );
        return EXIT_FAILURE;
    }

    m_backends = cfgsb_load_backends( m_modules );
    if ( !m_backends ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "Could not load backends.\n"); );
        return EXIT_FAILURE;
    }
    REGISTER_MUTEX( &m_backends_mutex, CFGS_MO_BACKENDS );
    

    /* should unload ackends, etc. but we will exit anyway */ 
    if ( start_notifications_mechanism() != EXIT_SUCCESS ) {
        return EXIT_FAILURE; 
    }
    
    
    if ( start_server() != EXIT_SUCCESS ) {
        return EXIT_FAILURE; 
    }
    /* at this point the socket server has been stopped */

    
    stop_notifications_mechanism(); 
        
    cfgsb_unload_backends( m_backends );
    (void)cfgsb_shutdown();
    
    LOG( cfgs_log(CFGST_LL_INFO, "csconfigd " VERSION " exit.\n"); );
    return EXIT_SUCCESS;
}


int
main( int argc, char **argv, char **envp )
{
    m_argv0 = argv[ 0 ];
    
    m_daemon.argc = argc;
    m_daemon.argv = argv;
    m_daemon.envp = envp;
    m_daemon.func = csconfigd;
    m_daemon.single_instance = false; 
    
    m_daemon.install_handlers       = CFGS_DH_THREAD; 
    m_daemon.sighandlers[ SIGHUP ]  = sighup_handler;
    m_daemon.sighandlers[ SIGPIPE ] = sigpipe_handler; 
    m_daemon.sighandlers[ SIGIO ]   = SIG_IGN;
    m_daemon.sighandlers[ SIGURG ]  = SIG_IGN;
    m_daemon.sighandlers[ SIGFPE ]  = SIG_IGN;
    m_daemon.sighandlers[ SIGUSR1 ] = SIG_IGN;
    m_daemon.sighandlers[ SIGUSR2 ] = SIG_IGN;
    m_daemon.sighandlers[ SIGALRM ] = SIG_IGN;
    m_daemon.sighandlers[ SIGTERM ] = sigterm_handler;
    m_daemon.sighandlers[ SIGINT ]  = sigterm_handler;
    /*FIXME: SIGQUIT  */

#if defined(ONE_SHOT)
    /* No forking */
    return csconfigd( argc, argv, envp );
#else
    return cfgs_run_daemon( &m_daemon );
#endif
}
 

