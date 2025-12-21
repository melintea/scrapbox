
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/22 18:38:24 $
 *
 *  Mapping values to filesystem entities. 
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <glob.h>
#include <fnmatch.h>

#include "fs.h"
#include "cfgs_log.h"
#include "cfgs_hash.h"
#include "cfgs_mem.h"



/* FIXME: mem free ? */
/* FIXME: use cfgs_cache.h */



cfgs_entry *
is_cached( const char *name )
{
    return NULL;
}


/* check negative hits cache */
bool
does_not_exists( const char *name )
{
    return false;
}


void
add_to_positive_hit( const char * name )
{
    /*FIXME cfgs_cache.h*/
}


void
add_to_negative_hit( const char * name )
{
    /*FIXME cfgs_cache.h*/
}


/* how many path segments are in name */
/* FIXME: maxlen, len useless */
static int
nsegs( const char *name )
{
    const char *p = name;
    int  nsegs = 0;
    int  maxlen = 0, len = 0;

    if ( !name )
        return 0;

    while ( *p ) {
        if ( *p == CFGS_SEG_SEP || *p == FS_PATH_SEP_C ) {
            nsegs++;
            if ( len > maxlen ) {
                maxlen = len;
                len = 0;
            }
        } else
            len++;
        
        p++;
    }

    return nsegs;
}


static int
makedir( const char *dir )
{
    int ret;
   
    lassert( dir != NULL );
    ret = mkdir( dir, FS_DIR_PERM );
    LOG( cfgs_log(CFGST_LL_INFO, "mkdir %s returns %d\n", dir, ret); );
    
    if ( ret == -1 && errno == EEXIST ) {
        errno = 0;
        ret = 0;
    }
TEST_ERROR    
    return ret;
}


static bool
file_on_disk( const char *file )
{
    struct stat s;
    int         ret;
   
    lassert( file != NULL );
        
    ret = stat( file, &s );
    if ( ret==0 && S_ISREG(s.st_mode) ) 
        return true;
        
    /*FIXME: errno 2 no such file should be reported ? */
    LOG( cfgs_log(CFGST_LL_CRITIC, "file_on_disk: %s\n", file ); );
    if ( errno == ENOENT || errno == ENOTDIR ) {
        errno = 0;
    }
    
TEST_ERROR    
    return false;
}


/* FIXME: use creat*/
/* ret 0 on success */
static int
makefile( const char *fname )
{
    struct stat s;
    int         ret;
   
    lassert( fname != NULL );
        
    ret = stat( fname, &s );
    if ( ret==0 && !(S_ISREG(s.st_mode)) ) {
TEST_ERROR    
        LOG( cfgs_log(CFGST_LL_CRITIC, "%s is not a regular file!\n", fname); );
        return -1;
    }
    
    if ( ret != 0 && errno == ENOENT ) {
        FILE *f;
        
        errno = 0; 
        f  = fopen( fname, "w" );
        if ( !f ) {
            LOG( cfgs_log(CFGST_LL_CRITIC, "fopen(w) '%s'\n", fname); );
            return -1;
        }
        fclose( f );
TEST_ERROR        
        return 0;
    }
TEST_ERROR    
    return ret;
}


static bool
dir_exists( const char *dir, const char *seg, bool forcecreate )
{
    static bool dir_on_disk( const char *dir );

    char crt[ FILENAME_MAX ] = {0};
    
    //printf( "** %s  =>  %s \n", dir, seg );
    snprintf( crt, FILENAME_MAX-1, "%s%s", dir, seg );
    
    //if ( cache && in_cache(cache, crt) != -1 ) {
    //    return true;
    //}
TEST_ERROR
    if ( dir_on_disk(crt) )
        return true;
    else {
        if ( forcecreate ) {
            if ( makedir(crt) != 0 )
                return false;
        
            //if ( cache )
            //    add_to_cache( cache, crt );
            
            return true;
        } else
            return false;
    }
    
    return false;
}


