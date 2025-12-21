
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:19:24 $
 *
 *  Adapted from mutt/hash.c/.h ver 3.0
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

#include <string.h>

#include "cfgs_hash.h"
#include "cfgs_mem.h"
#include "cfgs_log.h"

#define SOMEPRIME 149711


int 
cfgs_hash_string( const unsigned char *s, int n )
{
  int h = 0;
  
  lassert( s != NULL );

#if 0
  while (*s)
    h += *s++;
#else
  while (*s)
    h += (h << 7) + *s++;
  h = (h * SOMEPRIME) % n;
  h = (h >= 0) ? h : h + n;
#endif

  return (h % n);
}


cfgs_hash*
cfgs_hash_new( unsigned int nelem )
{
  cfgs_hash *table = XCALLOC( cfgs_hash, 1 );
  if ( !table )
      return NULL;
      
  if( nelem == 0 )
    nelem = 5; /*default*/
    
  table->nelem = nelem;
  table->table = XCALLOC( cfgs_hash_elem*, nelem );
  if ( !table->table ) {
      xfree( table );
      return NULL;
  }

  return table;
}


int 
cfgs_hash_insert( cfgs_hash *table, const char *key, void *data, int allow_dup )
{
  cfgs_hash_elem *ptr;
  int            h;

  if ( !table || !key )
      return CFGST_HASH_INVALID_IDX;
      
  ptr = XCALLOC( cfgs_hash_elem, 1 );
  if ( !ptr )
      return CFGST_HASH_INVALID_IDX;
      
  h   = cfgs_hash_string( (const unsigned char *) key, table->nelem );
  ptr->key  = key;
  ptr->data = data;

  if( allow_dup ) {
    ptr->next = table->table[h];
    table->table[h] = ptr;
  } else {
    cfgs_hash_elem *tmp, *last;
    int r;

    for ( tmp = table->table[h], last = NULL; tmp; last = tmp, tmp = tmp->next ) {
      r = strcmp( tmp->key, key );
      if (r == 0) {
          xfree( ptr );
          LOG( cfgs_log(CFGST_LL_CRITIC, "cfgs_hash_insert failed\n"); );
          return CFGST_HASH_INVALID_IDX;
      }
      if ( r > 0 )
          break;
    }
    if ( last )
      last->next = ptr;
    else
      table->table[h] = ptr;
    ptr->next = tmp;
  }

  return h;
}


static void *
hash_find( const cfgs_hash * table, int hash, const char *key )
{
  cfgs_hash_elem *ptr;
  
  lassert( table && key );
  
  ptr = table->table[hash];
  for( ; ptr; ptr = ptr->next ) {
    if (strcmp(key, ptr->key) == 0)
      return (ptr->data);
  }
  return NULL;
}


void *
cfgs_hash_find( const cfgs_hash * table, const char *key )
{
    void *d;
    
    if ( !table || !key )
        return NULL;
    
    d = hash_find( table, cfgs_hash_string((const unsigned char *)key, table->nelem), key );
    
    return d;
}


static void 
hash_delete( cfgs_hash * table, int hash, const char *key, const void *data,
               void (*destroy) (void *) )
{
  cfgs_hash_elem *ptr   = table->table[hash];
  cfgs_hash_elem **last = &table->table[hash];

  for ( ; ptr; last = &ptr->next, ptr = ptr->next ) {
    /* if `data' is given, look for a matching ->data member.  this is
     * required for the case where we have multiple entries with the same
     * key
     */
    if ( (data == ptr->data) || (!data && strcmp (ptr->key, key) == 0) ) {
      *last = ptr->next;
      if ( destroy ) 
          destroy (ptr->data);
      xfree( ptr );
      return;
    }
  }
}


void 
cfgs_hash_delete( cfgs_hash * table, const char *key, const void *data,
               void (*destroy) (void *) )
{
    if ( !table || !key )
        return;
    hash_delete( table, 
            cfgs_hash_string((const unsigned char *)key, table->nelem), 
            key, data, destroy );
}


void 
cfgs_hash_free( cfgs_hash *ptr, void (*destroy) (void *) )
{
  int i;
  cfgs_hash *pptr = ptr;
  cfgs_hash_elem *elem, *tmp;

  for ( i = 0 ; i < pptr->nelem; i++ ) {
    for ( elem = pptr->table[i]; elem; ) {
      tmp = elem;
      elem = elem->next;
      if ( destroy )
          destroy (tmp->data);
      xfree( (void *) tmp );
    }
  }
  xfree( (void *) pptr->table );
  xfree( (void *) pptr );
}


