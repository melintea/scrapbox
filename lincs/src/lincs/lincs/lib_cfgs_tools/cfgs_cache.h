
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *  Cache object
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

#ifndef CSCACHE_H
#define CSCACHE_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs/cfgs_config.h"  /*bool*/


typedef struct _cfgs_cache cfgs_cache;

/*
 *  Elements in the cache are identified by name.  get_name is used to dig for the name in
 *  a struct.  
 */
cfgs_cache *cfgs_cache_new( const char* (*get_name)(void*) );
void       cfgs_cache_free( cfgs_cache *c, void (*free_item)(void *) );
bool       cfgs_cache_add( cfgs_cache *c, void *elem );
bool       cfgs_neg_hit( cfgs_cache *c, const char *name );
void       *cfgs_pos_hit( cfgs_cache *c, const char *name );


#ifdef __cplusplus
}
#endif

#endif /*CSCACHE_H*/
 
