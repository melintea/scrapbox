
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *  \file 
 *  \brief  Send/Receive data over http
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

#ifndef HTTP_PROTOCOL_H
#define HTTP_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs_str.h"
#include "cfgs_error.h"


bool      http_client_send( int sock, char *buf, int len, cfgs_err *err );
/** 
 * Verification of validity included.  Free returned poiter 
 */ 
cfgs_buf *http_client_recv( int sock, cfgs_err *err );

bool      http_server_send( int sock, char *buf, int len, cfgs_err *err );
/** Receive request.  Verification of validity included.  Free returned poiter */ 
cfgs_buf *http_server_recv( int sock, cfgs_err *err );


#ifdef __cplusplus
}
#endif

#endif /*HTTP_PROTOCOL_H*/
 
