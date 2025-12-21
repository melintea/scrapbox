
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *  \file cfgs_mutex.c
 *  Mutex manipulation tools: check for deadlocks, etc. 
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
#include "cfgs_log.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cfgs_mutex.h"


#define CFGS_INVALID_THR  ((pthread_t)(-1))


typedef struct _cfgs_mutex {
    pthread_mutex_t *mutex;
    int             order;  /**< priority...*/
    pthread_t       thread; 
    const char      *infos;
} cfgs_mutex;


/* FIXME: replace it with a map */ 
/* +one for cfgs_log mutex, last entry */
static cfgs_mutex m_mutexes[ CFGS_MO_MAX+1 ] = {0};



static int 
find_ord( pthread_mutex_t *m )
{
    int ord = -1, i;
    
    for ( i=0; i<CFGS_MO_MAX; i++ ) {
        if ( m_mutexes[i].mutex == m ) {
            ord = m_mutexes[i].order; 
            break;
        }
    }

    if ( ord < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "mutex not cfgs_mutex_register'ed \n"); );
    }
    lassert( ord >= 0 );
        
    return ord;
}


/* assert no mutex of higher prio taken by current thread */
static void
assert_no_higher( int ord )
{
    pthread_t self = pthread_self(); 
    int       i;
    int       fd = _cfgs_logfd() >= 0 ? _cfgs_logfd() : 2; 
    
    for ( i=0; i<CFGS_MO_MAX; i++ ) {
        if (  m_mutexes[i].thread == self 
           && m_mutexes[i].order > ord
           ) {
             cfgs_mutex_dump( fd ); 
             cfgs_mutex_dump( 2 ); 
             lassert( false );
        }
    }
}


void 
cfgs_mutex_register( pthread_mutex_t *m, int ord, const char *infos )
{
    lassert( ord <= CFGS_MO_MAX );
    
    if ( m_mutexes[ord].mutex ) {
        /* mutex already registered for 'ord' order */
        lassert( false ); /* programming error */
    }
    
    m_mutexes[ord].order  = ord;
    m_mutexes[ord].mutex  = m;
    m_mutexes[ord].infos  = infos;
    m_mutexes[ord].thread = CFGS_INVALID_THR; 
}


/* How often we try  to lock the mutex, tries per second */
/* FIXME: use a cond var instead */
#define LOCK_RESOLUTION  (20)
#define NANOS            (1000000000)
#define ATTEMPTS         (MUTEX_LOCK_TOUT * LOCK_RESOLUTION)
int 
cfgs_mutex_lock( pthread_mutex_t *m )
{
    int      ret = 0;
    int      ord;
    long     attempts = ATTEMPTS; 
    int      fd = _cfgs_logfd() >= 0 ? _cfgs_logfd() : 2; 
    
    lassert( m != NULL );
    
    ord = find_ord( m );
    
    LOG( cfgs_log(CFGST_LL_CRITIC, 
            "cfgs_mutex_lock %p attempt by thread %ld ...\n", 
            m, pthread_self()); );
    assert_no_higher( ord ); 
    
    do {
        const struct timespec ts = { 0, NANOS/LOCK_RESOLUTION };

        ret = pthread_mutex_trylock( m );
        if ( ret == 0 )
            break;
        nanosleep( &ts, NULL ); /*FIXME: remaining time, ret code */
        /*pthread_yield();  /* mising in older versions */
    } while ( --attempts > 0 );
    
    if ( ret != 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, 
                "cfgs_mutex_lock(%d): %ld pthread_mutex_trylock(%p) \n", 
                ret, pthread_self(), m); );
        cfgs_mutex_dump( fd ); 
        cfgs_mutex_dump( 2 ); 
        return ret;
    }

    /* account it */
    if ( ord >= 0 ) {
        m_mutexes[ord].thread = pthread_self(); 
    }
    
    LOG( cfgs_log(CFGST_LL_CRITIC, 
            "cfgs_mutex_lock %p by thread %ld \n", 
            m, pthread_self()); );
    return ret;
}


int 
cfgs_mutex_unlock( pthread_mutex_t *m )
{
    int ret;
    int ord;
    int fd = _cfgs_logfd() >= 0 ? _cfgs_logfd() : 2; 
    
    lassert( m != NULL );
    
    ord = find_ord( m );
    lassert( ord >= 0 );
    
    LOG( cfgs_log(CFGST_LL_CRITIC, 
            "cfgs_mutex_UNlock %p attempt by thread %ld ...\n", 
            m, pthread_self()); );
    ret = pthread_mutex_unlock( m );
    if ( ret != 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, 
                "cfgs_mutex_UNlock(%d): %ld pthread_mutex_unlock %p\n", 
                ret, pthread_self(), m); );
        cfgs_mutex_dump( fd ); 
        cfgs_mutex_dump( 2 ); 
        return ret;
    }
    
    /* cleanup corresponding entry */
    if ( ord >= 0 ) {
        m_mutexes[ord].thread = CFGS_INVALID_THR; 
    }
    
    LOG( cfgs_log(CFGST_LL_CRITIC, 
            "cfgs_mutex_UNlock %p by thread %ld \n", 
            m, pthread_self()); );
    return ret;
}


#define LINE_SZ (128)
void 
cfgs_mutex_dump( int fd )
{
    int  cfgst_rwrite( int fd, const void *buf, int len ); 
    int        i;
    const char hdr[] = 
        "From thread %ld: \n"
        "  Order Mutex   Thread  infos \n";
        /* 0123456789.123456789.123456789.123456789. */
    char       line[ LINE_SZ+1 ] = {0};
    unsigned   llen;
    
    llen = snprintf( line, LINE_SZ, hdr, pthread_self() );  
    cfgst_rwrite( fd, line, llen );
    
    for ( i=0; i<CFGS_MO_MAX; i++ ) {
        if ( !(m_mutexes[i].mutex) )
            continue;
            
        /* print it */
        memset( line, 0, LINE_SZ );
        llen = snprintf( line, LINE_SZ, "  %03d %p %ld \t%s \n", 
                m_mutexes[i].order,  m_mutexes[i].mutex, 
        m_mutexes[i].thread, SAFE(m_mutexes[i].infos) );
        cfgst_rwrite( fd, line, llen );
    }
    
    cfgst_rwrite( fd, "\n", 1 );
}


void 
assert_no_mutex( void )
{
}


