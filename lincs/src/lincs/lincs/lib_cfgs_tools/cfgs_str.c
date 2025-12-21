
/*
 *  $Revision: 1.5 $
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

#include "cfgs/cfgs_config.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
/* read tags from file */
#include <sys/types.h>
#include <sys/stat.h>

#include "expat.h"

#include "cfgs_str.h"
#include "cfgs_mem.h"
#include "cfgs_dlist.h"
#include "cfgs_log.h"
#include "cfgs_val.h" /* for attributes */


#ifndef max
#  define max(a,b)  (((a)>(b)) ? (a) : (b))
#endif


cfgs_str *
cfgs_str_new( const char *s )
{
    cfgs_str *new = XCALLOC( cfgs_str, 1 );
    if ( !new )
        return NULL;
    
    if ( s ) new->name = xstrdup( s );
    if ( !new->name ) {
        cfgs_str_free( new );
        return NULL;
    }
    
    return new;
}


void  
cfgs_str_free( cfgs_str *css )
{
    if ( !css )
        return;
    
    xfree( css->name );
    xfree( css );
}


bool  
cfgs_str_cat(  cfgs_str *css, const char *s )
{
    lassert( css && s );
    if ( !css || !s )
        return false;
	
    css->name = XREALLOC( char, css->name, strlen(css->name)+strlen(s)+1 );
    if ( css->name ) {
        strcat( css->name, s );
        return true;
    }
    else
        return false;
}


cfgs_pair *
cfgs_pair_new( const char *f, const char *s )
{
    cfgs_pair *new = XCALLOC( cfgs_pair, 1 );
    if ( !new )
        return NULL;
    
    if ( f ) { 
        new->first  = xstrdup( f );
        if ( !new->first ) {
            cfgs_pair_free( new );
            return NULL;
        }
    }
    if ( s ) {
        new->second = xstrdup( s );
        if ( !new->second ) {
            cfgs_pair_free( new );
            return NULL;
        }
    }
    
    return new;
}


void  
cfgs_pair_free( cfgs_pair *css )
{
    if ( !css )
        return;
    
    xfree( css->first );
    xfree( css->second );
    xfree( css );
}


cfgs_pair *
cfgs_pair_dup( cfgs_pair *l )
{
    cfgs_pair *new = NULL;
    cfgs_pair *next; 
    
    if ( !l )
        return NULL;
    
    new = cfgs_pair_new( l->first, l->second );
    if ( !new )
        return NULL;
    new = (cfgs_pair*)cfgs_dlist_add_tail( NULL, (cfgs_dlist*)new );
    
    next = l;
    while ( (next = next->next) != NULL ) {
        cfgs_pair *p = cfgs_pair_new( next->first, next->second );
        if ( !p ) {
            cfgs_pair_free( new );
            return NULL;
        }
        
        new = (cfgs_pair*)cfgs_dlist_add_tail( (cfgs_dlist*)new, (cfgs_dlist*)p );
    }
    
    return new;
}


cfgs_buf *
cfgs_buf_new( const char *s, long len )
{
    cfgs_buf *new = XCALLOC( cfgs_buf, 1 );
    if ( !new )
        return NULL;
    
    if ( len > 0 ) {
        new->buf = XCALLOC( char ,len );
        if ( !new->buf ) {
            cfgs_buf_free( new );
            return NULL;
        }
        new->len  = len;
        
        if ( s ) {
            memmove( new->buf, s, len );
            new->used = len;
        }
    }
    return new;
}


void  
cfgs_buf_free( cfgs_buf *css )
{
    if ( !css )
        return;
    
    xfree( css->buf );
    xfree( css );
}


void  
cfgs_buf_reset( cfgs_buf *css )
{
    css->used = 0;
}


bool  
cfgs_buf_realloc( cfgs_buf *css, int len )
{
    css->buf = XREALLOC( char, css->buf, len );
    if ( css->buf && errno != ENOMEM ) {
        css->len = len;
        return true;
    }
    else
        return false;
}


bool  
cfgs_buf_cat( cfgs_buf *css, const char *s, long len )
{
    if ( css->len < len+css->used ) {
        if ( !cfgs_buf_realloc(css, 2*css->len+len) )
            return false;
    }
    
    if ( !css->buf )
        return false;
        
    memmove( css->buf+css->used, s, len );
    css->used += len;
    return true;
}


