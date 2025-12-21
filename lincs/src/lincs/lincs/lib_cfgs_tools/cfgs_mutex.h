
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *  \file cfgs_mutex.h
 *  Mutex manipulation tools: check for deadlocks, etc. 
 *  FIXME: check pthread docs. 
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

#ifndef CFGS_MUTEX_H
#define CFGS_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif


#include <pthread.h>


/** Max. number of mutexes we take care of */
#define CFGS_MO_MAX (126)

/** Locking attempts will timeout after \def MUTEX_LOCK_TOUT seconds.  */
#define MUTEX_LOCK_TOUT (10)


/** 
 * Order of acquiring mutexes known to the system.  One that is not registered: 
 * the mutex used by cfgs_log.c, which has a prio of CFGS_MO_MAX+1.  
 */
#define CFGS_MO_CONN         (10)  /* cfgs_configd.c */
#define CFGS_MO_BACKENDS     (20)  /* cfgs_configd.c */
#define CFGS_MO_NOTIF_QUEUE  (30)  /* cfgs_configd.c */
#define CFGS_MO_NOTIF_LIST   (40)  /* cfgs_configd.c */


#define REGISTER_MUTEX( m, ord ) \
    cfgs_mutex_register( m, ord, #m ":" __FILE__ ); 
/** 
 * Register a mutex so that checks can be performed upon (deadlocks, etc.). 
 * @param ord has CFGS_MP_MAX maximum value and tells in which order to acquire
 * mutexes.  The higher the value, the later you can acquire it.  
 */
void cfgs_mutex_register( pthread_mutex_t *m, int ord, const char *infos );

/**
 * Same as pthread_mutex_lock.  Will timeout after MUTEX_LOCK_TOUT.  
 */
int cfgs_mutex_lock( pthread_mutex_t *m );

/*FIXME: provide non blocking attempts to lock or polling */ 
int cfgs_mutex_islocked( pthread_mutex_t *m );

/**
 * Same as pthread_mutex_unlock. 
 */
int cfgs_mutex_unlock( pthread_mutex_t *m );

/**
 *  Prints to file descriptor @param fd the current situation. 
 */
void cfgs_mutex_dump( int fd );


/**
 *  Checks that current thread has no mutex locked
 */
void assert_no_mutex( void );

#ifndef _NDEBUG
#  define lassert_no_mutex() assert_no_mutex() 
#else
#  define lassert_no_mutex() 
#endif


#ifdef __cplusplus
}
#endif

#endif /*CFGS_MUTEX_H*/

