
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
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined __CYGWIN32__ || defined __CYGWIN__
#  include <cygwin/ipc.h>
#  include <cygwin/sem.h>
#  include <cygwin/shm.h>
#else
#  include <sys/ipc.h>
#  include <sys/sem.h>
#  include <sys/shm.h>
#endif
#include <sys/stat.h>

#include "cfgs_daemon.h"
#include "cfgs_log.h"


#ifndef FOPEN_MAX
#  pragma message ("[No ANSI FOPEN_MAX ?]")
#  define FOPEN_MAX  NOFILE
#endif



static cfgs_daemon *m_daemon = NULL;


/* One instance only allowed */
static key_t m_sem_key;                /* Semaphore key */
static int   m_sem_id;                 
static pid_t m_sem_pid = (pid_t)-1;    /* Process that locked semaphore */


/* FIXME: expose exclusive functions to API  */

void 
cfgs_release_excl( void )
{
    if ( getpid() == m_sem_pid ) {
        (void)semctl( m_sem_id, 0, IPC_RMID, 0 );
    }
    return;
}


bool 
cfgs_get_excl( const char *excl_path, char excl_id )
{
    int   sem_perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    struct sembuf sem_ops[2] = {{0, 0, IPC_NOWAIT}, {0, 1, SEM_UNDO}};
    int rc;

    if ( (m_sem_key = ftok(excl_path, excl_id)) == (key_t)-1 ) {
        return false;
    }

    if ( (m_sem_id = semget(m_sem_key, 1, IPC_CREAT | sem_perms)) == -1 ) {
        return false;
    }

    /* not atomic, can be interrupted */
    do {
        rc = semop( m_sem_id, sem_ops, 2 );
    } while ( (rc == -1) && (errno == EINTR) );

    if ( rc == -1 ) {
        return false;
    }
    errno = 0;

    m_sem_pid = getpid();
    (void)atexit( cfgs_release_excl );
    return true;
}


/*
 *  Call getsockname() against the 1st 3 file descriptors.
 *  If a call fails, return false, indicating inetd did not start
 *  this process.
 *  Compare the socket addresses and port numbers obtained.
 *  If any differences are detected, return false, indicating inetd 
 *  did not start this process.
 */
static bool 
is_parent_inetd( void )
{
    int                fd;
    struct sockaddr_in sockaddr;
    int                sockaddrlen;
    unsigned long      addr;
    unsigned short     port;


    for ( fd = 0; fd <= 2; fd++ ) {
        sockaddrlen = sizeof sockaddr;
        if ( getsockname(fd, (struct sockaddr *) &sockaddr,&sockaddrlen) == -1 ) {
            if ((errno == EBADF) || (errno == ENOTSOCK)) {
                errno = 0; 
                return false;
            } else {
                LOG( cfgs_log(CFGST_LL_CRITIC, "is_parent_inetd:getsockname \n"); );
                exit( EXIT_FAILURE );
            }
        }

        if ( sockaddr.sin_family != AF_INET ) {
            return false;
        }

        if ( fd == 0 ) {
            addr = ntohl(sockaddr.sin_addr.s_addr);
            port = ntohs(sockaddr.sin_port);
        } else {
            if (  (addr != ntohl(sockaddr.sin_addr.s_addr))
               || (port != ntohs(sockaddr.sin_port)) ) {
                return false;
            }
        }
    }

    /* inetd started this process */
    return true;
}


void
sigchld_handler( int signo )
{
    pid_t pid;
    int   old_errno = errno; 
    
    do {
        pid = waitpid( (pid_t)-1, NULL, WNOHANG );
    } while ( (pid>(pid_t)0) || (pid==(pid_t)-1 && errno==EINTR) );
    
    errno = old_errno;
}


static bool
prevent_zombies( void )
{
    struct sigaction sa;
    
    sa.sa_handler = sigchld_handler;
    sigemptyset( &(sa.sa_mask) );
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if ( sigaction(SIGCHLD, &sa, NULL) ) return false;

    return true; 
}