inline bool  
cfgs_buf_cat_str( cfgs_buf *css, const char *s )
{
    return cfgs_buf_cat( css, s, strlen(s) );
}


inline bool  
cfgs_buf_cat_ch( cfgs_buf *css, char ch )
{
    if ( css->len < css->used+1 ) {
        css->buf = XREALLOC( char, css->buf, max(1024, 2*css->len) );
        if ( !css->buf ) {
            cfgs_buf_free( css );
            return false;
        }
        css->len = max( 1024, 2*css->len );
    }

    css->buf[ css->used++ ] = ch;
    return true;
}


#define CFGS_TAG_HASH_SIZE  (10)

cfgs_tag *
cfgs_tag_new( const char *type )
{
    cfgs_tag  *t  = XCALLOC( cfgs_tag, 1 );

    if ( !t )
        return NULL;
    
    t->attr_hash = cfgs_hash_new( CFGS_TAG_HASH_SIZE );
    if ( !t->attr_hash ) {
        cfgs_tag_free(t); 
        return NULL;
    }
    
    if ( type ) {
        /*
        cfgs_tag_add_attr( t, CFGS_EA_NAME, type );
        t->type  = cfgs_tag_attr( t, CFGS_EA_NAME );
        lassert( 0 == strcmp(type, t->type) );
        */
        t->type = xstrdup( type );
        if ( !t->type ) {
            cfgs_tag_free(t); 
            return NULL;
        }
    }
    
    return t;
}


void  
cfgs_tag_free( cfgs_tag *tag )
{
    if ( !tag )
        return;
    
    CFGST_DLIST_FREE( tag->attr, cfgs_pair_free );

    if ( tag->type )
        xfree( tag->type );
    if ( tag->attr_hash )
        cfgs_hash_free( tag->attr_hash, NULL );

    xfree( tag );
}


bool  
cfgs_tag_add_attr( cfgs_tag* tag, const char *name, const char *val )
{
    int       idx;
    cfgs_pair *att; 
    
    lassert( tag && name );
    if ( !tag || !name )
        return false;
    
    att = cfgs_pair_new( name, val );
    if ( !att ) {
        return false; 
    }
    
    tag->attr = (cfgs_pair*)cfgs_dlist_add_tail( (cfgs_dlist*)tag->attr, 
                        (cfgs_dlist*)att );

    if ( !tag->attr_hash )
        tag->attr_hash = cfgs_hash_new( CFGS_TAG_HASH_SIZE );
    if ( !tag->attr_hash )
        return false;
    idx = cfgs_hash_insert( tag->attr_hash, att->first, att, 1/*allow dup*/ ); 
    
    return true;
}


char *
cfgs_tag_attr( cfgs_tag* tag, const char *name )
{
    cfgs_pair *p = cfgs_hash_find( tag->attr_hash, name );
    char      *val = NULL;
    
    if ( p ) {
        val = p->second;
    }

    return val;
}


bool 
cfgs_tag_to_cfgs_buf( cfgs_tag *t, cfgs_buf *buf )
{
    lassert( t && buf );
    
    if ( !cfgs_buf_cat(buf, "<", 1 ) ) return false;
    
    if ( t->type ) {
        cfgs_pair *att = t->attr;

        if ( !cfgs_buf_cat(buf, t->type, strlen(t->type)) ) return false;
    
        while ( att ) {
            if ( att->first && att->second ) {
                if ( !cfgs_buf_cat(buf, " ", 1) ) return false;
                if ( !cfgs_buf_cat(buf, att->first, strlen(att->first)) ) return false;
                if ( !cfgs_buf_cat(buf, "=\"", 2) ) return false;
                if ( !cfgs_buf_cat(buf, att->second, strlen(att->second)) ) return false;
                if ( !cfgs_buf_cat(buf, "\"", 1) ) return false;
            }
    
            att = att->next;
        }
    }
    if ( !cfgs_buf_cat(buf, "/>\r\n", 4) ) return false;
    
    return true;
}


