
/*
 *  $Revision: 1.4 $
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

#ifndef FS_H
#define FS_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs/cfgs_config.h"
#include "cfgs_client_api.h"
#include "cfgs_hash.h"

#include <stdio.h>



/* keep values into files named VALS_FILE */
#define VALS_FILE  "VALUES"

/* access rights  */
#define FS_DIR_PERM   (0750)
#define FS_FILE_PERM  ()

#define FS_PATH_SEP_C  '/'
#define FS_PATH_SEP_S  "/"

#define FS_ROOT_DIR    "/"


/*
 *  Catch all structure for in/out callback params.  
 */
typedef struct _match_data {
    char   *valname;
    cfgs_entry *value;    /*in*/
    char   *filename;
    cfgs_entry **vlist;   /*out*/
} match_data;

/* function to call when we found the file corresponding to a value name */
typedef int callonmatch( match_data* );


/*  Scans the filesytem for value files that are matching the valname criteria, 
 *  starting at rootdir.  Will create intermediate folders if forcecreate.  
 *  Returns the number of matches or -1 on error.   
 */
int fs_search( const char *rootdir, const char *valname, 
        bool forcecreate, 
        callonmatch *mf, match_data *data 
        );

/* negative hit */
bool   does_not_exists( const char *name );
/* positive hit */
cfgs_entry *is_cached( const char *name );

bool is_regexp( const char *name );

/** @return true if @param dir exists and is a directory.  */
bool dir_on_disk( const char *dir );




#ifdef __cplusplus
}
#endif

#endif /*FS_H*/
 
