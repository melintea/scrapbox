
/**
 *  $Revision: 1.3 $
 *  $Date: 2004/03/18 17:05:44 $
 *
 *  \file 
 *  \brief Backend manipulation API.  
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfgs_backend.h"
#include "cfgs_mem.h"
#include "cfgs_dlist.h"
#include "cfgs_log.h"


static bool  m_engine_init = false;


bool 
cfgsb_init( void )
{
    int  errs = 0;
    char *modpath = NULL;
    
    /* 
     * At bootstrap time, cfgsb_init is called twice, once by the
     * client lib that has to load cfgs_stacker, once by the stacker 
     * backend itself.  Run time, the daemon loads directly the stacker. 
     */
    lassert( m_engine_init == false );
    
    /*LTDL_SET_PRELOADED_SYMBOLS();*/
    errs = lt_dlinit();
    if ( errs ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgsb_init:connect error: '%s'\n", lt_dlerror()); );
        return false;
    }

#ifdef CFGS_MOD_PATH
    (void)lt_dladdsearchdir( CFGS_MOD_PATH );
#endif 
    /* FIXME: CFGS_ENV_MOD_PATH security problem, get rid of */
    modpath = getenv( CFGS_ENV_MOD_PATH );
    if ( modpath )
       (void)lt_dladdsearchdir( modpath );

    lt_dlmalloc = (lt_ptr(*)(size_t))xmalloc;
    lt_dlfree   = (void(*)(lt_ptr))xfree;
    /* discouraged by the ltdl.h doc  
    lt_dlrealloc= (lt_ptr(*)(lt_ptr,size_t))xrealloc;
    */
    
    m_engine_init = true;
    return true;
}


bool 
cfgsb_shutdown( void )
{
    int ret;
    
    lassert( m_engine_init == true );
    ret = lt_dlexit();
    if ( ret != 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgsb_shutdown:lt_dlexit ret: %d\n", ret); );
    }

    m_engine_init = false;    
    return true; 
}


cfgs_backend *
cfgsb_backend_new( void )
{
    return XCALLOC( cfgs_backend, 1 );
}


void       
cfgsb_backend_free( cfgs_backend *bk )
{
    lassert( bk != NULL );
    xfree( bk->name );
    xfree( bk );
}


cfgs_backend *
cfgsb_load_backend( const char *name )
{
    cfgs_backend *bk = cfgsb_backend_new();
    
    lassert( name );
    
    if ( !bk || !name )
        return NULL;
        
    lassert( m_engine_init == true );

    bk->name = xstrdup( name );
    if ( !bk->name ) {
        cfgsb_backend_free( bk );
        return NULL;
    }
    
    bk->handle = lt_dlopenext( name );
    if ( !bk->handle ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgsb_load_backend:lt_dlopenext error: '%s' for '%s'\n", lt_dlerror(), name?name:"NULL" ); );
        LOG( cfgs_log(CFGST_LL_CRITIC, "Please set '%s' accordingly\n", CFGS_ENV_MOD_PATH ); );
        fprintf( stderr, "cfgsb_load_backend:lt_dlopenext error: '%s' for '%s'\n", lt_dlerror(), name?name:"NULL" );
        fprintf( stderr, "Please set '%s' accordingly\n", CFGS_ENV_MOD_PATH );
        cfgsb_backend_free( bk );
        return NULL;
    }
    
#define X(b,a)  bk->a = lt_dlsym( bk->handle, #a );
    CFGS_API_EXPORTS
#undef X

    bk->on_load   = lt_dlsym( bk->handle, "on_load" );
    bk->on_unload = lt_dlsym( bk->handle, "on_unload" );
    if (  !bk->on_unload || !bk->on_load 
#define X(b,a)  || !bk->a
       CFGS_API_EXPORTS
#undef X
       || !(*bk->on_load)() ) {
        (void)cfgsb_unload_backend( bk );
        bk = NULL;
        LOG( cfgs_log(CFGST_LL_CRITIC, "Invalid backend '%s' \n", name); );
    }
    
    return bk;
}


bool       
cfgsb_unload_backend( cfgs_backend *bk )
{
    int errs;
    
    lassert( bk && bk->handle && bk->on_unload );
    
    (void)bk->on_unload();
    
    errs = lt_dlclose( bk->handle );
    if ( errs ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, 
                "cfgsb_unload_backend:lt_dlclose error: '%s' on '%s'\n", lt_dlerror(), bk->name); );
        return false;
    }

    cfgsb_backend_free( bk );
    
    return true; 
}


cfgs_backend *
cfgsb_load_backends( const char **names )
{
    const char **n   = names;
    cfgs_backend *head = NULL;
    
    lassert ( names );
    
    while ( n && *n ) {
        cfgs_backend *bk = cfgsb_load_backend( *n );
        if ( bk ) {
            head = (cfgs_backend*)cfgs_dlist_add_tail( (cfgs_dlist*)head, 
                           (cfgs_dlist*)bk );
        }
        
        n++;
    }
    
    return head;
}


bool
cfgsb_unload_backends( cfgs_backend *backends )
{
    bool       ret = true;
    cfgs_backend *bk = backends;
    
    while ( bk ) {
        cfgs_backend *bknext = bk->next;
        ret = ret && cfgsb_unload_backend( bk ); 
        bk  = bknext;
    }
    
    return ret;
}


cfgs_backend *
cfgsb_find_backend( cfgs_backend *bklist, const char *name )
{
    cfgs_backend *bk = bklist;
    
    lassert( name );
    if ( !name )
        return NULL;
    
    while ( bk ) {
        if ( bk->name && 0 == strcmp(bk->name, name) )
            return bk;
        bk = bk->next;
    }
    
    return NULL;
}



