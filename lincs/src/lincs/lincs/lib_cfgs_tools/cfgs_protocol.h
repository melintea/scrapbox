
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
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

#ifndef CFGS_PROTOCOL_H
#define CFGS_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs_client_api.h"
#include "cfgs_str.h"
#include "cfgs_error.h"
#include "cfgs_backend.h"


#define CFGS_PROTOCOL              "cfgs_configd"
#define CFGS_PROTOCOL_VERSION      "1.0"
#define CFGS_PROTOCOL_VERSION_1_0  CFGS_PROTOCOL_VERSION


#define X(a,b)  a,
typedef enum {
    CFGS_API_EXPORTS
} CFGS_FUNC_INDEX;
#undef X

#define INVALID_CFGS_FUNC_INDEX  (-1)


/** Host protocol - the protocol that transports the xml messages.  
    Server/client side. */
typedef struct _cfgsp_hosting_protocol {
    bool      (*client_send)( int sock, char *buf, int len, cfgs_err *err );
    cfgs_buf* (*client_recv)( int sock, cfgs_err *err );
    bool      (*server_send)( int sock, char *buf, int len, cfgs_err *err );
    cfgs_buf* (*server_recv)( int sock, cfgs_err *err );
} cfgsp_hosting_protocol;

/** Which protocol to use to transport messages */ 
typedef enum {
    CFGSP_HOST_PROTO_HTTP,
} CFGSP_HOST_PROTO;


/*FIXME: include in session sock & hproto ? */
/** entry point for client */
void *cfgsp_send_rq( 
    cfgs_session     *sess, 
    int              sock, 
    CFGSP_HOST_PROTO hproto,
    CFGS_FUNC_INDEX  idx, 
    ... 
    );



/** Protocol callback parameters catch-all.  Server side. */
typedef struct _cfgsp_data {
    cfgs_session    *sess;
    CFGS_FUNC_INDEX idx;
    /***/
    cfgs_tag *attribs; /* do not free! */
} cfgsp_data;
/** Tag processing callback */
typedef void* (CFGSP_CALLBACK)( cfgsp_data* );


/** Server side.  */
bool cfgsp_process_rq( 
    int               sock,
    CFGSP_HOST_PROTO  hproto,
    CFGSP_CALLBACK    *tag_callback, 
    cfgsp_data        *cb_data 
    );
    

#ifdef __cplusplus
}
#endif

#endif /*CFGS_PROTOCOL_H*/
 
