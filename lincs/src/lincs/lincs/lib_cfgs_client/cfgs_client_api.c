
/*
 *  $Revision: 1.8 $
 *  $Date: 2004/03/31 21:20:42 $
 *
 *  Client API
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

#include <stdio.h>
#include <stdlib.h>

#include "cfgs/cfgs_config.h"
#include "cfgs_client_api.h"
#include "cfgs_backend.h"
#include "cfgs_mem.h"
#include "cfgs_log.h"
#include "cfgs_dlist.h"
#include "cfgs_sock.h"
#include "cfgs_protocol.h"


static const char    m_module[]  = CFGS_BOOTSTRAP_BACKEND; 

static cfgs_backend  *m_backend  = NULL;
static int           m_connect   = CFGST_INVALID_SOCKET;

/* FIXME: logically, m_connect&co should be in session to allow threading ? */


static int 
connect_daemon()
{
    /* FIXME: catch SIGPIPE, set alarm */
    
    /*int sock = cfgst_connect( CSST_INET, "127.0.0.1", CFGS_CONFIGD_PORT );*/
    int sock = cfgst_connect( CSST_UNIX, CFGS_CONFIGD_PATH, CFGS_CONFIGD_PORT/*useless*/ );
    //FIXME: send credentials ?
    
    return sock;
}


static void
disconnect_daemon()
{
    cfgst_disconnect( m_connect );
}


cfgs_session *
cfgs_connect( void )
{
    cfgs_session *sess = NULL;
    
    /* FIXME: if cannot connect to daemon, load bootstrap */  
    m_connect = connect_daemon();
    if ( m_connect < 0 ) {
        /* FIXME: move it out ? */
        if ( !cfgsb_init() ) 
            return (cfgs_session*)0;
     
        m_backend = cfgsb_load_backend( m_module );
    }
    
    if ( !m_backend && m_connect == CFGST_INVALID_SOCKET ) 
        return (cfgs_session*)0;
    
    sess = cfgs_session_new();
    
    return sess;
}


bool 
cfgs_disconnect( cfgs_session *c )
{
    lassert( m_backend != NULL || m_connect > 0 );
    if ( m_connect > 0 ) {
        disconnect_daemon();
        m_connect = CFGST_INVALID_SOCKET;
    } else {
        cfgsb_unload_backend( m_backend ); 
        m_backend = NULL;
        /* FIXME: move it out ? */
        (void)cfgsb_shutdown();
    }
    lassert( m_backend == NULL && m_connect == CFGST_INVALID_SOCKET );
    
    cfgs_session_free( c );
    
    return true; 
}


cfgs_entry*
cfgs_getval( cfgs_session *sess, const char *name, const char *layer )
{
    cfgs_entry     *pv = NULL;
    
    if ( !sess || !name )
        return NULL;
    
    if ( !layer ) {
        layer = CFGS_DEFAULT_LAYER; 
    }
    
    if ( m_connect != CFGST_INVALID_SOCKET ) {
        pv = (cfgs_entry*)cfgsp_send_rq( sess, m_connect, CFGSP_HOST_PROTO_HTTP, 
                CFGS_GETVAL, name, layer );
    } else {
        lassert( m_backend != NULL );
        pv = (*m_backend->cfgs_getval)( sess, name, layer );
    }
    
    return pv;
}


static bool
set_layer( cfgs_entry *head )
{
    cfgs_entry *val;
    
    for ( val=head;  val; val=val->next ) {
        if ( !cfgs_entry_attr(val, CFGS_EA_LAYER) ) {
            if ( !cfgs_entry_add_attr(val, CFGS_EA_LAYER, CFGS_DEFAULT_LAYER) ) {
                return false;
            }
        }
    }
    
    return true;
}


int 
cfgs_setval( cfgs_session *sess, cfgs_entry *vl )
{
    int        nvals = 0;
    
    if ( !sess || !vl )
        return -1;
    
    if ( !set_layer(vl) )
            return -1;
    
    if ( m_connect != CFGST_INVALID_SOCKET ) {
        nvals = (int)cfgsp_send_rq( sess, m_connect, CFGSP_HOST_PROTO_HTTP, CFGS_SETVAL, vl ); 
    } else {
        lassert( m_backend != NULL );
        nvals = (*m_backend->cfgs_setval)( sess, vl );
    }
    
    return nvals;
}


