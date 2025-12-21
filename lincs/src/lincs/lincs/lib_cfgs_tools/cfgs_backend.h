
/**
 *  $Revision: 1.5 $
 *  $Date: 2004/03/30 21:00:59 $
 *
 *  \file 
 *  \brief Backend specific structures & API.  
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

#ifndef CSBACKEND_H
#define CSBACKEND_H

#ifdef __cplusplus
extern "C" {
#endif


#include "ltdl.h"
#include "cfgs_client_api.h"
#include "cfgs_str.h"



#undef  CFGS_BACKEND_NAME  /* defined for each backend */


/**
 * Init/shutdown ltdl engine
 */
bool cfgsb_init( void );
bool cfgsb_shutdown( void );



#if CFGS_CRT_REV != 4
#  error please update _cfgs_backend to CFGS_CRT_REV if needed
#endif
typedef struct _cfgs_backend cfgs_backend;
/** \struct _cfgs_backend
 *  \brief  Structure that allows backend manipulations.  
 */
struct _cfgs_backend {
    cfgs_backend  *next; /**< Chained list pointers */
    cfgs_backend  *prev;
    char        *name;
    lt_dlhandle handle;
    bool        (*on_load)( void );   /**< Function executed at load time */
    bool        (*on_unload)( void ); /**< Function executed when backend is unloaded */
    cfgs_entry* (*cfgs_getval)( cfgs_session *sess, const char *name, const char *layer );
    int         (*cfgs_setval)( cfgs_session *sess, cfgs_entry *vl );
    int         (*cfgs_rmval)(  cfgs_session *sess, const char *name, const char *layer );
    bool        (*cfgs_register_notif)( cfgs_session *s, cfgs_notif *notif );
    cfgs_str*   (*cfgs_getsubvals)( cfgs_session *s, const char *valname, const char *layer );
    cfgs_str*   (*cfgs_getsublayers)( cfgs_session *s, const char *layername );
    cfgs_str*   (*cfgs_getinfos)( cfgs_session *s );
};

cfgs_backend *cfgsb_backend_new( void );
void         cfgsb_backend_free( cfgs_backend *bk );

cfgs_backend *cfgsb_load_backend( const char *name );
bool         cfgsb_unload_backend( cfgs_backend *bk );

/** 
 *  @param names is a NULL terminated list.
 *  @return a linked list. 
 */
cfgs_backend *cfgsb_load_backends( const char **names );
bool         cfgsb_unload_backends( cfgs_backend *backends );

cfgs_backend *cfgsb_find_backend( cfgs_backend *bklist, const char *name );



#ifdef __cplusplus
}
#endif

#endif /*CSBACKEND_H*/
 