static bool
ignore_signals( void )
{
    struct sigaction sa;
    
    sa.sa_handler = SIG_IGN;
    sigemptyset( &(sa.sa_mask) );
    sa.sa_flags = 0;

    if ( !prevent_zombies() )  return false; 

    if ( sigaction(SIGINT,    &sa, NULL) ) return false;
    if ( sigaction(SIGQUIT,   &sa, NULL) ) return false;
    if ( sigaction(SIGTTOU,   &sa, NULL) ) return false;
    if ( sigaction(SIGTTIN,   &sa, NULL) ) return false;
    if ( sigaction(SIGTSTP,   &sa, NULL) ) return false;
    /* immune from pgrp leader death */
    if ( sigaction(SIGHUP,    &sa, NULL) ) return false;
#ifdef SIGDANGER
    if ( sigaction(SIGDANGER, &sa, NULL) ) return false;
#endif

    return true; 
}


bool
close_files( bool started_by_inetd )
{
    int fd;
    int minfd = started_by_inetd ? 3 : 0;
    
    for ( fd=minfd; fd<FOPEN_MAX; fd++ )
        close( fd );
    errno = 0;        /* probably got set to EBADF from a close */
    
    if ( !started_by_inetd ) {
        fd = open( "/dev/null", O_RDWR );
        if ( fd >= 0 ) {
            if ( fd != 0 )
                dup2( fd, 0 );
            if ( fd != 1 )
                dup2( fd, 1 );
            if ( fd != 2 )
                dup2( fd, 2 ); 
            if ( fd > 2 )
                close( fd );
        } else
            return false; 
    }
    
    return true; 
}


static bool 
has_controlling_terminal( void )
{
    int   fd;
    char  ctermid_name[ L_ctermid ];


    /*
     *  Get the special path name for controlling terminals on this
     *  system.  It is probably "/dev/null" !? 
     */
    if ( ctermid(ctermid_name) == NULL ) {
        /* Error */
        return true;
    }

    /*
     *  If the open succeeded, this process has a controlling terminal.
     */
    fd = open( ctermid_name, O_RDWR | O_NOCTTY );
    if ( fd != -1 ) {
        close( fd );
        return true;
    }

    if ( (fd == -1) && (errno != ENXIO) ) {
        return true;
    }


    errno = 0; /*ENXIO*/
    return false;
}


static void 
release_controlling_terminal( void )
{
    int   fd;
    char  ctermid_name[ L_ctermid ];


    if ( ctermid(ctermid_name) == NULL ) {
        return;
    }

    /*
     *  If the open fails, nothing else can be done.
     */
    fd = open(ctermid_name, O_RDWR | O_NOCTTY);
    if ( fd == -1 ) {
        return;
    }

     /* TIOCNOTTY ioctl command against the terminal. */
    (void)ioctl( fd, TIOCNOTTY, 0 );
    (void)close( fd );

    return;
}


/*
 *  Attempt to make this process the session leader of a new session.
 *  If this succeeds, the process loses its controlling terminal.
 */
static bool 
create_session( bool started_by_inetd )
{
    pid_t sid, pid;
    /*
     *  The attempt may fail if the process is already a session
     *  leader.  
     */
    if ( setpgrp() == -1 ) {
        perror( "*** create_session:sepgrp" );
    }
    /*
     *  FIXME: this fails - setsid: operation not permitted SIGCONT
     *  Need to fork before but only if not started by inetd.  Already forked here !?
     */
    if ( setsid() == -1 ) {
        perror( "*** create_session:setsid" );
    }

    /*
     *  Make sure this process is a session leader.
     */
    /* gcc warning:  getsid ifdef __USE_XOPEN_EXTENDED in  unistd.h => -D_GNU_SOURCE */
    sid = getsid( (pid_t)0 ); 
    pid = getpid(); 
    if ( !(sid == pid) ) {
        fprintf( stderr, "\n*** Not a session leader: pid=%ld, sid=%ld \n", 
                (long)pid, (long)sid );
        /*FIXME: this fails - setsid operation not permitted */
        /*return false;*/
    }

    /*
     *  If the controlling terminal cannot be released, give up.
     *  (A process started by init may have opened a terminal,
     *  and acquired a controlling terminal).
     */
    if ( has_controlling_terminal() ) {
        release_controlling_terminal();

        if ( has_controlling_terminal() ) {
            return false;
        }
    }

    /*
     *  This process is the daemon.
     *  - it is the only process in its session and process group.
     *    (see previous FIXME)
     *  - its session has no controlling terminal.
     */
    errno = 0; 
    return true;
}


