
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
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "cfgs_client_api.h"
#include "cfgs_dlist.h"
#include "cfgs_log.h"
#include "cfgs_tags.h"


#define PROGNAME   "cfgs_tool"
const char progname[] = PROGNAME;



/*
 *  Keep those in sync.  
 */
typedef enum {
    CSTOOL_CMD_NONE = -1,
    CSTOOL_CMD_SET,
    CSTOOL_CMD_GET,
    CSTOOL_CMD_REMOVE,
    /* add here */
    CSTOOL_CMD_HELP,
    CSTOOL_CMD_VERSION,
    CSTOOL_CMD_QUIET,
} CSTOOL_CMD;

static struct option g_long_option[] =
{
    {"set",     1, NULL, CSTOOL_CMD_SET},
    {"get",     1, NULL, CSTOOL_CMD_GET},
    {"remove",  1, NULL, CSTOOL_CMD_REMOVE},
    /* add here if a processing function is needed - see g_process_funcs */
    {"help",    0, NULL, CSTOOL_CMD_HELP},
    {"version", 0, NULL, CSTOOL_CMD_VERSION},
    {"quiet",   0, NULL, CSTOOL_CMD_QUIET},
    {NULL,      0, NULL, 0},
};
const char g_short_option[] = "s:g:r:hvq";

/*#define MAX_CMD (sizeof(g_long_option)/sizeof(struct option)) */
/* Commands that will be sent to CS system. */
#define MAX_CMD (3)


/*
 *  Keep those in sync.  
 */
#if CFGS_ET_VERSION != 1 
  error CFGS_ET enum (cfgs_val.h) has changed.  Please inspect structures below, including g_process_funcs. 
#endif

typedef struct _entry {
    CFGS_ET      code;
    const char   *name;
} entry;

#define X(a,b)  {a, b},
static const entry g_entries[] = {
    CFGS_ENTRY_TYPES
    {CFGS_ET_NONE, NULL },
};
#undef X


#define MAX_ENTRY ((sizeof(g_entries)/sizeof(entry)) - 1)


typedef int process_func( void );

static int set_value( void );
static int get_value( void );
static int remove_value( void );
static int set_scheme( void );
static int get_scheme( void );
static int remove_scheme( void );
static int set_layer( void );
static int get_layer( void );
static int remove_layer( void );

static process_func *g_process_funcs[MAX_ENTRY][MAX_CMD] = {
    {set_value,  get_value,  remove_value,  }, /*value*/
    {set_scheme, get_scheme, remove_scheme, }, /*scheme*/
    {set_layer,  get_layer,  remove_layer,  }, /*layer*/
};


static CSTOOL_CMD    g_command    = CSTOOL_CMD_NONE;
static CFGS_ET       g_entry_type = CFGS_ET_NONE;
static cfgs_entry    g_entry; 
static cfgs_session  *g_session = NULL;
static bool          g_quiet = false; /* If true do not print on terminal */


static CSTOOL_CMD
get_entry_type( const char *e )
{
    const entry *pc = g_entries;

    if ( !e )
        return CFGS_ET_NONE;
    
    while ( pc->name ) {
        if ( 0 == strcmp(pc->name, e) )
            return pc->code;
        pc++;
    }
    
    return CFGS_ET_NONE;
}


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
"Usage: %s <command> <entry> [params]\n"
"    command: --help | --version | --quiet \n"
"             --set | -s | --get | -g | --remove | -r\n"
"    entry:   value | scheme | layer \n"
"    params:  'name1=value1' 'name2=value2' etc. Quotes are mandatory. \n"
"You can use glob(3) regular expressions as name\n" 
"\n" 
"Ex: %s --set value 'name=/network/eth0/ip' 'value=123.123.123.123' \n"
"\n", 
    progname, progname );
}

 
static int 
set_value( void )
{
    int        ret;
    
    if ( !g_quiet )
        printf( "'%s' = '%s' ... ", 
                SAFE( cfgs_entry_attr(&g_entry, CFGS_EA_NAME) ), 
                SAFE( cfgs_entry_attr(&g_entry, CFGS_EA_VALUE) )
                );
    g_entry.entry_type = CFGS_ET_VALUE; 
    ret = cfgs_setval( g_session, &g_entry );
    if ( ret > 0 ) {
        if ( !g_quiet ) printf( "ok\n" );
        return EXIT_SUCCESS;
    } else {
        if ( !g_quiet ) printf( "!!! ERROR: %d !!!\n", ret );
        return EXIT_FAILURE;
    }
}


