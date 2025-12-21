
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

#ifndef CSHASH_H
#define CSHASH_H

#ifdef __cplusplus
extern "C" {
#endif


#define CFGST_HASH_INVALID_IDX  (-1)

typedef struct _hash_elem cfgs_hash_elem;

struct _hash_elem
{
  const char     *key;
  void           *data;
  cfgs_hash_elem *next;
} ;

typedef struct
{
  int            nelem;
  cfgs_hash_elem **table;
} cfgs_hash;

cfgs_hash *cfgs_hash_new( unsigned int nelem );
int  cfgs_hash_string( const unsigned char *s, int n );

/**
 * @param table        hash table to update
 * @param key          key to hash on
 * @param data         data to associate with `key'
 * @param allow_dup    if nonzero, duplicate keys are allowed in the table 
 *
 * @return an index >=0 or CFGST_HASH_INVALID_IDX 
 */
int  cfgs_hash_insert( cfgs_hash * table, const char *key, void *data, int allow_dup );
void *cfgs_hash_find( const cfgs_hash * table, const char *key );
void cfgs_hash_delete( cfgs_hash * table, const char *key, 
               const void *data,
               void (*destroy) (void *) );

/**
 * @param hash         pointer to the hash table to be freed
 * @param destroy()    function to call to free the ->data member (optional) 
 */
void cfgs_hash_free( cfgs_hash * hash, void (*destroy) (void *) );



#ifdef __cplusplus
}
#endif

#endif /*CSHASH_H*/
 