static bool
first_instance( const char *prog )
{
    return cfgs_get_excl( prog, 'D' ); 
}


void
install_signal_handlers( cfgs_daemon *d )
{
    int   i; 
    struct sigaction sa;
    
    memset( &sa, 0, sizeof(struct sigaction) );
    sa.sa_flags = SA_RESTART;
    sigfillset( &sa.sa_mask );

    for ( i=0; i<_NSIG; i++ ) {
        if ( *d->sighandlers[i] != NULL ) {
            sa.sa_handler = d->sighandlers[ i ];
            if ( sigaction(i, &sa, NULL ) )
                exit( errno );
        }
    }
}


/*
 *  Signal dispatcher: thread dealing with signals.  See 'Writing reliable AIX 
 *  daemons' by Eric Agar.  Very robust!
 */
static void *
signals_handling( void * arg )
{
    sigset_t         mask;
    int              signo;
    sighandler_func  *sfunc;


    LOG( cfgs_log(CFGST_LL_INFO, "signals_handling thread %ld \n", pthread_self()); );
    
    /*
    (void)sigemptyset( &mask );
    for ( i=0; i<_NSIG; i++ ) {
        if ( *m_daemon->sighandlers[i] != NULL ) {
            (void)sigaddset( &mask, i ); 
        }
    }
    */
    (void)sigfillset( &mask );
    (void)pthread_sigmask( SIG_BLOCK, &mask, NULL ); 

    while ( true ) {
        (void)sigwait( &mask, &signo );
        LOG( cfgs_log(CFGST_LL_INFO, "Received signal %d \n", signo); );
        lassert( signo >=0 && signo <= _NSIG );

        sfunc = m_daemon->sighandlers[ signo ]; 
        if ( sfunc && sfunc != SIG_IGN && sfunc != SIG_DFL && sfunc != SIG_ERR ) {
            (*sfunc)( signo );
        }
    }

    return NULL;
}


/* 
 * All signals delivered to the daemon will be handled 
 * by a specialised thread.  
 */ 
static bool
start_signals_handling_thread( cfgs_daemon *d )
{
    sigset_t   mask;
    pthread_t  signal_thread;
    int        rc;
    int        i; 
    
    m_daemon = d; /* for signals_handling */

    /*
    (void)sigemptyset( &mask );
    for ( i=0; i<_NSIG; i++ ) {
        if ( *d->sighandlers[i] != NULL ) {
            (void)sigaddset( &mask, i ); 
        }
    }
    */
    
    /* This is the main execution thread */
    (void)sigfillset( &mask );
    for ( i=0; i<_NSIG; i++ ) {
        if (  *m_daemon->sighandlers[i] == NULL 
           || *m_daemon->sighandlers[i] == SIG_DFL ) {
            (void)sigdelset( &mask, i ); 
        }
    }
    (void)pthread_sigmask( SIG_BLOCK, &mask, NULL ); 
    
    /* Useless, signals are blocked by mask */
    install_signal_handlers( m_daemon );
    
    /*FIXME: allow SIGTERM */
    
    
    rc = pthread_create( &signal_thread, NULL, signals_handling, NULL );
    if ( rc != 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "pthread_create() error: %d.\n", rc); ); 
        return false;
    }

    return true;
}


void
install_signal_thread( cfgs_daemon *d )
{
    if ( !start_signals_handling_thread(d) ) {
        exit( EXIT_FAILURE );
    }
}


void
install_sigpipe( cfgs_daemon *d )
{
}