static void
print_entry( cfgs_entry *e )
{
    cfgs_pair *p; 
    
    if ( g_quiet )
        return;
    
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


/* FIXME: get/rm should ret <0 on error */
static int 
get_value( void )
{
    char   *vname = cfgs_entry_attr( &g_entry, CFGS_EA_NAME );
    char   *layer = cfgs_entry_attr( &g_entry, CFGS_EA_LAYER );
    cfgs_entry *val, *head;

    /* might happen, when out of memory, because cfgs_val attributes are dynamically
       allocated */    
    if ( !vname ) {
        return EXIT_FAILURE;
    }

    head = cfgs_getval( g_session, vname, layer );
    
    for ( val=head;  val; val=val->next ) {
        print_entry( val );
    }
    CFGST_DLIST_FREE( head, cfgs_entry_free );
    
    return EXIT_SUCCESS;
}


static int 
remove_value( void )
{
    char *vname = cfgs_entry_attr( &g_entry, CFGS_EA_NAME );
    char *layer = cfgs_entry_attr( &g_entry, CFGS_EA_LAYER );
    int  ret;
    
    if ( !g_quiet ) printf( "Deleting '%s' from %s ... ", SAFE(vname), SAFE(layer) );
    ret = cfgs_rmval( g_session, vname, layer );
    /* ret might be 0 if no value found */
    /* FIXME: see if 0 should be considered an error - ex: rm */
    if ( ret >= 0 ) {
        if ( !g_quiet ) printf( "ok\n" );
        return EXIT_SUCCESS;
    } else {
        if ( !g_quiet ) printf( "!!! ERROR: %d !!!\n", ret );
        return EXIT_FAILURE;
    }
}


static int 
set_scheme( void )
{
    printf( "set_scheme: not implemented\n" );
    return EXIT_SUCCESS;
}


static int 
get_scheme( void )
{
    printf( "get_scheme: not implemented\n" );
    return EXIT_SUCCESS;
}


static int 
remove_scheme( void )
{
    printf( "remove_scheme: not implemented\n" );
    return EXIT_SUCCESS;
}


static int 
set_layer( void )
{
    printf( "set_layer: not implemented\n" );
    return EXIT_SUCCESS;
}


static int 
get_layer( void )
{
    printf( "get_layer: not implemented\n" );
    return EXIT_SUCCESS;
}


static int 
remove_layer( void )
{
    printf( "remove_layer: not implemented\n" );
    return EXIT_SUCCESS;
}


void
exit_err( int status )
{
    perror( progname );
    exit( status );
}

int
main( int argc, char **argv, char **envp )
{
    const cfgs_err *err;
    int optidx;
    int morehelp;
    int numparams = 0;
    int i;
    int ret;
    
    if ( argc < 2 ) {
        usage();
        return EXIT_FAILURE;
    }
    
    morehelp = 0;
    while ( true ) {
        int c;

        if ( (c = getopt_long(argc, argv, g_short_option, g_long_option, &optidx)) < 0 ) 
            break;
            
        switch (c) {
        case 'h':
        case CSTOOL_CMD_HELP:
            usage();
            return EXIT_SUCCESS;
        case 'v':
        case CSTOOL_CMD_VERSION:
            version();
            return EXIT_SUCCESS;
        case 'q':
        case CSTOOL_CMD_QUIET:
            g_quiet = true;
            break;

        case 's':
        case CSTOOL_CMD_SET:
            g_command    = CSTOOL_CMD_SET;
            g_entry_type = get_entry_type( optarg );
            break;
        case 'g':
        case CSTOOL_CMD_GET:
            g_command    = CSTOOL_CMD_GET;
            g_entry_type = get_entry_type( optarg );
            break;
        case 'r':
        case CSTOOL_CMD_REMOVE:
            g_command    = CSTOOL_CMD_REMOVE;
            g_entry_type = get_entry_type( optarg );
            break;

        default:
            fprintf(stderr, "\07Invalid switch or option needs an argument.\n");
            morehelp++;
        }
    } /*while*/

    if ( morehelp ) {
        usage();
        return EXIT_FAILURE;
    }

    numparams = argc - optind;
    if ( numparams <= 0 ) {
        fprintf( stderr, "%s: Must specify parameters!\n", progname );
        return EXIT_FAILURE;
    } 
    
    if ( g_command == CSTOOL_CMD_NONE ) {
        fprintf( stderr, "%s: Invalid comand!\n", progname );
        return EXIT_FAILURE;
    }
    if ( g_entry_type == CFGS_ET_NONE ) {
        fprintf( stderr, "%s: Invalid entry type!\n", progname );
        return EXIT_FAILURE;
    }
    
    lassert( g_entry_type >=0 && g_entry_type <= MAX_ENTRY );
    lassert( g_command >=0 && g_command <= MAX_CMD );
    lassert( argc >= 3 );

    /* process params */
    for ( i=3; i<argc; i++ ) {
        char *n, *v;
    
        n = argv[i];
        v = strchr( n, '=' );
        if ( v )
            *v = '\0', v++;
        /* FIXME: there is a failure path with cfgs_entry_add_attr that rets true */
        if ( !cfgs_entry_add_attr(&g_entry, n, v) ) {
            exit_err( EXIT_FAILURE );
        }
    }
    g_entry.value_type = CFGS_VT_UCPTR;
    
    
    g_session = cfgs_connect();
    if ( !g_session ) {
        exit_err( EXIT_FAILURE );
    }

    
    lassert( g_process_funcs[g_entry_type][g_command] );
    if ( g_process_funcs[g_entry_type][g_command] )
        ret = (*g_process_funcs[g_entry_type][g_command])();
    
    err = cfgs_geterror( g_session );
    if ( cfgs_iserr(err) ) {
        cfgs_perror( err, PROGNAME, stderr );
    }
    
    (void)cfgs_disconnect( g_session );
    return ret;
}

