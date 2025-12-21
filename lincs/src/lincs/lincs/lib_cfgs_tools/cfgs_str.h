
/*
 *  $Revision: 1.4 $
 *  $Date: 2004/03/30 20:39:55 $
 *
 *  Text manipulation
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

#ifndef CSSTRINGLIST_H
#define CSSTRINGLIST_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs_hash.h"
#include "cfgs_tags.h"


typedef struct _cfgs_str cfgs_str;
struct _cfgs_str {
    cfgs_str  *next;
    cfgs_str  *prev;
    char      *name;
};

cfgs_str *cfgs_str_new( const char *s );
bool  cfgs_str_cat(  cfgs_str *css, const char *s );
void  cfgs_str_free( cfgs_str *css );


typedef struct _cfgs_pair cfgs_pair;
struct _cfgs_pair {
    cfgs_pair  *next;
    cfgs_pair  *prev;
    char       *first;
    char       *second;
};

cfgs_pair *cfgs_pair_new( const char *f, const char *s );
void   cfgs_pair_free( cfgs_pair *css );
/* duplicates attribs list */
cfgs_pair *cfgs_pair_dup( cfgs_pair *l );


typedef struct _cfgs_buf cfgs_buf;
struct _cfgs_buf {
    cfgs_buf  *next;
    cfgs_buf  *prev;
    long   len;
    long   used;
    char   *buf;
};

cfgs_buf *cfgs_buf_new( const char *s, long len );
void  cfgs_buf_free( cfgs_buf *css );
void  cfgs_buf_reset( cfgs_buf *css );
bool  cfgs_buf_realloc( cfgs_buf *css, int len );
bool  cfgs_buf_cat( cfgs_buf *css, const char *s, long len );
bool  cfgs_buf_cat_str( cfgs_buf *css, const char *s );
bool  cfgs_buf_cat_ch( cfgs_buf *css, char ch );


/** See cfgs_tags.h for a list of pre-defined attributes */ 
typedef struct _cfgs_tag cfgs_tag;
struct _cfgs_tag {
    cfgs_tag  *next;
    cfgs_tag  *prev;
    char      *type;
    cfgs_pair *attr;
    cfgs_hash *attr_hash; 
};

cfgs_tag *cfgs_tag_new( const char *type );
void     cfgs_tag_free( cfgs_tag *tag );
bool     cfgs_tag_add_attr( cfgs_tag* tag, const char *name, const char *val ); 
/** @return value of 'name' attribute */
char     *cfgs_tag_attr( cfgs_tag* tag, const char *name );
int      cfgs_tags_write( int file, cfgs_tag *tag );
cfgs_tag *cfgs_tags_read( int file );
/** 
 * Trsf @param tag list to xml.  
 * @return is NULL terminated.  cfgs_str_free the returned pointer. 
 */ 
cfgs_buf *cfgs_tags_to_cfgs_buf( cfgs_tag *tag );
/** One tag to xml */
bool     cfgs_tag_to_cfgs_buf( cfgs_tag *tag, cfgs_buf *buf ); 
/**
 * @param buf should contain well formatted tags, i.e. start with '<' and 
 * end with '>' in balanced pairs.  @param buf has @param len bytes.  
 */
cfgs_tag *cfgs_tags_from_str( const char *buf, int len );



#ifdef __cplusplus
}
#endif

#endif /*CSSTRINGLIST_H*/
 
