
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

#ifndef CSMEM_H
#define CSMEM_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs/cfgs_config.h"

/* See the RANDOM_NULL and FAIL_FREQ defines in the source file */


#define XCALLOC(type, num)                                  \
        ((type *) xcalloc ((num), sizeof(type)))
#define XMALLOC(type, num)                                  \
        ((type *) xmalloc ((num) * sizeof(type)))
#define XREALLOC(type, p, num)                              \
        ((type *) xrealloc ((p), (num) * sizeof(type)))
#define XFREE(stale)  do {                                  \
            if (stale) { free (stale);  stale = 0; }        \
        } while (0)

extern void *xcalloc( size_t num, size_t size );
extern void *xmalloc( size_t num );
extern void *xrealloc( void *p, size_t num );
extern char *xstrdup( const char *string );
extern char *xstrerror( int errnum );
extern void xfree( void *p );


#if WITH_DMALLOC
#  include <dmalloc.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /*CSMEM_H*/
 