int 
cfgs_rmval( cfgs_session *sess, const char *name, const char *layer )
{
    int nvals = 0;
    
    if ( !sess || !name )
        return -1;
    
    if ( !layer ) {
        layer = CFGS_DEFAULT_LAYER; 
    }
    
    if ( m_connect != CFGST_INVALID_SOCKET ) {
        nvals = (int)cfgsp_send_rq( sess, m_connect, CFGSP_HOST_PROTO_HTTP, 
                CFGS_RMVAL, name, layer ); 
    } else {
        lassert( m_backend != NULL );
        nvals = (*m_backend->cfgs_rmval)( sess, name, layer );
    }
    
    return nvals;
}


/* FIXME: move it into cfgs_stacker, session dependent? */
cfgs_str *
cfgs_get_backends( void )
{
    cfgs_str      *strs = NULL;
    
    lassert( m_backend != NULL || m_connect > 0 );
    
    if ( m_backend ) {
        cfgs_backend  *bk   = m_backend;
    
        while ( bk && bk->name ) {
            cfgs_str *bkn = cfgs_str_new( bk->name );
            if ( !bkn ) {
                cfgs_str_free( strs );
                return NULL;
            }
    
            strs = (cfgs_str*)cfgs_dlist_add_tail( (cfgs_dlist*)strs, (cfgs_dlist*)bkn );
            
            bk = bk->next;
        }
    } else if ( m_connect != CFGST_INVALID_SOCKET ) {
        /*FIXME*/
    }
    
    return strs;
}


const cfgs_err*
cfgs_geterror( cfgs_session *s )
{
    lassert( s != NULL );
    return cfgs_session_geterr( s );
}


cfgs_stats*
getstats( cfgs_session *sess )
{
    return NULL; /*FIXME*/
}


int 
cfgs_register_notif( cfgs_session *sess, cfgs_notif *notif )
{
    int ret;
    
    if ( !sess || !notif )
        return -1; 
    
    if ( m_connect == CFGST_INVALID_SOCKET ) {
        cfgs_session_store_error( sess, CFGS_ERRT_INTERNAL, 
                CFGSP_ERR_SERVER_CONNECT, NULL );
        return -1; 
    }
    
    ret = (int)cfgsp_send_rq( sess, m_connect, CFGSP_HOST_PROTO_HTTP, 
                    CFGS_REG_NOTIF, notif ); 
    return ret; 
}


cfgs_str *
cfgs_getsubvals( cfgs_session *sess, const char *valname, const char *layer )
{
    cfgs_str     *pv = NULL;
    
    if ( !sess || !valname )
        return NULL;
    
    if ( !layer ) {
        layer = CFGS_DEFAULT_LAYER; 
    }
    
    if ( m_connect != CFGST_INVALID_SOCKET ) {
        pv = (cfgs_str*)cfgsp_send_rq( sess, m_connect, CFGSP_HOST_PROTO_HTTP, 
                CFGS_GETSUBVALS, valname, layer );
    } else {
        lassert( m_backend != NULL );
        pv = (*m_backend->cfgs_getsubvals)( sess, valname, layer );
    }
    
    return pv;
}


cfgs_str *
cfgs_getsublayers( cfgs_session *sess, const char *layername )
{
    cfgs_str     *pv = NULL;
    
    if ( !sess || !layername )
        return NULL;
    
    if ( m_connect != CFGST_INVALID_SOCKET ) {
        pv = (cfgs_str*)cfgsp_send_rq( sess, m_connect, CFGSP_HOST_PROTO_HTTP, 
                CFGS_GETSUBLAYERS, layername );
    } else {
        lassert( m_backend != NULL );
        pv = (*m_backend->cfgs_getsublayers)( sess, layername );
    }
    
    return pv;
}


cfgs_str *
cfgs_getinfos( cfgs_session *sess )
{
    cfgs_str     *pv = NULL;
    
    if ( !sess )
        return NULL;
    
    if ( m_connect != CFGST_INVALID_SOCKET ) {
        pv = (cfgs_str*)cfgsp_send_rq( sess, m_connect, CFGSP_HOST_PROTO_HTTP, 
                CFGS_GETINFOS );
    } else {
        lassert( m_backend != NULL );
        pv = (*m_backend->cfgs_getinfos)( sess );
    }
    
    /* The xml parsing lib cuts \n, so we replace the ';' placeholders.  */
    if ( pv && pv->name ) {
        char *p = pv->name;
	    while ( *p++ ) {
	        if ( *p == ';' ) *p = '\n';
	    }
    }
    
    return pv;
}


