
/*
 *  $Revision: 1.7 $
 *  $Date: 2004/03/31 21:20:42 $
 *
 *  \brief Stacker backend, implementng client API.  
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "cfgs_backend.h"
#include "cfgs_log.h"
#include "cfgs_mem.h"
#include "cfgs_dlist.h"
#include "cfgs_client_api.h"
#include "cfgs_str.h"


#define CFGS_BACKEND_NAME  "cfgs_stacker"

const char g_progname[] = CFGS_BACKEND_NAME;


/* 
 * exported functions:
 */
#define on_unload                 cfgs_stacker ## _LTX_on_unload
#define on_load                   cfgs_stacker ## _LTX_on_load

#if CFGS_CRT_REV != 4
#  error please update defines to CFGS_CRT_REV if needed
#endif
#define cfgs_getval                 cfgs_stacker ## _LTX_cfgs_getval
#define cfgs_setval                 cfgs_stacker ## _LTX_cfgs_setval
#define cfgs_rmval                  cfgs_stacker ## _LTX_cfgs_rmval
#define cfgs_getsubvals             cfgs_stacker ## _LTX_cfgs_getsubvals
#define cfgs_getsublayers           cfgs_stacker ## _LTX_cfgs_getsublayers
#define cfgs_getinfos               cfgs_stacker ## _LTX_cfgs_getinfos



/* backends managed by the stacker backend */
static const char *m_modules[] = {
    "cfgs_fs_bk",
    NULL
};

static cfgs_backend *m_backends = NULL;



cfgs_entry*
cfgs_getval( cfgs_session *sess, const char *name, const char *layer )
{
#undef cfgs_getval /* pesky macros, need to work with other backends */
    cfgs_entry   *pv = NULL;
    cfgs_backend *bk = m_backends;

    lassert( m_backends != NULL );
    
    if ( !sess || !name || !bk )
        return NULL;

    /*
     * FIXME: look in cache to find backend to poll for name, else poll all backends, then cache it
     */
    while ( bk && !pv ) {
        pv = (*bk->cfgs_getval)( sess, name, layer );
        bk = bk->next;
    }
    
    return pv;
}


int 
cfgs_setval( cfgs_session *sess, cfgs_entry *vl )
{
#undef cfgs_setval
    cfgs_backend *bk   = m_backends;
    int          nvals = 0;

    lassert( m_backends != NULL );
    
    if ( !sess || !vl || !bk )
        return -1;

    /*
     * FIXME: look in cache to find backend to poll for name, else poll all backends, then cache it
     */
    while ( bk ) {
        nvals += (*bk->cfgs_setval)( sess, vl );
        bk = bk->next;
    }
    
    return nvals;
}


int 
cfgs_rmval( cfgs_session *sess, const char *name, const char *layer )
{
#undef cfgs_rmval
    cfgs_backend *bk   = m_backends;
    int          nvals = 0;

    lassert( m_backends != NULL );
    if ( !sess || !name || !bk ) 
        return -1;

    /*
     * FIXME: look in cache to find backend to poll for name, else poll all backends, then cache it
     */
    while ( bk ) {
        nvals += (*bk->cfgs_rmval)( sess, name, layer );
        bk = bk->next;
    }
    
    return nvals;
}


int 
cfgs_register_notif( cfgs_session *s, cfgs_notif *notif )
{
    /* Not used.  Notifications are the responsability of the server.  */
    lassert( false );
    return 0; 
}


bool
on_load( void )
{
    bool ret = true;
    
#if 0
    /* 
     * At bootstrap time, cfgsb_init has to be already been called,
     * either by the client lib, either by the daemon.  
     */
    if ( !cfgsb_init() ) {
        LOG( cfgs_log(CFGST_LL_INFO, "backend '%s' on_load, cfgsb_init failed \n", g_progname); );
        return false;
    }
#endif
        
    m_backends = cfgsb_load_backends( m_modules );
    if ( !m_backends ) 
        ret = false;
        
    LOG( cfgs_log(CFGST_LL_INFO, "backend '%s' on_load, returning %d \n", g_progname, ret); );
    return ret;
}


bool
on_unload( void )
{
    bool ret;
    
    ret = cfgsb_unload_backends( m_backends ); 
    m_backends = NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "backend '%s' unloaded, returning %d \n", g_progname, ret); );
    return ret;
}


cfgs_str *
cfgs_getsubvals( cfgs_session *sess, const char *valname, const char *layer )
{
#undef cfgs_getsubvals 
    cfgs_str     *pv = NULL;
    cfgs_backend *bk = m_backends;

    lassert( m_backends != NULL );
    
    if ( !sess || !valname || !layer || !bk )
        return NULL;

    while ( bk && !pv ) {
        pv = (*bk->cfgs_getsubvals)( sess, valname, layer );
        bk = bk->next;
    }
    
    return pv;
}


cfgs_str *
cfgs_getsublayers( cfgs_session *sess, const char *layername )
{
#undef cfgs_getsublayers 
    cfgs_str     *pv = NULL;
    cfgs_backend *bk = m_backends;

    lassert( m_backends != NULL );
    
    if ( !sess || !layername || !bk )
        return NULL;

    while ( bk && !pv ) {
        pv = (*bk->cfgs_getsublayers)( sess, layername );
        bk = bk->next;
    }
    
    return pv;
}


cfgs_str *
cfgs_getinfos( cfgs_session *sess )
{
#undef cfgs_getinfos 
    cfgs_backend *bk = m_backends;
    cfgs_str *ret = cfgs_str_new( 
            "" PACKAGE " " VERSION "; "
            ";Backend " CFGS_BACKEND_NAME 
            ";    CFGST_LOGFILE_DIR:    " CFGST_LOGFILE_DIR
            ";    CSFG_PREFIX_PATH:     " CSFG_PREFIX_PATH
            ";    CFGS_BIN_PATH:        " CFGS_BIN_PATH
            ";    CFGS_MOD_PATH:        " CFGS_MOD_PATH
            /*";    CFGS_VALUES_ROOT_DIR: " CFGS_VALUES_ROOT_DIR*/
	     );
    
    lassert( m_backends != NULL );
    
    if ( !sess || !bk )
        return NULL;

    while ( bk ) {
        cfgs_str *pv = (*bk->cfgs_getinfos)( sess );
        if ( pv ) cfgs_str_cat( ret, pv->name );
        cfgs_str_free( pv );
	
        bk = bk->next;
    }
    return ret; 
}