int 
cfgs_run_daemon( cfgs_daemon *d )
{
    bool  started_by_inetd = is_parent_inetd();

    if ( d->single_instance && !first_instance(d->argv[0]) ) {
        fprintf( stderr, "Daemon [%s] alaredy started, only one instance allowed \n", 
                    d->argv[0] );
        return EXIT_FAILURE;
    }

    /*
     * Stevens - Unix Network Programming:
     * If we were started by init (process 1) from the /etc/inittab file
     * there's no need to detach.
     * This test is unreliable due to an unavoidable ambiguity
     * if the process is started by some other process and orphaned
     * (i.e., if the parent process terminates before we are started).
       */
    if ( getppid() == 1 )
          goto out;

    /* user can override any of those handlers */
    if ( !ignore_signals() )
        return EXIT_FAILURE; 

    if ( !started_by_inetd ) {
        pid_t pid = fork();
        if ( pid < 0 ) {
            perror( "Can't do first fork" );
            exit( errno );
        } else if ( pid > 0 ) {
            exit( EXIT_SUCCESS );
        }
    
        fprintf( stderr, "\nDaemon [%s] will run with pid %ld \n\n", d->argv[0], 
                    (long)getpid() );
        /* child continues */
    }
   
    /*
     *  Disasociate frm control terminal and process group and make sure
     *  process cannot reaquire a terminal.  
     */
    if ( !create_session(started_by_inetd) ) {
        exit( errno );
    }


out:

    /*
     * Might stop the daemon short if log file already open.  Better close
     * only first three file descriptors ?  
     */
    if ( !close_files(started_by_inetd) )
        exit( EXIT_FAILURE );
#if 0 /*FIXME: uncomment after tests*/
    if ( chdir("/") == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgs_run_daemon:chdir"); );
        exit( errno );
    }
#endif    
    umask( 0 );


    /*  
     *   Re-set signal handling as wanted by user.  
     */
    switch ( d->install_handlers ) {
    case CFGS_DH_SET_HANDLERS:
        install_signal_handlers( d );
        break;
    case CFGS_DH_THREAD:
        install_signal_thread( d );
        break;
    case CFGS_DH_SIGPIPE:
        install_sigpipe( d );
        break;
    default:
        lassert( false );
        break;
    }


    openlog( "cfgs_run_daemon", LOG_PID | LOG_CONS, LOG_DAEMON );
    syslog( LOG_NOTICE, "[%s] initializing, version %s\n", d->argv[0], VERSION ); 

    return (*d->func)( d->argc, d->argv, d->envp );
}


/*------------------------------------------------------------------*/
/* FIXME: use _exit but does not calls atexit */
#define SIGPIPE_ASSERT( cond )  \
    { \
        if ( !(cond) ) { \
            LOG(cfgs_log(CFGST_LL_CRITIC, \
                "Sigpipe failed assertion (%s/%d): "#cond"\n", \
                (char*)__FILE__, __LINE__););  \
            _exit(255); \
        } \
    }


static sigset_t m_sigpipe_sigs;
static sigset_t m_sigpipe_caught_sigs;
static int      m_sigpipe_caught_cnt;
static int      m_sigpipe_fds[ 2 ] = {-1, -1};


static int 
sigpipe_nonbock( int pipe_fd )
{
    int file_flags;


    if ( (file_flags = fcntl(pipe_fd, F_GETFL, 0)) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_nonbock: fcntl(F_GETFL) \n"); );
        return -1;
    }

    if ( fcntl(pipe_fd, F_SETFL, (file_flags | O_NONBLOCK)) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_nonbock: fcntl(F_SETFL) \n"); );
        return -1;
    }

    return 0;
}


static int 
sigpipe_cleanup( void )
{
    int rc = 0;
    int sig;
    int sig_member;
    struct sigaction sa;

    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    for ( sig = 1; sig <= _NSIG; sig++ ) {
        sig_member = sigismember( &m_sigpipe_sigs, sig );
        if ( sig_member == -1 ) {
            rc = -1;
            continue;
        }
        if ( sig_member ) {
            if ( sigaction(sig, &sa, NULL) == -1 ) {
                LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_cleanup: sigaction \n"); );
                rc = -1;
            }
        }
    }

    sigemptyset( &m_sigpipe_sigs );
    sigemptyset( &m_sigpipe_caught_sigs );
    m_sigpipe_caught_cnt = 0;
 
    if ( close(m_sigpipe_fds[1]) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_cleanup: close(1) \n"); );
        rc = -1;
    }
    m_sigpipe_fds[1] = -1;

    if ( close(m_sigpipe_fds[0]) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_cleanup: close(0) \n"); );
        rc = -1;
    }
    m_sigpipe_fds[0] = -1;

    return rc;
}


