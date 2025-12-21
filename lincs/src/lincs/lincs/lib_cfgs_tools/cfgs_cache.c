
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:19:24 $
 *
 *  Cache object
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

#include <pthread.h>

#include "cfgs_log.h"
#include "cfgs_mem.h"
#include "cfgs_dlist.h"
#include "cfgs_hash.h"
#include "cfgs_cache.h"


struct _cfgs_cache
{
    pthread_mutex_t   mutex;               /* PTHREAD_MUTEX_INITIALIZER; */
    cfgs_dlist       *pos_elems;           /* keep positive hits in */
    cfgs_hash        *pos_hits;
    cfgs_hash        *neg_hits;            /* names of the negative hits */
    const char* (*get_name)( void* );      /* func to retrieve elem's name */
};


cfgs_cache *
cfgs_cache_new( const char* (*get_name)(void*) )
{
    cfgs_cache *c = XCALLOC( cfgs_cache, 1 );
    
    lassert( get_name != NULL );
    
    if ( c ) {
        pthread_mutex_init( &c->mutex, NULL );
        c->get_name = get_name;
        /*FIXME: hashes, list*/
    }
    
    return c;
}


void    
cfgs_cache_free( cfgs_cache *c, void (*free_item)(void *) )
{
    if ( !c )
        return;
    
    if ( free_item && c->pos_elems ) {
        CFGST_DLIST_FREE( c->pos_elems, free_item );
    }
    
    /*FIXME*/
    
    xfree( c );
}



