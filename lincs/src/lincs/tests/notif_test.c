
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:24:51 $
 *
 *  Test the notification mechanism:
 *    -set handler for SIGUSR1
 *    -register to be notified with SIGUSR1 on '*' value names
 *    -set value /tests/notif_test
 *    -at this point notification should arrive: return 0
 *    -await SIGUSR1 for 10 seconds and exit with -1 if not signaled 
 */
/*
# 
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "cfgs_client_api.h"
#include "cfgs_log.h"
#include "cfgs_tags.h"


#define PROGNAME   "notif_test"
const char progname[] = PROGNAME;

#define VALNAME_REG  "*"
#define VALNAME_SET  "/tests/"PROGNAME

bool g_sigusr1_delivered = false;


static void 
version( void )
{
    fprintf( stderr, 
"%s " VERSION "\n",
    progname );
};

static void 
usage( void )
{
    version();
    fprintf( stderr, 
"\nNotification test program:\n"
"" );
}

 
void
exit_err( int status )
{
    perror( progname );
    exit( status );
}


void
sigusr1_handler( int signo )
{
    int   old_errno = errno; 
    
    printf( "  Received signal %d \n", signo );
    g_sigusr1_delivered = true; 
    
    errno = old_errno;
}


static bool
set_sigusr1_handler( void )
{
    struct sigaction sa;
    
    sa.sa_handler = sigusr1_handler;
    sigemptyset( &(sa.sa_mask) );
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if ( sigaction(SIGUSR1, &sa, NULL) ) 
        return false;

    printf( "  [%ld] sigusr1_handler was set \n", (long)getpid() );
    return true; 
}


static int
register_notif( void )
{
    const cfgs_err *err;
    int            ret;
    cfgs_notif     notif = {0}; 
    cfgs_session   *session = NULL;

    notif.pid     = getpid(); 
    notif.signal  = SIGUSR1;
    notif.type    = CSNT_LOCAL;
    notif.valname = VALNAME_REG; 

    session = cfgs_connect();
    if ( !session ) {
        exit_err( EXIT_FAILURE );
    }

    printf( "  Registering for '" VALNAME_REG "'... " );

    ret = cfgs_register_notif( session, &notif );
    if ( ret > 0 ) {
        printf( "ok\n" );
    } else {
        printf( "!!! ERROR: %d !!!\n", ret );
    }
    
    err = cfgs_geterror( session );
    if ( cfgs_iserr(err) ) {
        cfgs_perror( err, PROGNAME, stderr );
    }
    
    (void)cfgs_disconnect( session );
    return ret;
}

static int
set_value( void )
{
    const cfgs_err *err;
    int            ret;
    cfgs_entry     entry = {0}; 
    cfgs_session   *session = NULL;
    

    if ( !cfgs_entry_add_attr(&entry, CFGS_EA_NAME, VALNAME_SET) )
        exit_err( EXIT_FAILURE );
    if ( !cfgs_entry_add_attr(&entry, CFGS_EA_VALUE, VALNAME_SET) )
        exit_err( EXIT_FAILURE );
    entry.entry_type = CFGS_ET_VALUE; 
    
    session = cfgs_connect();
    if ( !session ) {
        exit_err( EXIT_FAILURE );
    }

    printf( "  Setting " VALNAME_SET "... \n" );

    ret = cfgs_setval( session, &entry );

    printf( "  cfgs_setval(" VALNAME_SET ") should return 1: %d\n", ret );
    
    err = cfgs_geterror( session );
    if ( cfgs_iserr(err) ) {
        cfgs_perror( err, PROGNAME " - set_value", stderr );
    }
    
    (void)cfgs_disconnect( session );
    return ret;
}

int
main( int argc, char **argv, char **envp )
{
    usage();
    
    if ( !set_sigusr1_handler() ) {
        return EXIT_FAILURE;
    }
    
    if ( 1 != register_notif() ) {
        return EXIT_FAILURE;
    }

    if ( 1 != set_value() ) {
        return EXIT_FAILURE;
    }
    
    if ( !g_sigusr1_delivered ) {
        sleep( 10 );
        if ( !g_sigusr1_delivered ) {
            return EXIT_FAILURE;
        }
    }

    g_sigusr1_delivered = false;
    printf( "  Calling set_value() a second time... \n" );
    if ( 1 != set_value() ) {
        return EXIT_FAILURE;
    }
    
    if ( !g_sigusr1_delivered ) {
        sleep( 10 );
        if ( !g_sigusr1_delivered ) {
            return EXIT_FAILURE;
        }
    }
    
    g_sigusr1_delivered = false;
    //FIXME: unregister notif & set_value; flag should be false
    
    return EXIT_SUCCESS;
}