static bool
file_exists( const char *file, bool forcecreate )
{
    int  ret;

    if ( forcecreate ) {
        ret = makefile( file );
        return ret == 0;
    } else
        return file_on_disk( file );

    return false;
}


static int 
check_vals( const char *dir, 
        bool forcecreate, 
        callonmatch *mf, match_data *data )
{
    char crt[ FILENAME_MAX ] ={0};
    int  num = 0;
    
    snprintf( crt, FILENAME_MAX-1, "%s/%s", dir, VALS_FILE );
    //printf( "**** %s \n", crt );
    
    if ( !file_exists(crt, forcecreate) ) {
        /*add_to_negative_hit( stack_to_path(valstack, STK_TOP) );*/
        return 0;
    }

    /*
     *  work
     */
    data->filename = crt;
    num += (*mf)( data );
    if ( num > 0 ) {
        add_to_positive_hit( dir );
    }
    
    return num;
}


typedef struct _srch_dirs {
    char path[ FILENAME_MAX ];
    char crtd[ FILENAME_MAX ]; /* current dir   */
    char seg[ FILENAME_MAX ];  /* child of crtd */
} srch_dirs;


/* returns -1 on error.  */
static int 
fs_search_exact( const char *rootdir, const char *valname, 
        bool forcecreate, 
        callonmatch *mf, match_data *data 
        )
{
    int        num = 0;
    srch_dirs  *srch;
    const char *bs, *es, *end;
    char       *p;

    LOG( cfgs_log(CFGST_LL_CRITIC, "fs_search_exact\n"); );

    srch = XCALLOC( srch_dirs, 1 );
    if ( !srch )
        return -1;
    
    /* lassert( sizeof(srch->path) >= FILENAME_MAX ); */
    snprintf( srch->path, FILENAME_MAX-1, "%s%s", rootdir, valname );
#if 1
    /* FIXME: if path separator is not the same as CFGS_SEG_SEP. Check tests. */
    for ( p=srch->path; *p; p++ ) {
        if ( *p == '.' )
            *p = FS_PATH_SEP_C;
    }
#endif
    snprintf( srch->crtd, FILENAME_MAX-1, FS_ROOT_DIR );
    
    es  = srch->path; 
    end = srch->path + strlen( srch->path );
    lassert( *end == 0 );
    bs  = es;
    while ( bs < end ) {
        /* find limits of a path segment */
        bs = es;
        es = strchr( es+1, FS_PATH_SEP_C );
        if ( !es )
            es = end;
        //printf( "\n%s\n%s\n", bs, es );
        
        strcat( srch->crtd, srch->seg );
        strncpy( srch->seg, bs+1, es-bs );
        srch->seg[ es-bs ] = '\0';
        
        if ( !dir_exists( srch->crtd, srch->seg, forcecreate ) ) {
            xfree( srch );
            return num;
        } else {
            ;
            /*num += check_vals( srch->crtd, forcecreate, mf, data );*/
        }
    }
    
    num += check_vals( srch->crtd, forcecreate, mf, data );
    xfree( srch );
    return num;
}


static int
globerr( const char *epath, int eerrno )
{
    LOG( cfgs_log(CFGST_LL_CRITIC, 
            "globerr: glob failed '%s' ('%d')\n", epath?epath:"", eerrno); );
    errno = 0;
    return 0;
}


