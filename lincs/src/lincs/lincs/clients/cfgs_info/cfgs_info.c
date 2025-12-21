
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
#include "cfgs_client_api.h"
#include "cfgs_dlist.h"
#include "cfgs_log.h"

#include <stdio.h>


#define PROGNAME   "cfgs_info"
const char progname[] = PROGNAME;



static void usage( void )
{
    fprintf( stderr, 
"\n%s " PACKAGE_NAME " " VERSION "\n"
"Usage: %s <name>\n\n", 
    progname, progname );
}

 
int
main( int argc, char **argv, char **envp )
{
    cfgs_session   *sess;
    cfgs_str       *bks, *bk;
    const cfgs_err *err;
    bool           iserr = false;
    
    /*
    signal( SIGHUP, SIG_IGN );
    signal( SIGINT, SIG_IGN );
    signal( SIGQUIT, SIG_IGN );
    signal( SIGPIPE, SIG_IGN );
    signal( SIGTERM, SIG_IGN );
    */

    usage();
        
    sess = cfgs_connect();
    if ( !sess ) {
        return EXIT_FAILURE;
    }

    /* FIXME: cfgs_get_backends, cs_get_regenerated_files */    
    bks = cfgs_get_backends();
    /*printf( "\n" );*/
    /*       123456789 123456789 123456789 123456789 123456789 123456789 */
    printf( "Backend       File \n" );
    printf( "-----------------------------------------------------------\n" );

    bk = bks;
    while ( bk ) {
        char  *bn;

        lassert( bk->name != NULL );
        bn = bk->name;
        printf( "'%s' \n", bn );
    
        bk = bk->next;
    }
    CFGST_DLIST_FREE( bks, cfgs_str_free );
    printf( "\n" );
    
    err   = cfgs_geterror( sess );
    iserr = cfgs_iserr( err );
    if ( iserr ) {
        fprintf( stderr, "Error: " PROGNAME ": %s\n", err->xml );
    }

    (void)cfgs_disconnect( sess );
    return iserr ? EXIT_FAILURE : EXIT_SUCCESS; 
}
 
 
