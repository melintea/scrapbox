
/*
 *  $Revision: 1.11 $
 *  $Date: 2004/03/31 21:20:41 $
 *
 *  Client API
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
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>


#include "cfgs_backend.h"
#include "cfgs_log.h"
#include "cfgs_mem.h"
#include "cfgs_dlist.h"
#include "cfgs_client_api.h"
#include "cfgs_str.h"
#include "fs.h"


#define CFGS_BACKEND_NAME  "cfgs_fs_bk"

const char g_progname[] = CFGS_BACKEND_NAME;


/* 
 * exported functions:
 */
#define on_unload                 cfgs_fs_bk ## _LTX_on_unload
#define on_load                   cfgs_fs_bk ## _LTX_on_load

#if CFGS_CRT_REV != 4
#  error please update defines to CFGS_CRT_REV if needed
#endif
#define cfgs_getval                 cfgs_fs_bk ## _LTX_cfgs_getval
#define cfgs_setval                 cfgs_fs_bk ## _LTX_cfgs_setval
#define cfgs_rmval                  cfgs_fs_bk ## _LTX_cfgs_rmval
#define cfgs_getsubvals             cfgs_fs_bk ## _LTX_cfgs_getsubvals
#define cfgs_getsublayers           cfgs_fs_bk ## _LTX_cfgs_getsublayers
#define cfgs_getinfos               cfgs_fs_bk ## _LTX_cfgs_getinfos



/* Return -1 on error */
int 
on_match_get( match_data *data )
{
    int    f;
    int    ret  = 0;
    cfgs_entry *val = NULL;
    cfgs_tag   *tag = NULL;

    lassert( data && data->filename );
    if ( !data || !data->filename )
        return -1;
    
    LOG( cfgs_log(CFGST_LL_INFO, "OnMatchGet '%s' -> '%s'!\n", data->valname, data->filename); );

    /*FIXME: replace open with restartable func*/
    f = open( data->filename, O_RDONLY );
    lassert( f >= 0 );
    if ( f == -1 )
        return -1;

    tag = cfgs_tags_read( f );
    if ( !tag )
        return -1;
    
    val = cfgs_entries_from_tags( tag );
    if ( val ) {
        ret = cfgs_dlist_length( (cfgs_dlist*)val );
        lassert( ret > 0 );
        *data->vlist = (cfgs_entry*)cfgs_dlist_add_tail( (cfgs_dlist*)*data->vlist, 
                               (cfgs_dlist*)val );
    } else
        ret = 0;
    CFGST_DLIST_FREE( tag, cfgs_tag_free );
    
    close( f );
    return ret;
}


int 
on_match_set( match_data *data )
{
    int    f;
    int    ret = 0;
    char   *name; 

    lassert( data && data->filename && data->value );
    if ( !data || !data->filename || !data->value )
        return -1;

    name = cfgs_entry_attr( data->value, CFGS_EA_NAME ); 
    lassert( name && *name );
    if ( !name )
        return -1;
    LOG( cfgs_log(CFGST_LL_INFO, "OnMatchSet '%s' -> '%s'!\n", name, data->filename); );

    f = open( data->filename, O_TRUNC|O_WRONLY|O_SYNC/*|O_NOFOLLOW*/ );
    lassert( f >= 0 );
    if ( f == -1 )
        return -1;

    lassert( data->value != NULL && data->value->value_type == CFGS_VT_UCPTR );
    if ( data && data->value ) {
        cfgs_tag  *tag = cs_tags_from_entries( data->value );
        if ( tag ) {
            ret = cfgs_dlist_length( (cfgs_dlist*)tag );
            lassert( ret > 0 );
            cfgs_tags_write( f, tag );
            CFGST_DLIST_FREE( tag, cfgs_tag_free );
        } else
            ret = 0;
    }
    
    if ( 0 != close(f) )
        return -1; 
    return ret;
}


int 
on_match_rm( match_data *data )
{
    lassert( data && data->filename );
    if ( !data || !data->filename )
        return -1;
    
    LOG( cfgs_log(CFGST_LL_INFO, "OnMatchSet '%s' -> '%s'!\n", data->valname, data->filename); );

    if ( 0 == unlink(data->filename) )
        return 1;

    return 0;
}


static const char*
get_entry_dir( CFGS_ET entry_type )
{
#if CFGS_ET_VERSION != 1 
  error CFGS_ET enum (cfgs_val.h) has changed.  Please inspect structures below. 
#endif

    const char *ret = NULL;

    switch ( entry_type ) {
    case CFGS_ET_VALUE:  ret = "values";  break;
    case CFGS_ET_SCHEME: ret = "schemes"; break;
    case CFGS_ET_LAYER:  ret = "layers";  break;
    default:
        /* programming error: invalid CFGS_ET */
        lassert( false );
        break;
    }
    
    return ret;
}