//FIXME: prints only first value.  Problem might be elsewhere
static int
glob_recursive( char *crtdir, const char *pattern, callonmatch *mf, match_data *data )
{
    int    ret;
    int    i;
    int    num = 0;
    glob_t matches;
    
    lassert( crtdir && pattern && mf && data );

/* FIXME: configure.in check? */
#if defined __CYGWIN32__ || defined __CYGWIN__ 
#  define GLOB_ONLYDIR (0)   
#endif
TEST_ERROR
    ret = glob( crtdir, GLOB_NOSORT|GLOB_ONLYDIR, globerr, &matches );
    if ( ret != 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, 
            "glob failed for '%s'(ret='%d')\n", crtdir, ret); );
        if ( errno == ENOTDIR )  errno = 0;
        return 0; /*FIME: -1? will cause name='*' to fail if no vals*/
    }
    
    for ( i=0; i<matches.gl_pathc; i++ ) {
        int    n = 0;
    
        /* FIXME: use crtdir as a file name */
        snprintf( crtdir, FILENAME_MAX-1, "%s%s%s", 
                matches.gl_pathv[i], FS_PATH_SEP_S, VALS_FILE );
TEST_ERROR        
        if (  file_exists(crtdir, false) 
           &&  0 == fnmatch(pattern, matches.gl_pathv[i], 0) 
           ) {
            LOG( cfgs_log(CFGST_LL_INFO, "glob & fnmatch: '%s' \n", matches.gl_pathv[i]); );
            //printf( "**** match: %s \n", matches.gl_pathv[i] );
            
            data->filename = crtdir;
            n = (*mf)( data );
            if ( n > 0 ) {
                num += n;
                //FIXME: matches.gl_pathv[i] will be freed. xstrdup?
                add_to_positive_hit( matches.gl_pathv[i] ); 
            }
        }
TEST_ERROR    
        snprintf( crtdir, FILENAME_MAX-1, "%s%s%s", 
                matches.gl_pathv[i], FS_PATH_SEP_S, "*" );
        num += glob_recursive( crtdir, pattern, mf, data );
    }
    
    globfree( &matches );
    return num;
}


typedef struct _srch_pat {
    char pattern[ FILENAME_MAX ];
    char crtdir[ FILENAME_MAX ]; 
} srch_pat;

/*
 *  Note: la facon la plus simple de tester la presence d'une valeur serait de 
 *  chdir dans le repertoire CFGS_VALUES_ROOT_DIR "/" MOD_NAME "/" valuename.  Mais ceci
 *  ne marcherait pas pour des recherches de type '.net.*' - on doit chercher
 *  niveau par niveau.  A discuter. 
 *  FIXME: fwt 
 *  Return -1 on error.  
 */
static int 
fs_search_regexp( const char *rootdir, const char *valname, 
        bool forcecreate, 
        callonmatch *mf, match_data *data 
        )
{
    srch_pat *pat = NULL;
    int      num = 0;
    
    pat = XCALLOC( srch_pat, 1 );
    if ( !pat ) 
        return -1;
    
    snprintf( pat->pattern, FILENAME_MAX-1, "%s%s%s", rootdir, FS_PATH_SEP_S, valname );
    snprintf( pat->crtdir,  FILENAME_MAX-1, "%s%s%s", rootdir, FS_PATH_SEP_S, "*" );
TEST_ERROR    
    num = glob_recursive( pat->crtdir, pat->pattern, mf, data );
TEST_ERROR    
    xfree( pat );
    return num;
}


bool
is_regexp( const char *name )
{
    const char *p;
    
    for ( p=name; *p; p++ ) {
        if ( *p == '*' || *p == '?' )
            return true;;
    }
    
    return false;
}


int 
fs_search( const char *rootdir, const char *valname, 
        bool forcecreate, 
        callonmatch *mf, match_data *data 
        )
{
    lassert( rootdir != NULL && valname != NULL ); 

    if ( valname == NULL )
        return -1;
    
    lassert( FILENAME_MAX > strlen(rootdir)+strlen(valname) );
    if ( FILENAME_MAX < strlen(rootdir)+strlen(valname) ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, 
                "fs_search: path too long '%s' + '%s'\n", rootdir, valname); );
        return -1;
    }
    
    /* FIXME: evaluate if it is worth this exact/regexp split */
    if ( is_regexp(valname) )
        return fs_search_regexp( rootdir, valname, forcecreate, mf, data );
    else
        return fs_search_exact( rootdir, valname, forcecreate, mf, data );
}


bool
dir_on_disk( const char *dir )
{
    struct stat s;
    int         ret;
   
    lassert( dir != NULL );
TEST_ERROR        
    ret = stat( dir, &s );
TEST_ERROR    
    if ( ret==0 && S_ISDIR(s.st_mode) ) 
        return true;
    
    return false;
}



