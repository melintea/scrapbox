
/*
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:19:24 $
 *
 *  \file 
 *  \brief  Send/Receive data over http
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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include "http_protocol.h"
#include "cfgs_mem.h"
#include "cfgs_log.h"
#include "cfgs_dlist.h"
#include "cfgs_str.h"
#include "cfgs_sock.h"


#define CFGSP_HEADER_SEP  "\r\n\r\n"



#define HDR_SZ  (strlen(fmt) + 16)
static char*
print_header( const char *fmt, int len )
{
    char *hdr = XCALLOC( char, HDR_SZ );

    if ( !hdr )
        return NULL;

    snprintf( hdr, HDR_SZ-1, fmt, len);
    
    return hdr;
}


static char*
print_client_header( int len )
{
    const char *fmt = 
         "POST / HTTP/1.0\r\n" 
         "User-Agent: Mozilla (" PACKAGE " " CFGS_VERSION ")\r\n" 
         "Connection: Keep-Alive\r\n"
         "Content-Type: text/xml\r\n"
         "Content-Length: %d\r\n"
         "\r\n"
         ;
    
    return print_header( fmt, len); 
}


static char*
print_server_header( int len )
{
    const char *fmt = 
         "HTTP/1.0 200 OK\r\n"
         "Content-Type: text/xml\r\n"
         "Content-Length: %d\r\n"
         "\r\n"
         ;
    
    return print_header( fmt, len); 
}


static bool
http_send( 
    int sock, 
    char *hdr, /*NULL terminated*/
    char *body, int blen, 
    cfgs_err *err )
{
    int  sent; 

    sent = cfgst_fullsend( sock, hdr, strlen(hdr), 0 );
    if ( sent != strlen(hdr) ) {
        return false;
    }

    sent = cfgst_fullsend( sock, body, blen, 0 );
    if ( sent != blen ) {
        return false;
    }
    
    return true;
}


bool   
http_client_send( int sock, char *buf, int len, cfgs_err *err )
{
    char *hdr = print_client_header( len );
    
    if ( !hdr ) {
        return false;
    }
    
    if ( !http_send(sock, hdr, buf, len, err) ) {
        xfree( hdr );
        return false;
    }

    xfree( hdr );    
    return true;
}


bool   
http_server_send( int sock, char *buf, int len, cfgs_err *err )
{
    char *hdr = print_server_header( len );
    
    if ( !hdr ) {
        return false;
    }
    
    if ( !http_send(sock, hdr, buf, len, err) ) {
        xfree( hdr );
        return false;
    }

    xfree( hdr );    
    return true;
}


/** 
 * Will read from file fd into buf until upto is encountered 
 * in the stream or CGFS_MAX_HDR_LEN bytes are in the buffer.  
 * @param buf will contain @param upto too.  
 *
 * FIXME: move to cfgs_sock
 */
static bool  
buf_read_upto( int fd, cfgs_buf *cbuf, const char *upto )
{
    bool reading = true;
    int  plen    = strlen( upto );
    char rch     = upto[ plen-1 ];
    int  no_tout = 1;
    
    /*FIXME: this is an expensive loop */
    while ( reading ) {
        char ch;
        int  len;
        
        errno = 0;
        len = cfgst_rread( fd, &ch, 1 );
        /* socket closed.  since socket has been closed, further processing is 
           useless */
        if ( len == 0 )
            return false; 
        /* No more chars to read.   */
        else if ( len < 0 && (errno == EAGAIN || errno == EINTR) ){
            LOG( cfgs_log(CFGST_LL_CRITIC, "buf_read_upto" ); );
            errno = 0;
            if ( no_tout > 0 ) {
                no_tout = cfgst_microsleep( fd, CFGST_SE_READ, CGFS_SOCK_TOUT*1000000 ); 
TEST_ERROR                
                if ( no_tout < 0 )
                    return false; 
                continue;
            } else
                return false;
        } else {
            no_tout = 1;
            if ( !cfgs_buf_cat_ch(cbuf, ch) )
                return false;

            if ( ch == rch && cbuf->used >= plen ) {
                char *ppat, *pbuf;
                int  i;
            
                /* reverse scan for pattern */
                ppat = (char*)&upto[ plen-1 ];
                pbuf = &cbuf->buf[ cbuf->used-1 ];
                reading = false;
                for ( i=plen; i>0; i--, ppat--, pbuf-- ) {
                    if ( *ppat != *pbuf ) {
                        reading = true;
                        break;
                    }
                }
            } /* if scan */
        
            if ( cbuf->used >= CGFS_MAX_HDR_LEN )
                reading = false;
        }/*if len*/
    }

    return !reading;
}


