
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

#include <stdio.h>
#include <errno.h>
#include <netdb.h>  /*h_errno*/
#include <string.h> 
#include <stdio.h>
#include <stdarg.h>


#include "cfgs_error.h"
#include "cfgs_log.h"
#include "cfgs_tags.h"


#ifdef X
#  error X macro defined
#endif
#define X(a, b)  b,
static const char *m_errstr[] = {
    CFGS_ERR_TABLE
};
#undef X


#define X( a, b ) b,
static const char *m_errtypes[] = {
    CFGS_ERRT_TABLE
};
#undef X


const char *
cfgs_strerror( CFGS_ERR err )
{
    lassert( err < CFGS_ERR_MAX && err >= 0 );
    if ( err < 0 || err > CFGS_ERR_MAX ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgs_strerror: invalid error code: %d", err); ); 
        return "";
    }
    return m_errstr[ err ];
}

/* FIXME: investigate getsockopt/SOL_SOCKET/SO_ERROR
   so_error/<sys/socketvar.h> */


/*FIXME: err to cfgs_buf* tag func to be reworked and exposed */
/**
 *  Prints tag string into @param err xml buffer.  
 *  @return number of chars printed ( see snprintf(3) ) or -1 on error.  
 */
static int        
cfgs_xml_err( cfgs_err *err )
{
    int ret = 0;
    
    lassert( err != NULL );
    if ( !err )
        return -1;

   ret = snprintf( err->xml, 3*CFGS_ERR_STR_MAX, 
             "<%s %s=\"%d\" %s=\"%s\" %s=\"%d\" %s=\"%s\" %s=\"%s\" />\r\n", 
             CFGS_TAG_ERROR, 
             CFGS_EA_ERR_TYPE,        err->errtype, 
             CFGS_EA_ERR_EXPLANATION, m_errstr[err->errtype], 
             CFGS_EA_ERR_CODE,        err->code, 
             CFGS_EA_ERR_STRERROR,    err->host_strerror, 
             CFGS_EA_ERR_INFO,        err->extra_infos 
             );
         
    return ret; 
}


void 
cfgs_seterr( 
            cfgs_err   *err, 
            CFGS_ERRT  type, 
            CFGS_ERR   code, 
            const char *custom_info,
            ...  
            ) 
{
    va_list    ciargs;
   
    va_start( ciargs, custom_info );
    cfgs_vseterr( err, type, code, custom_info, ciargs );
    va_end( ciargs );
}


void 
cfgs_vseterr( 
            cfgs_err   *err, 
            CFGS_ERRT  type, 
            CFGS_ERR   code, 
            const char *custom_info,
            va_list    ciargs
            ) 
{
   /* FIXME: might catch internal inconsistencies but need
      err->errtype set to CFGS_ERR_INTERNAL, etc */
   const char *estr = m_errstr[ CFGS_ERR_INTERNAL ];
   
   lassert( code < CFGS_ERR_MAX && code >=0 );
   
   lassert( err != NULL );
   if ( !err )
       return;
   
   if ( cfgs_iserr(err) ) 
       return; 
       
   memset( err, '\0', sizeof(cfgs_err) );
   
   err->errtype = type;
   switch ( type ) {
   case CFGS_ERRT_NO_ERROR: 
       err->code = CFGS_ERR_OK;
       estr = cfgs_strerror( code );
       break;
   case CFGS_ERRT_INTERNAL: 
       err->code = code;
       estr = cfgs_strerror( code );
       break;
   case CFGS_ERRT_ERRNO:    
       err->code = errno;
       estr = strerror( errno );
       break;
   case CFGS_ERRT_HERRNO:   
       err->code = h_errno;
       /* h_errlist[h_errno] */
       estr = strerror( errno );
       break;
   default:
       /* programming error */
       lassert( false );
       break;
   }
   snprintf( err->host_strerror, CFGS_ERR_STR_MAX, estr ); 
   
   /* FIXME: make sure to escape " in custom infos */
   if ( custom_info ) 
       vsnprintf( err->extra_infos, CFGS_ERR_STR_MAX, custom_info, ciargs ); 
   
   cfgs_xml_err( err );
   LOG( cfgs_log(CFGST_LL_CRITIC, "[%d] %s [%d] %s (%s)\n", 
        err->errtype, cfgs_strerror(err->errtype),
        err->code,    err->host_strerror, err->extra_infos); );
}


bool 
cfgs_err_from_tag( cfgs_err *err, cfgs_tag *t )
{
    const char *type, *code;
    
    lassert( err && t ); 
    if ( !err || !t )
        return false; 
    
    /* do not overwrite previous stored error */
    if ( cfgs_iserr(err) ) {
        return true; 
    }
    
    /*skip non-CFGS_TAG_ERROR tags. */
    if ( 0 != strcmp(CFGS_TAG_ERROR, t->type) ) {
        /* Might be a programming error? */
        return false;
    }
    
    type = cfgs_tag_attr( t, CFGS_EA_ERR_TYPE );
    code = cfgs_tag_attr( t, CFGS_EA_ERR_CODE );
    if ( type && code ) {
        err->errtype = atoi( type );
        err->code    = atoi( code );
        strncpy( err->host_strerror, SAFE_STR(cfgs_tag_attr(t,CFGS_EA_ERR_STRERROR)), CFGS_ERR_STR_MAX );
        strncpy( err->extra_infos,   SAFE_STR(cfgs_tag_attr(t,CFGS_EA_ERR_INFO)), CFGS_ERR_STR_MAX );
        return true; 
    }
    
    cfgs_xml_err( err );
    
    return false; 
}


bool       
cfgs_iserr( const cfgs_err *err )
{
    lassert( err != NULL );
    
    if (  (err->errtype == CFGS_ERRT_NO_ERROR)
       || (err->errtype == CFGS_ERRT_INTERNAL && err->code == CFGS_ERR_OK) 
       || (err->errtype == CFGS_ERRT_ERRNO && err->code == 0)
       || (err->errtype == CFGS_ERRT_HERRNO && err->code == 0)
       /*|| *err->strerror*/
       )
        return false;
    
    return true;
}


void
cfgs_perror( const cfgs_err *err, const char *msg, FILE *fd )
{
    if ( !fd )
        return;

    if ( !err )
        return;
    
    if ( msg )
        fprintf( fd, "%s: ", msg );    
    
    fprintf( fd, "[%d] %s [%d] %s (%s)\n", 
        err->errtype, cfgs_strerror(err->errtype),
        err->code,    err->host_strerror, err->extra_infos );
}



