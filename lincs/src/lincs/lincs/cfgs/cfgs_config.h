
/**
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *  \file 
 *  \brief Main configuration header.  
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
 
#ifndef CSCONFIG_H
#define CSCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef HAVE_CONFIG_H
#  include "cfgs/cfgs_hostcfg.h"
#endif


#include <stdlib.h>


/** \def ONE_SHOT. For debugging purposes, no daemon forking or threading */
/** \def RANDOM_NULL. For testing purposes xmalloc functions will return NULL
    once in a while - check also FAIL_FREQ */


/** \def CFGS_VERSION.  See VERSION in config.h/cfgs_hostcfg.h */
#define CFGS_VERSION        VERSION
#define CFGS_VERSION_MAJOR  VERSION_MAJOR
#define CFGS_VERSION_MINOR  VERSION_MINOR

/* for each binary */
extern const char g_progname[];


#ifndef __cplusplus
typedef enum {false, true} bool;
#endif


/** \def CFGS_VALUES_ROOT_DIR The place where to store all config infos. */
/* FIXME: un par module ? */
#ifndef CFGS_VALUES_ROOT_DIR
#  error CFGS_VALUES_ROOT_DIR undefined!
#endif


/** Segment separator.  Ex: /network/eth0/ip */
#define CFGS_SEG_SEP  '/'


/** Folder where backends/modules are - environment variable. */
#define CFGS_ENV_MOD_PATH      "CFGS_MOD_PATH"


/** cfgs_configd daemon port */
#define CFGS_CONFIGD_PORT      (9000)
/** cfgs_configd unix sockets path */
#define CFGS_CONFIGD_PATH      "/tmp/cfgs_configd.sock"
/* FIXME: make those flexible. CGFS_MAX_BODY_LEN belongs to cfgs_protocol.h ? */ 
/** \def  CGFS_MAX_HDR_LEN http max accepted header length */
#define CGFS_MAX_HDR_LEN       (4098-1)
/** \def  CGFS_MAX_BODY_LEN max accepted xml body length */ 
#define CGFS_MAX_BODY_LEN      (4098-1)
/** \def CGFS_SOCK_TOUT Timeout(seconds) */
#define CGFS_SOCK_TOUT         (5) 
/** \def CGFS_MAX_CONNEXIONS Max. number of simultaneous connexions */
#define CGFS_MAX_CONNEXIONS    (1000)



/** Priviledged backend(s) */
#define CFGS_STACKER_BACKEND    "cfgs_stacker"
/* not sure at bootstrap time we should not load directly cfgs_fs_bk */
#define CFGS_BOOTSTRAP_BACKEND  CFGS_STACKER_BACKEND /*"cfgs_fs_bk"*/

/** Default layer if none is specified */
#define CFGS_DEFAULT_LAYER       "default"



#ifdef __cplusplus
}
#endif

#endif /*CSCONFIG_H*/
 
