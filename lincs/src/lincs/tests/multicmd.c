
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:24:51 $
 *
 *  Test multiple commands in one xml message. 
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
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>

#include "cfgs_client_api.h"
#include "cfgs_dlist.h"
#include "cfgs_log.h"
#include "cfgs_tags.h"


#define PROGNAME   "multicmd"
const char progname[] = PROGNAME;



/*
 *  Keep those in sync.  
 */
#if CFGS_ET_VERSION != 1 
  error CFGS_ET enum (cfgs_val.h) has changed.  Please inspect structures below, including g_process_funcs. 
#endif

static cfgs_entry    *g_entries = NULL; 
static cfgs_session  *g_session = NULL;


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
"Usage: %s xml_file \n"
"\n", 
    progname );
}

 
static void
print_entry( cfgs_entry *e )
{
    cfgs_pair *p; 
    
    if ( !e ) 
        return;
    
    lassert( e->value_type == CFGS_VT_UCPTR );
    
    printf( "Entry: \n" );
    for ( p=e->attr; p; p=p->next ) {
        char *n, *v; 
    
        n = p->first;
        v = p->second;
        printf( "  '%s' = '%s' \n", SAFE(n), SAFE(v) );
    }
    printf( "\n" );
}


static void
print_entries( cfgs_entry *e )
{
    int        len = cfgs_dlist_length( (cfgs_dlist*)g_entries );
    cfgs_entry *val;
    
    for ( val=e;  val; val=val->next ) {
        print_entry( val );
    }
    
    printf( "Number of entries: %d\n", len );
}


static cfgs_entry *
read_entries( const char *filename )
{
    int    f;
    int    len  = 0;
    cfgs_entry *val = NULL;
    cfgs_tag   *tag = NULL;

    lassert( filename );
    if ( !filename ) {
        fprintf( stderr, "Error: read_entries NULL param\n" );
        return NULL;
    }
    
    f = open( filename, O_RDONLY );
    if ( f == -1 ) {
        perror( filename );
        return NULL;
    }

    tag = cfgs_tags_read( f );
    if ( !tag ) {
        return NULL;
    }
    
    val = cfgs_entries_from_tags( tag );
    if ( val ) {
        len = cfgs_dlist_length( (cfgs_dlist*)val );
        lassert( len > 0 );
    } 
    CFGST_DLIST_FREE( tag, cfgs_tag_free );
    
    close( f );
    
    return val;
}


void
exit_err( int status )
{
    perror( progname );
    exit( status );
}

void
print_cfgs_err( void )
{
    const cfgs_err *err;

    err = cfgs_geterror( g_session );
    if ( cfgs_iserr(err) ) {
        cfgs_perror( err, PROGNAME, stderr );
        exit_err( EXIT_FAILURE );
    }
}

int
main( int argc, char **argv, char **envp )
{
    int            ret = EXIT_FAILURE;
    int            len = 0;
    
    if ( argc != 2 ) {
        usage();
        return EXIT_FAILURE;
    }

    /* Parse xml file into cfgs_entry list */ 
    g_entries = read_entries( argv[1] );
    if ( !g_entries ) {
        exit_err( EXIT_FAILURE );
    }
    print_entries( g_entries );

        
    g_session = cfgs_connect();
    if ( !g_session ) {
        print_cfgs_err();
        exit_err( EXIT_FAILURE );
    }

    /* Send command.  Return should be the list size, i.e. len.  */
    len = cfgs_dlist_length( (cfgs_dlist*)g_entries );
    printf( "Sending a batch of %d commands... ", len );
    ret = cfgs_setval( g_session, g_entries );
    if ( ret != len ) {
        fprintf( stderr, "Error: len=%d != ret=%d \n\n", len, ret );
        print_cfgs_err();
        (void)cfgs_disconnect( g_session );
        exit_err( EXIT_FAILURE );
    }
    printf( "OK \n" );
    
    /*Re-send commands over KEEP-ALIVE connection.  Might die by SIGPIPE. */
    printf( "RE-Sending a batch of %d commands... ", len );
    ret = cfgs_setval( g_session, g_entries );
    if ( ret != len ) {
        fprintf( stderr, "Error: len=%d != ret=%d \n\n", len, ret );
        print_cfgs_err();
        (void)cfgs_disconnect( g_session );
        exit_err( EXIT_FAILURE );
    }
    printf( "OK \n" );
    
    print_cfgs_err();
    (void)cfgs_disconnect( g_session );
    /*
     * Allocated ressources freed when exit. 
     */ 
    return EXIT_SUCCESS;
}


