
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
#include "cfgs_log.h"

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <execinfo.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/types.h>

#if defined __CYGWIN32__ || defined __CYGWIN__
#  include <cygwin/ipc.h>
#  include <cygwin/sem.h>
#  include <cygwin/shm.h>
#else
#  include <sys/ipc.h>
#  include <sys/sem.h>
#  include <sys/shm.h>
#endif

#include <netdb.h>
#include <pthread.h>



#define FTOK_FILE     CFGST_LOGFILE_DIR "/" CFGST_LOGFILE_DEFAULT

static FILE          *m_log_fd  = NULL;
static CFGST_LOGLEVEL m_loglevel = CFGST_LL_DEFAULT;
static char           m_logname[FILENAME_MAX] = FTOK_FILE;


/*
 *  multithread/multiproc safe
 */
#ifdef CFGST_LOG_MULTIPROCESS
static key_t  m_semkey;
static int    m_semid = -1;
static key_t  m_shmkey;
static int    m_shmid = -1;
#endif

static pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;


#if 1
/* debugging help - put a breakpoint here */
#include <netdb.h>
void
is_error( void )
{
    return;
}

/*
 * Should be in API ?
 */
void 
cfgs_test_error( const char *file, int line )
{
    assert( file != NULL );
    if ( errno != 0 || h_errno != 0 ) {
        cfgs_log( CFGST_LL_CRITIC, "cfgs_test_error: %s/%d \n", file, line );
        is_error();
    }
}

#endif


static bool
lock()
{
    int ret;

#ifdef CFGST_LOG_MULTIPROCESS
    struct sembuf sem_lock = { 0, -1, 0 };
#endif
    
    ret = pthread_mutex_lock( &m_mutex );
    if ( ret != 0 ) {
        fprintf( stderr, "cslog: cannot lock mutex (err. %d) !\n", ret );
        return false;
    }
    
#ifdef CFGST_LOG_MULTIPROCESS
    ret = semop( m_semid, &sem_lock, 1 );
    if ( ret == -1 ) {
        fprintf( stderr, "cslog: cannot lock semaphore !\n" );
        return false;
    }
#endif

    return true;
}


static bool
unlock()
{
    int ret;

#ifdef CFGST_LOG_MULTIPROCESS
    struct sembuf sem_unlock = { 0, 1, 0 };
    
    ret = semop( m_semid, &sem_unlock, 1 );
    if ( ret == -1 ) {
        fprintf( stderr, "cslog: cannot unlock semaphore !\n" );
        return false;
    }
#endif

    ret = pthread_mutex_unlock( &m_mutex );
    if ( ret != 0 ) {
        fprintf( stderr, "cslog: cannot unlock mutex (err. %d) !\n", ret );
        return false;
    }

    return true;
}




static inline pid_t  
get_pid( void )
{
    return getpid();
}


static bool
can_log( CFGST_LOGLEVEL ll )
{
    if ( ll <= m_loglevel )
        return true;
    
    return false;
}


static int
log_level( void )
{
    int        ll = CFGST_LL_DEFAULT; 
    const char *ell = getenv( CFGS_LOGLEVEL );
    
    if ( ell && *ell && isdigit(*ell) )
        ll = atoi( ell );
    
    return ll;
}


static bool
initialize( void )
{
    FILE *fd;
    int  file_flags;
    int  old_errno = errno;
    
    if ( m_log_fd != NULL ) 
        return true;
    
    /* set m_loglevel based on CFGS_LOGLEVEL env. var. */
    m_loglevel = log_level();
    
    fd = fopen( m_logname, "a" ); 
    if ( !fd ) {
        perror( "open LOGFILE" );
        return false;
    }
    setvbuf( fd, NULL, _IONBF, 0 );

    /* next call sets errno 38 ENOSYS */
    if ( (file_flags = fcntl(fileno(fd), F_GETFL, 0)) == -1 ) {
        cfgs_close_log();
        return false;
    }

    if ( fcntl(fileno(fd), F_SETFL, (file_flags | O_NONBLOCK)) == -1 ) {
        cfgs_close_log();
        return false;
    }
    
#ifdef CFGST_LOG_MULTIPROCESS    
    m_semkey = ftok( FTOK_FILE, 's' );
    m_semid  = semget( m_semkey, 1, 666|IPC_CREAT );
    if ( m_semid == -1 ) {
        fprintf( stderr, "cslog: cannot semget !\n");
        return false;
    }
#endif
    
    m_log_fd = fd;
    errno = old_errno;
    return true;
}


#define STACK_DUMP_SZ  (20)
inline static void 
dump_stack( FILE *fp )
{
    void   *array[ STACK_DUMP_SZ ];
    size_t size;
    char   **strings;

    size    = backtrace( array, STACK_DUMP_SZ );
    strings = backtrace_symbols( array, size );

    if ( strings ) {
        int    i;
        for ( i = 0; i < size; i++ )
           fprintf( fp, "%s\n", strings[i] );
        free( strings );
    }
}


void 
cfgs_log( CFGST_LOGLEVEL level, const char* fmt, ... )
{
    va_list  args;
    time_t   ltime;
    char*    ptime;

    lassert( fmt != NULL );
    
    if ( !can_log(level) )
        return;
    
    if ( !initialize() ) {
        return;
    }


    if ( !lock() )
        return;

    time( &ltime );
    ptime = ctime( &ltime );
    
    fseek( m_log_fd, SEEK_END, 0 );
    fprintf( m_log_fd, "\n[%ld] %s", (long)get_pid(), ptime );
    if ( level <= CFGST_LL_CRITIC )
        fprintf( m_log_fd, "(errno=[%d] %s, h_errno=[%d] )\n", 
                errno, strerror(errno), h_errno );

    va_start( args, fmt );
    vfprintf( m_log_fd, fmt, args );
    va_end( args );
    
    if ( level == CFGST_LL_CRITIC ) {
        dump_stack( m_log_fd );
    }

    unlock();
}


void 
cfgs_close_log( void )
{
#ifdef CFGST_LOG_MULTIPROCESS
    if ( m_semid != -1 ) 
        semctl( m_semid, 0, IPC_RMID, 0), m_semid = -1;
#endif

    if ( m_log_fd != NULL ) {
        fclose( m_log_fd );
        m_log_fd = NULL;
    }
}


int 
_cfgs_logfd( void )
{
    if ( m_log_fd ) {
        return fileno( m_log_fd ); 
    }
    
    return -1; 
}