static void 
sigpipe_catcher( int sig )
{
    int sig_caught;
    int rc;


    sig_caught = sigismember( &m_sigpipe_caught_sigs, sig );
    SIGPIPE_ASSERT( sig_caught >= 0 );
    
    if ( !sig_caught ) {
        rc = sigaddset( &m_sigpipe_caught_sigs, sig );
        SIGPIPE_ASSERT( rc == 0 );
    
        if ( m_sigpipe_caught_cnt++ == 0 ) {
            do {
                rc = write( m_sigpipe_fds[1], "S", 1 );
            } while ( rc == -1 && errno == EINTR );
            SIGPIPE_ASSERT (rc == 1 );
            errno = 0;
        }
    }

    return;
}


int 
cfgs_sigpipe_start( int *sigs, int num_sigs )
{
    int i;
    struct sigaction sa;


    if ( m_sigpipe_fds[0] >= 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_start: m_sigpipe_fds[0] >= 0 \n"); );
        return -1;
    }

    sigemptyset( &m_sigpipe_sigs );

    for ( i = 0; i < num_sigs; i++ ) {
        if ( sigaddset(&m_sigpipe_sigs, sigs[i]) == -1 ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_start: sigaddset(%d) \n", i); );
            (void)sigpipe_cleanup();
            return -1;
        }
    }

    sigemptyset( &m_sigpipe_caught_sigs );
    m_sigpipe_caught_cnt = 0;

    if ( pipe(m_sigpipe_fds) == -1 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_start: pipe \n"); );
        (void)sigpipe_cleanup();
        return -1;
    }

    if (  (sigpipe_nonbock(m_sigpipe_fds[0]) == -1) 
       || (sigpipe_nonbock(m_sigpipe_fds[1]) == -1)) {
        (void)sigpipe_cleanup();
        return -1;
    }

    sa.sa_handler = sigpipe_catcher;
    sa.sa_mask    = m_sigpipe_sigs;
    sa.sa_flags   = SA_RESTART;

    for ( i = 0; i < num_sigs; i++ ) {
        if (sigaction(sigs[i], &sa, NULL) == -1) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_start: sigaction(%d) \n", i); );
            (void)sigpipe_cleanup();
            return -1;
        }
    }

    return m_sigpipe_fds[0];
}


int 
cfgs_sigpipe_caught( int sig )
{
    int         sig_caught;
    sigset_t    saved_sigset;
    char        buf[20];  
    int         rc;


    if ( sig < 1 || sig > _NSIG ) {
        return 0;
    }
    
    rc = sigprocmask( SIG_BLOCK, &m_sigpipe_sigs, &saved_sigset );
    SIGPIPE_ASSERT( rc == 0 );
    
    sig_caught = sigismember( &m_sigpipe_caught_sigs, sig );
    SIGPIPE_ASSERT( sig_caught >= 0 );

    if ( sig_caught ) {
        rc = sigdelset( &m_sigpipe_caught_sigs, sig );
        SIGPIPE_ASSERT( rc == 0 );
        
        if ( --m_sigpipe_caught_cnt == 0 ) {
            do {
                rc = read( m_sigpipe_fds[0], &buf, sizeof(buf) );
            } while (rc > 0 || (rc == -1 && errno == EINTR) );
            SIGPIPE_ASSERT( rc == -1 && errno == EAGAIN );
            errno = 0;
        }
    }

    rc = sigprocmask( SIG_SETMASK, &saved_sigset, NULL );
    SIGPIPE_ASSERT( rc == 0 );

    return sig_caught;
}


int  
cfgs_sigpipe_stop( void )
{
    if ( m_sigpipe_fds[0] < 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "sigpipe_stop \n"); );
        return -1;
    }

    return sigpipe_cleanup();
}


