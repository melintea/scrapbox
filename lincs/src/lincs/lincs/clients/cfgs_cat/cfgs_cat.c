
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:19:23 $
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>


#define PROGNAME   "cfgs_cat"
const char progname[] = PROGNAME;


/*
 * Function to regenerate a /etc file into fd file descriptor.  
 * Returns number of written bytes or negative if error.  
 */
typedef cfgs_buf* file_func( const char *filename );

typedef struct _regenerated_file {
    const char *filename;
    file_func  *ffunc;
} regenerated_file;


static cfgs_buf* regenerate_dummy( const char *filename );
const regenerated_file g_regenerated_files[] = {
    { "/etc/dummy",            regenerate_dummy },
    { NULL, NULL },
};



static file_func *
find_regenerator( const char *filename, regenerated_file *files )
{
    regenerated_file *prf = files;

    lassert( files );

    if ( !filename )
        return NULL;
    
    while ( prf && prf->filename != NULL ) {
        if ( 0 == strcmp(prf->filename, filename) )
            return prf->ffunc;
        prf++;
    }
    
    return NULL;
}


static cfgs_buf *
get_regenerated_file( const char *filename )
{
    cfgs_buf     *buf   = NULL;
    file_func    *ffunc = find_regenerator( filename, 
                              (regenerated_file*)g_regenerated_files );

    /* FIXME: if not in the table, check if there is an executable named
       filename under CFGS_ROOT_DIR.  filename is absolute.  
     */    
    if ( !filename || !ffunc )
        return NULL;
    
    buf = (*ffunc)( filename );

    return buf; 
}


static cfgs_buf*
regenerate_dummy( const char *filename )
{
    const char content[] = 
"# \n"
"# Regenerated /etc/dummy by " PROGNAME " \n"
"# \n"
"\n"
"Some funny content \n"
"\n"
"# EOF\n"
;

    return cfgs_buf_new( content, strlen(content) );
}


static void usage( void )
{
    fprintf( stderr, 
"\n%s " VERSION "\n"
"Usage: %s <filename>\n"
"Ex: %s /etc/dummy \n"
"On success, will dump content on stdout and return 0, else -1\n" 
"\n", 
    progname, progname, progname );
}


static char g_filename[ FILENAME_MAX ] = {0};

 
int
main( int argc, char **argv, char **envp )
{
    cfgs_buf       *content;
    
    if ( argc != 2 ) {
        usage();
        return EXIT_FAILURE;
    }

    /* FIXME: check rigths */ 
    
    /* if no absolute path name */
    if ( *argv[1] != '/' ) {
        if ( !getcwd(g_filename, FILENAME_MAX) ) {
            perror( progname );
            exit( EXIT_FAILURE );
        }
        strcat( g_filename, "/" );
        strcat( g_filename, argv[1] );
    } else
        strcpy( g_filename, argv[1] );

    content = get_regenerated_file( g_filename );
    if ( !content )
        return EXIT_FAILURE;

    (void)cfgst_rwrite( 1, content->buf, content->used );
    cfgs_buf_free( content );
    
    return EXIT_SUCCESS;
}