static cfgs_buf*
read_header( int sock )
{
    bool     ret;
    cfgs_buf *hdr  = cfgs_buf_new( NULL, 0 );
    
    if ( !hdr ) {
        return NULL;
    }

    ret = buf_read_upto( sock, hdr, CFGSP_HEADER_SEP );
    if ( !ret || !cfgs_buf_cat_ch(hdr, '\0') ) {
        cfgs_buf_free( hdr );
        return NULL;
    }
    
    LOG( cfgs_log(ret?CFGST_LL_INFO:CFGST_LL_CRITIC, "read_header (%d): %s\n%s", 
            hdr->used, ret?"":"invalid header", hdr->buf); );
    
    return hdr;
}


static cfgs_buf*
read_body( int sock, int len )
{
    cfgs_buf *body = cfgs_buf_new( NULL, len+1 );
    
    if ( !body ) {
        return NULL;
    }
    
    body->used += cfgst_fullrecv( sock, body->buf, len, 0 );
    /*FIXME: make it a macro NULL_BUF */
    
    if ( !cfgs_buf_cat_ch(body, '\0') ) {
        cfgs_buf_free( body );
        return NULL;
    }
    
    LOG( cfgs_log(CFGST_LL_INFO, "read_body:\n%s", body->buf); ); 
        
    return body;
}


/*
 *  Case insensitive version of strstr()
 */
static char*
stristr( const char *haystack, const char *needle )
{
    const char *h, *n;

    n = needle;
    if ( *n == 0 ) {
        return (char*)haystack;
    }
    
    for ( ; *haystack != 0; haystack++ ) {
        if ( tolower(*haystack) != tolower(*n) ) {
            continue;
        }
    
        /*possible match*/
        h = haystack;
        while ( true ) {
            if ( *n == 0 ) 
                return (char*)haystack;

            if ( tolower(*h) != tolower(*n) ) 
                break;
        
            h++;
            n++;
        }
        n = needle;
    } /*for*/
    
    return NULL;
}


static int
content_length( cfgs_buf *hdr )
{
    char *b  = stristr( hdr->buf, "Content-Length: " );
    int  len = 0;
    
    if ( b ) {
        b  += strlen( "Content-Length: " );
        len = atoi( b );
    }
    
    LOG( cfgs_log(CFGST_LL_INFO, "content_lenght: %d\n", len); ); 
    return len;
}


#if 0
/* Keep-Alive by default */
static bool
keep_alive( cfgs_buf *hdr )
{
    char *b  = stristr( hdr->buf, "Connection:" );
    
    if ( b ) {
        bool alive = false;
    
        b  += strlen( "Connection:" );
        while ( isspace(*b) ) b++;
        if ( stristr(b, "Keep-Alive") == b )
            alive = true;
        LOG( cfgs_log(CFGST_LL_INFO, "Keep Alive: %d\n", alive); ); 
        return alive;
    }
    
    return false;
}
#endif


/* FIXME: should return cfgs_tag* list ? */
cfgs_buf *
http_client_recv( int sock, cfgs_err *err )
{
    cfgs_buf *hdr  = NULL; 
    cfgs_buf *body = NULL;
    int      blen;
    
    if ( (hdr = read_header(sock)) == NULL ) {
        return NULL;
    }

    /*FIXME: accept/reject by analysing header */
    
    /* Attempt to do the max we can.  
       Should reject request as it is probably an attemt to break in ? */ 
    blen  = content_length( hdr );
    if ( blen <= 0 ) {
        cfgs_buf_free( hdr );
        return NULL;
    }
    if ( blen > CGFS_MAX_BODY_LEN )
        blen = CGFS_MAX_BODY_LEN;
    

    if ( (body = read_body(sock, blen)) == NULL ) {
        cfgs_buf_free( hdr );
        return NULL;
    }

    cfgs_buf_free( hdr );
    return body;
}


cfgs_buf *
http_server_recv( int sock, cfgs_err *err )
{
    return http_client_recv( sock, err );
}


