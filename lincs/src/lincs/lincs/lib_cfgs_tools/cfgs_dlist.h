
/*
 *  $Revision: 1.4 $
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

#ifndef GENLIST_H
#define GENLIST_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs/cfgs_config.h"


typedef struct _cfgs_list cfgs_dlist;

struct _cfgs_list {
    cfgs_dlist *next;      /* chain forward pointer*/
    cfgs_dlist *prev;      /* if the head node of the list, this points to the tail */
    void      *userdata;  /* incase you want to use raw lists */
};

extern cfgs_dlist *cfgs_dlist_new( void *userdata );

/* "constructor": turn a structure having prev and next fields into a list with one elem */
extern cfgs_dlist *cfgs_dlist_cons( cfgs_dlist *head );

/* return the tail of the list */
extern cfgs_dlist *cfgs_dlist_tail( cfgs_dlist *head );
/* @return -1 on error */
extern int        cfgs_dlist_length( cfgs_dlist *head );

/* Function fo free a list item */
typedef void cfgs_free_item_func( void* );
/* If free is NULL, use xfree by default */
extern void      cfgs_dlist_free( cfgs_dlist *head, void (*free)(void *) );

#define CFGST_DLIST_FREE( list, free_item_func ) \
    cfgs_dlist_free( (cfgs_dlist*)list, (void (*)(void *))free_item_func );


/** If head is NULL, transform item into a valis list.  Returns head or NULL.  */
extern cfgs_dlist *cfgs_dlist_add_tail( cfgs_dlist *head, cfgs_dlist *item );
extern bool       cfgs_dlist_rem_tail( cfgs_dlist *head );


#ifdef __cplusplus
}
#endif

#endif /*GENLIST_H*/
 