/* name length limit */
static inline bool
check_dir_length( const char *name, const char *entry_dir )
{
    if ( !name || !entry_dir ) 
        return false;
    
    lassert( strlen(CFGS_VALUES_ROOT_DIR)                             
           + 1 + strlen(CFGS_BACKEND_NAME) + 1 + strlen(name)         
           + strlen(entry_dir)                                        
           < FILENAME_MAX );                                          
    if ( strlen(CFGS_VALUES_ROOT_DIR)                                 
       + 1 + strlen(CFGS_BACKEND_NAME) + 1 + strlen(name) + strlen(entry_dir)
       > FILENAME_MAX ) {                                        
        return false;                                                 
    }
    
    return true;
}


static bool
make_root_dir( char *root_dir, const char *layer, const char *entry_type )
{
    if ( !root_dir || !layer || !entry_type ) 
        return false;
    
    snprintf( root_dir, FILENAME_MAX-1, "%s%s%s%s%s%s%s",                  
            CFGS_VALUES_ROOT_DIR, FS_PATH_SEP_S, layer,   
            FS_PATH_SEP_S, CFGS_BACKEND_NAME,             
            FS_PATH_SEP_S, entry_type                     
            );
    return true;
}


cfgs_entry*
cs_getentry( cfgs_session *sess, 
             const char   *name, CFGS_ET entry_type,
             const char   *layer )
{
    int        nvals = 0;
    cfgs_entry *pval = NULL;
    match_data data  = {0};
    char       root_dir[ FILENAME_MAX ];
    const char *l = layer != NULL ? layer : CFGS_DEFAULT_LAYER;
    const char *entry_dir = get_entry_dir( entry_type );
    
    if ( !entry_dir || !name || !sess ) 
        return NULL;

    /* name length limit */
    if ( !check_dir_length(name, entry_dir) ) {
       /* FIXME: report error */                                      
        return NULL;
    }
    
    lassert( name != NULL );
    if ( !name || does_not_exists(name) ) 
        return NULL;

    if ( !((pval=is_cached(name)) != NULL) ) {
        /* FIXME: if layer == NULL, for every layer under CFGS_VALUES_ROOT_DIR? */
        make_root_dir( root_dir, l, entry_dir );
        data.valname = (char*)name;  
        data.vlist   = &pval; 
TEST_ERROR         
        nvals = fs_search( root_dir, name, false, on_match_get, &data );
        if ( nvals < 0 ) { 
            if ( pval ) {
                cfgs_entry_free( pval );
                pval = NULL;
            }
            /*FIXME: store error*/
            return NULL;
        }
TEST_ERROR        
    }

    return pval; 
}


cfgs_entry*
cfgs_getval( cfgs_session *sess, const char *name, const char *layer )
{
    if ( !sess || !name )
        return NULL;
        
    return cs_getentry( sess, name, CFGS_ET_VALUE, layer );
}

#if 0
//FIXME
cfgs_entry *
cfgs_getlayer( cfgs_session *s, const char *name )
{
    if ( !s || !name )
        return NULL;
        
    return cs_getentry( s, name, CFGS_ET_LAYER, "" );
}

/*
 *  If valname is not null, get the scheme for valname, otherwise use
 *  scheme to look for the proper cfgs_scheme.  
 */
cfgs_entry*
cfgs_getscheme( cfgs_session *ss, const char *valname, const char *scheme )
{
    if ( !ss || !name )
        return NULL;
        
    return cs_getentry( ss, name, CFGS_ET_SCHEME, layer );
}
#endif


int 
cs_setentry( cfgs_session *sess, cfgs_entry *vl, CFGS_ET entry_type )
{
    int        nvals = 0;
    cfgs_entry *crt  = vl;
    const char *entry_dir = get_entry_dir( entry_type );
    
    if ( !entry_dir || !sess || !vl ) {
        return -1;
    }

    for ( ; crt; crt=crt->next ) {
        const char *name, *layer; 
        char       root_dir[ FILENAME_MAX ];
        match_data data = {0};
        name = cfgs_entry_attr( crt, CFGS_EA_NAME );
        if ( !name || !*name )
            continue; 

        layer = cfgs_entry_attr( crt, CFGS_EA_LAYER );
        if ( !layer ) {
            cfgs_entry_add_attr( crt, CFGS_EA_LAYER, CFGS_DEFAULT_LAYER );
            layer = cfgs_entry_attr( crt, CFGS_EA_LAYER );
            if ( !layer ) {
                return -1; 
            }
        }
        
        /* regexps allowed only for get */
        if ( is_regexp(name) || is_regexp(layer) )
            continue;

        /* name length limit */
        if ( !check_dir_length(name, entry_dir) ) {
           /* FIXME: report error */                                      
            continue;
        }
TEST_ERROR    
        make_root_dir( root_dir, layer, entry_dir );
TEST_ERROR        
        data.value = vl;
        nvals += fs_search( root_dir, name, true, on_match_set, &data );
TEST_ERROR        
    }
   
    return nvals; 
}


int 
cfgs_setval( cfgs_session *sess, cfgs_entry *vl )
{
    if ( !sess || !vl )
        return -1;
        
    return cs_setentry( sess, vl, CFGS_ET_VALUE );
}

