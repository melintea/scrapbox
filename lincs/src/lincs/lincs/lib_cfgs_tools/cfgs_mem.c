
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

#include <string.h>
#include <errno.h>
#include <time.h>

#include "cfgs_error.h"
#include "cfgs_log.h"
#include "cfgs_mem.h"


/* this function exists with the only purpose to offer a stable breakpoint 
   on simulated mem allocation failure */
void
will_fail( void )
{
    LOG( cfgs_log(CFGST_LL_CRITIC, "memory function will return NULL\n"); );
    errno = ENOMEM;
}


#define FAIL_FREQ  (121)   /* how often to ret NULL */

static bool
random_fail( void )
{
    static int call_count = -1;  /* should be protected with a mutex */
    
    if ( call_count < 0 ) {
        srand( (unsigned)time(NULL) );
        call_count = ( rand() % FAIL_FREQ );
    }
    
    if ( (++call_count % FAIL_FREQ) == 0 ) {
        will_fail();
        return true;
    }
    
    return false;
}


void *
xmalloc( size_t num )
{
   void *new;
  
#ifdef RANDOM_NULL
    if ( random_fail() )
        return NULL;
#endif

   new = malloc( num );
   if ( !new ) 
      LOG( cfgs_log(CFGST_LL_CRITIC, "xmalloc returns NULL\n"); );

   return new;
}


void *
xrealloc( void *p, size_t num )
{
  void *new;


#ifdef RANDOM_NULL
    if ( random_fail() )
        return NULL;
#endif


  errno = 0;
  
  if ( !p )
    return xmalloc( num );

  new = realloc( p, num );
  if ( !new || errno == ENOMEM ) 
      LOG( cfgs_log(CFGST_LL_CRITIC, "xrealloc error\n"); );


  return new;
}


void *
xcalloc (size_t num, size_t size)
{
  void *new;

    
#ifdef RANDOM_NULL
    if ( random_fail() )
        return NULL;
#endif


  errno = 0;

  new = xmalloc( num * size );
  if ( !new ) {
      LOG( cfgs_log(CFGST_LL_CRITIC, "xcalloc returns NULL\n"); );
      return NULL;
  }

  bzero( new, num * size );
  return new;
}


void 
xfree( void *p )
{
    if ( p )
        free( p );
}


char *
xstrdup( const char *string )
{
    char *new;

#ifdef RANDOM_NULL
    if ( random_fail() )
        return NULL;
#endif

    new = strdup( string );
    if ( !new ) 
        LOG( cfgs_log(CFGST_LL_CRITIC, "xstrdup returns NULL\n"); );
    
    return new;
}