cfgs_buf *
cfgs_tags_to_cfgs_buf( cfgs_tag *tag )
{
    cfgs_tag *t;
    cfgs_buf *buf = cfgs_buf_new( NULL, 0 );
    
    if ( !buf )
        return NULL;
    
    for ( t=tag; t; t=t->next ) {
        if ( !cfgs_tag_to_cfgs_buf(t, buf) ) {
            cfgs_buf_free( buf );
            return NULL;
        }
    }
    
    if ( !cfgs_buf_cat_ch(buf, '\0') ) {
        cfgs_buf_free( buf );
        return NULL;
    }
    
    return buf;
}


int   
cfgs_tags_write( int file, cfgs_tag *tag )
{
    extern int  cfgst_rwrite( int fd, const void *buf, int len ); 
    cfgs_tag *t  = tag;
    int   num = 0;
    
    while ( t ) {
        cfgs_buf *s;
    
        s = cfgs_tags_to_cfgs_buf( t );
        if ( s && s->buf ) {
            cfgst_rwrite( file, s->buf, strlen(s->buf) );
            cfgst_rwrite( file, "\r\n", 2 );
            cfgs_buf_free( s ), s = NULL;
            num++;
        }
    
        t = t->next;
    }

    return num;    
}


#define CSEOF  (int)-1


/*
 * expat code 
 */
 
cfgs_tag *
cfgs_tags_read( int file )
{
    struct stat fs; 
    int         rez;
    cfgs_buf    *buf;
    cfgs_tag    *ret;
    
    rez = fstat( file, &fs );
    if ( rez != 0 ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "cfgs_tags_read: could not stat %d\n", file); );
        return NULL;
    }
    
    buf = cfgs_buf_new( NULL, fs.st_size+1/*NULL term*/ );
    if ( !buf )
        return NULL;
    
    /* read all file into buf */
    rez = cfgst_rread( file, buf->buf, fs.st_size ); 
    lassert( rez == fs.st_size ); 
    buf->used = fs.st_size + 1;   /* FIXME: not too clean */

    ret = cfgs_tags_from_str( buf->buf, buf->used-1 );
    
    cfgs_buf_free( buf );
    return ret;
}


#define IS_STR( a, b )   (0 == strcmp(a,b))

static void
handle_elem_start(void *userData, const char *name, const char **atts)
{
    cfgs_tag   **tags = (cfgs_tag**)userData;
    cfgs_tag   *tag; 
    int        i;

    lassert( tags != NULL );
    
    if ( !name )
        return;
    
    tag = cfgs_tag_new( name );
    if ( !tag )
        return;
    
    *tags = (cfgs_tag*)cfgs_dlist_add_tail( (cfgs_dlist*)*tags, (cfgs_dlist*)tag );

    for ( i=0; atts[i]; i+=2 ) {
        cfgs_pair *p = cfgs_pair_new( atts[i], atts[i+1] );
        if ( p )
            if ( !cfgs_tag_add_attr(tag, p->first, p->second) )
                break; 
    }
}


static void
handle_elem_end(void *userData, const char *name)
{
}


void
handle_char_data( void *userData, const XML_Char *s, int len )
{
}

cfgs_tag *
cfgs_tags_from_str( const char *buf, int len )
{
    cfgs_tag   *ret   = NULL;
    XML_Parser parser = XML_ParserCreate( NULL );
    /*FIXME: one parser created for each file we manipulate*/
    
    if ( !parser ) {
        LOG( cfgs_log(CFGST_LL_CRITIC, "_cfgs_tags_from_str: XML_ParserCreate failed.\n"); );
        return NULL;
    }
    
    XML_SetElementHandler( parser, handle_elem_start, handle_elem_end );
    XML_SetCharacterDataHandler( parser, handle_char_data );
    XML_SetUserData( parser, (void*)&ret ); 
    if ( !XML_Parse(parser, buf, len, true) ) {
        cfgs_log(CFGST_LL_CRITIC, "XML_Parse error: %s at line %d \n",
              XML_ErrorString(XML_GetErrorCode(parser)),
              XML_GetCurrentLineNumber(parser) );
    }
    
    
    XML_ParserFree( parser );
    return ret;
}