#if 0
//FIXME
int 
cfgs_setlayer( cfgs_session *sess, cfgs_entry *vl )
{
    if ( !sess || !vl )
        return -1;
        
    return cs_setentry( sess, vl, CFGS_ET_LAYER );
}

int 
cfgs_setscheme( cfgs_session *sess, cfgs_entry *vl )
{
    if ( !sess || !vl )
        return -1;
        
    return cs_setentry( sess, vl, CFGS_ET_SCHEME );
}
#endif


int 
cs_rmentry( cfgs_session *sess, const char *name, 
            const char *layer,  CFGS_ET entry_type )
{
    int        nvals = 0;
    match_data data  = {0};
    char       root_dir[ FILENAME_MAX ];
    const char *l = layer != NULL ? layer : CFGS_DEFAULT_LAYER;
    const char *entry_dir = get_entry_dir( entry_type );
    
    if ( !entry_dir || !name || !sess ) {
        return -1;
    }
    

    /* regexps allowed only for get */
    if ( is_regexp(name) || is_regexp(l) )
        return 0; /*FIXME: -1 ?*/

    /* name length limit */
    if ( !check_dir_length(name, entry_dir) ) {
       /* FIXME: report error */                                      
        return -1;
    }

    /* FIXME: for every layer under CFGS_VALUES_ROOT_DIR */
    make_root_dir( root_dir, l, entry_dir );
    data.valname = (char*)name;
    nvals += fs_search( root_dir, name, false, on_match_rm, &data );
    if ( nvals < 0 ) {
        /*FIXME: report error */
    }
   
    return nvals; 
}


int 
cfgs_rmval( cfgs_session *sess, const char *name, const char *layer )
{
    if ( !sess || !name )
        return -1;
        
    return cs_rmentry( sess, name, layer, CFGS_ET_VALUE );
}

#if 0
//FIXME
int 
cfgs_rmscheme( cfgs_session *sess, const char *name, const char *layer )
{
    if ( !sess || !name )
        return -1;
        
    return cs_rmentry( sess, name, layer, CFGS_ET_SCHEME );
}

int 
cfgs_rmlayer( cfgs_session *sess, const char *name )
{
    if ( !sess || !name )
        return -1;
        
    return cs_rmentry( sess, name, "", CFGS_ET_LAYER );
}
#endif


int 
cfgs_register_notif( cfgs_session *s, cfgs_notif *notif )
{
    /* Not used.  Notifications are the responsability of the server.  */
    lassert( false );
    return 0; 
}


bool
on_load( void )
{
    LOG( cfgs_log(CFGST_LL_INFO, "backend '%s' successfully loaded\n", g_progname); );
    return true;
}


bool
on_unload( void )
{
    LOG( cfgs_log(CFGST_LL_INFO, "backend '%s' successfully unloaded \n", g_progname); );
    return true;
}


cfgs_str *
cfgs_getsubvals( cfgs_session *sess, const char *valname, const char *layer )
{
    cfgs_str   *pval = NULL;
    char       root_dir[ FILENAME_MAX ];
    const char *l = layer != NULL ? layer : CFGS_DEFAULT_LAYER;
    const char *entry_dir = get_entry_dir( CFGS_ET_VALUE );
    DIR        *dirp = NULL;
    struct dirent *dire = NULL;
    int        old_errno;
    
    lassert( valname != NULL );
    if ( !entry_dir || !valname || !sess ) 
        return NULL;

    /* name length limit */
    if ( !check_dir_length(valname, entry_dir) ) {
       /* FIXME: report error */                                      
        return NULL;
    }
    
    make_root_dir( root_dir, l, entry_dir );
    strcat( root_dir, valname ); /*lenght has been verified*/
    if ( !dir_on_disk(root_dir) )
        return NULL;
	
    dirp = opendir( root_dir );
    if ( !dirp ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgs_getsubvals::opendir \n"); );
	    return NULL;
    }
    
TEST_ERROR;
    old_errno = errno;
    /*FIXME: overwritten by next call.  Thr. safe: readdir_r*/
    for ( dire = readdir(dirp); dire; dire = readdir(dirp) ) {
        cfgs_str *s;
        if ( 0 == strcmp(".", dire->d_name) || 0 == strcmp("..", dire->d_name) )
	        continue;
		/*FIXME: filter out files*/
	    s = cfgs_str_new( dire->d_name );
	    if ( !s )
	        break;
		pval = (cfgs_str*)cfgs_dlist_add_tail( (cfgs_dlist*)pval, (cfgs_dlist*)s );
    }
    if ( errno == 0 ) 
        errno = old_errno; 
    closedir( dirp );

    return pval; 
}


cfgs_str *
cfgs_getsublayers( cfgs_session *s, const char *layername )
{
    return NULL; //FIXME
}


cfgs_str *
cfgs_getinfos( cfgs_session *s )
{
    cfgs_str *ret = cfgs_str_new( 
            "; ;Backend " CFGS_BACKEND_NAME 
            ";    CFGS_VALUES_ROOT_DIR: " CFGS_VALUES_ROOT_DIR 
	    );
    return ret; 
}


