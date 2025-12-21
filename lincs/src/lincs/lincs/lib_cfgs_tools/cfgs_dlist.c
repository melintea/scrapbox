
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/18 17:05:44 $
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


#include "cfgs_dlist.h"
#include "cfgs_mem.h"

#include "cfgs_log.h"


cfgs_dlist *
cfgs_dlist_new( void *userdata )
{
  cfgs_dlist *newl = XMALLOC (cfgs_dlist, 1);
  if ( !newl )
      return NULL;

  newl->next = NULL;
  newl->prev = newl; /*tail!*/
  newl->userdata = userdata;

  return newl;
}


cfgs_dlist *
cfgs_dlist_cons( cfgs_dlist *head )
{
  lassert( head != NULL );
  if ( head == NULL )
      return NULL;

  head->prev = head;
  return head;
}


cfgs_dlist *
cfgs_dlist_tail( cfgs_dlist *head )
{
  if ( head == NULL )
      return NULL;
  return head->prev;
}


int 
cfgs_dlist_length( cfgs_dlist *head )
{
  size_t n;
  
  lassert( head != NULL );
  if ( head == NULL )
      return -1;

  for ( n = 0; head; ++n )
      head = head->next;

  lassert( n > 0 );
  return n;
}


#define CFGST_DLIST(type, pitem)  (type*)cfgs_dlist_cons((cfgs_dlist*)pitem)
cfgs_dlist *      
cfgs_dlist_add_tail( cfgs_dlist *head, cfgs_dlist *item )
{
    cfgs_dlist *tail = NULL;
    
    if ( !item )
        return NULL;
    
    /* construct list around the item */
    if ( head == NULL ) {
        cfgs_dlist *h = cfgs_dlist_cons( (cfgs_dlist*)item );
	    return h;
    }
    
    tail = cfgs_dlist_tail( head );
    /* list has not been constructed with cfgs_dlist_cons/CFGST_DLIST */
    lassert( tail != NULL && item != NULL );    
    if ( !tail )
        return NULL;

    tail->next = item;
    item->prev = tail;
    item->next = NULL;
    
    head->prev = item;
    
    return head; 
}


bool      
cfgs_dlist_rem_tail( cfgs_dlist *head )
{
    cfgs_dlist *tail = NULL;

    lassert( head );    
    if ( !head )
        return false;
    
    tail = cfgs_dlist_tail( head );
    if ( !tail )
        return false;
    
    tail = tail->prev;

    tail->next = NULL;
    head->prev = tail;
    
    return true; 
}


void      
cfgs_dlist_free( cfgs_dlist *head, void (*free)(void *) )
{
    cfgs_dlist   *crt = head;
    void (*iff)(void *) = free ? free : xfree;
    
    while ( crt ) {
        cfgs_dlist *next = crt->next;
        (*iff)( crt );
        crt = next;
    }
}


