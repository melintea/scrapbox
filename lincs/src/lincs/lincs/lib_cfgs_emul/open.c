
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:19:24 $
 *
 *  NOTE: /etc/ld.so.preload 
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


#define INVALID_FD    (-1)

#define TMPFILE_BUFSZ (128)
#define TMPFILE_TEMPL "/tmp/cs_emul.XXXXXX"

#define REGENERATE_CMD "exec " CFGS_BIN_PATH "/" "cfgs_cat %s"


//-------------------------------------------------------------------
#if 0
#  define LOGF "/LIBEMUL.LOG"

static void 
elog( const char *s )
{
    int fd = __libc_open( LOGF, O_APPEND );
    __libc_write( fd, s, strlen(s) );
    __libc_write( fd, '\n', 1 );
    __libc_close( fd );
    fprintf( stderr, "*** %s\n", s );
}
#endif
//-------------------------------------------------------------------

 
/* FIXME: a 64 version ? */
#define check_if_regenerated64  check_if_regenerated

/*
 * If returned val is > 0 (a valid file descriptor) all went ok, else
 * returns INVALID_FD.  tmpfile has been closed.  
 */
#define CMDBUF_SZ  (2*FILENAME_MAX + 128)
static int
check_if_regenerated( const char *pathname, int flags, char *tmpfile )
{
    char cmdbuf[ CMDBUF_SZ ] = {0};
    FILE *regf = NULL;
    
    /* FIXME: 
           if !pathname starts with /etc then 
               if !cwd is /etc then
                   return
     */
    
    /* avoid a popen loop when opening /dev/tty on some machines */
    if ( pathname == strstr(pathname, "/dev") )
        return INVALID_FD;
    
    snprintf( cmdbuf, CMDBUF_SZ-1, REGENERATE_CMD, pathname );
    /*FIXME: get rid of popen => pipe+fork+execl+wait4*/
    regf = popen( cmdbuf, "r" );
    if ( regf ) {
        int tmpfd = INVALID_FD;

    tmpfd = mkstemp( tmpfile );

        if ( tmpfd == INVALID_FD ) {
            pclose( regf );
            return INVALID_FD;
        }

        while ( !feof(regf) ) {
            char    buf[ 1024 ];
            size_t  len;
            len  = fread( buf, 1, 1024, regf );
            __libc_write( tmpfd, buf, len );
        }
       
        if ( pclose(regf) == -1 ) {
            __libc_close( tmpfd );
            unlink( tmpfile );
            return INVALID_FD;
        }
        
        __libc_close( tmpfd );
        return tmpfd;
    } /*regf*/
    
    return INVALID_FD;
}


/*
 *  if file can be regenerated
 *      if O_WRONLY || O_RDWR || O_CREAT then return invalid handle
 *      return regenerated
 *
 *  return __libc_open...
 */
int 
open( const char *pathname, int flags, ... ) 
{
    int  ret = INVALID_FD;
    char tmpfile[ TMPFILE_BUFSZ ] = TMPFILE_TEMPL;

    ret = check_if_regenerated( pathname, flags, tmpfile );
    if ( ret != INVALID_FD ) {
        int fd;
    
        if ( flags & O_WRONLY || flags & O_RDWR || flags & O_CREAT ) {
            unlink( tmpfile );
            errno = EACCES; /*EROFS*/
            return INVALID_FD;
        }
        
        fd = __libc_open( tmpfile, flags );
        /* do not leave behind a bunch of TMPFILE_TEMPL files in /tmp */
        unlink( tmpfile );
        return fd;
    }

    if ( flags && O_CREAT ) {
        mode_t mode;
        va_list ap;
        va_start( ap, flags );
        mode = va_arg( ap, mode_t );
        va_end( ap );
        return __libc_open( pathname, flags, mode );
    }
    else 
        return __libc_open( pathname, flags );
}


int 
open64( const char *pathname, int flags, ... ) 
{
    int  ret = INVALID_FD;
    char tmpfile[ TMPFILE_BUFSZ ] = TMPFILE_TEMPL;

    ret = check_if_regenerated64( pathname, flags, tmpfile );
    if ( ret != INVALID_FD ) {
        int fd;
    
        if ( flags & O_WRONLY || flags & O_RDWR || flags & O_CREAT ) {
            unlink( tmpfile );
            errno = EACCES; /*EROFS*/
            return INVALID_FD;
        }
        
        fd = __libc_open64( tmpfile, flags );
        unlink( tmpfile );
        return fd;
    }

    if ( flags && O_CREAT ) {
        mode_t mode;
        va_list ap;
        va_start( ap, flags );
        mode = va_arg( ap, mode_t );
        va_end( ap );
        return __libc_open64( pathname, flags, mode );
    }
    else 
        return __libc_open64( pathname, flags );
}


