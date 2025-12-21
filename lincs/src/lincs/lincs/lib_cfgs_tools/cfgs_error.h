
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *  \file
 *  \brief Error information manipulation API & structures. 
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

#ifndef CSERROR_H
#define CSERROR_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>

#include "cfgs_str.h"


/** Error type */
#define CFGS_ERRT_TABLE \
    X( CFGS_ERRT_NO_ERROR, "No error" ) \
    X( CFGS_ERRT_INTERNAL, "Internal" ) \
    X( CFGS_ERRT_ERRNO,    "errno" )    \
    X( CFGS_ERRT_HERRNO,   "h_errno" )  \
    /**/
    
#define X( a, b ) a,
typedef enum {
    CFGS_ERRT_TABLE
} CFGS_ERRT;
#undef X


/* FIXME: unify with protocol errors */
/** 
 * CFGS_ERRT_INTERNAL is one of:
 */
/*  sym                     explanation */
#define CFGS_ERR_TABLE \
    X( CFGS_ERR_OK,         "Ok") \
    X( CFGS_ERR_INTERNAL,   "cfgs_geterr: internal programming error") \
    /* protocol */ \
    X( CFGSP_ERR_VERSION,          "Unsupported/unknown protocol version" ) \
    X( CFGSP_ERR_FUNC_CALL,        "Unsupported function call" ) \
    X( CFGSP_ERR_SERVER_CONNECT,   "Not connected to server" ) \
    X( CFGSP_ERR_SERVER,           "Unknown Server error" ) \
    X( CFGSP_MAX_CONN,             "Maximum number of clients connected to server" ) \
    /*  */ \
    X( CFGS_ERR_MAX,        "Keep last")
    

#ifdef X
#  error X macro defined
#endif
#define X(a, b)  a,
typedef enum {
    CFGS_ERR_TABLE
} CFGS_ERR;
#undef X


#define CFGS_ERR_STR_MAX (255)
typedef struct _cfgs_err cfgs_err;
/** \struct _cfgs_err
 *  \brief  Error related informations.
 */
struct _cfgs_err {
    CFGS_ERRT errtype; /* has two corresponding tag params: 
                        *  CFGS_EA_ERR_TYPE
                        *  CFGS_EA_ERR_EXPLANATION
                        */
    int       code;   /**< h_errno/errno/CFGS_ERR */
    char      host_strerror[ CFGS_ERR_STR_MAX+1 ]; /**< Host-specific */
    char      extra_infos[ CFGS_ERR_STR_MAX+1 ]; 
    char      xml[ 3*CFGS_ERR_STR_MAX+1 ]; /*FIXME: get rid of this anomaly*/
};


/**
 *  Returns the error string from CFGS_ERRT_TABLE corresponding to err.  
 */
const char *cfgs_strerror( CFGS_ERR err );
/**
 *  Store error infos into @param err.  Will not overwrite previous stored 
 *  error infos.  
 */
void cfgs_seterr( 
                cfgs_err   *err, 
                CFGS_ERRT  type, 
                CFGS_ERR   code,         /* If needed */
                const char *custom_info, /* Format specification */
                ...  
                );
/** @see cfgs_seterr */
void cfgs_vseterr( 
                cfgs_err   *err, 
                CFGS_ERRT  type, 
                CFGS_ERR   code,         /* If needed */
                const char *custom_info,
                va_list    ap
                );
bool       cfgs_iserr( const cfgs_err *err );
/** Will not override a previous stored error.
    @return true if operation successfull. */
bool       cfgs_err_from_tag( cfgs_err *err, cfgs_tag *tag ); 

/** perror like printing with the followig format: <br>
 *  [CFGS_ERRT errtype] errtype explanation [code] host_strerror (extra_infos) <br>
 *  (See @see cfgs_err). 
 */
void       cfgs_perror( const cfgs_err *err, const char *msg, FILE *fd ); 


#ifdef __cplusplus
}
#endif

#endif /*CSERROR_H*/
 
