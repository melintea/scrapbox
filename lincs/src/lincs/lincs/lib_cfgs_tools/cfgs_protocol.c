
/*
 *  $Revision: 1.7 $
 *  $Date: 2004/03/31 21:20:42 $
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


#include "cfgs/cfgs_config.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "cfgs_protocol.h"
#include "http_protocol.h"
#include "cfgs_mem.h"
#include "cfgs_log.h"
#include "cfgs_dlist.h"
#include "cfgs_str.h"
#include "cfgs_sock.h"
#include "cfgs_tags.h"



/*
 * request strings
 */
#define X(a,b)  #b, 
static char *m_request_strings[] = {
    CFGS_API_EXPORTS
    NULL
};
#undef X


/*
 * requests to text (client)
 */
typedef cfgs_buf* rq_to_xml_func( va_list );
#define X(a,b)  static cfgs_buf * b##_rq_to_xml( va_list );
CFGS_API_EXPORTS
#undef X
#define X(a,b)  b##_rq_to_xml,
rq_to_xml_func *m_rq_to_xml[] = {
    CFGS_API_EXPORTS
};
#undef X
 
/*
 * request handlers (server)
 */
typedef cfgs_buf* rqh_handler_func( cfgs_tag*, CFGSP_CALLBACK*, cfgsp_data *cb_data );
#define X(a,b)  static cfgs_buf * b##_rqh_handler( cfgs_tag*, CFGSP_CALLBACK*, cfgsp_data *cb_data );
CFGS_API_EXPORTS
#undef X
#define X(a,b)  b##_rqh_handler,
rqh_handler_func *m_rqh_handler[] = {
    CFGS_API_EXPORTS
};
#undef X
 
/*
 * answers to text (server).  Will free 'in'.  
 */
typedef cfgs_buf* answer_to_xml_func( void* /*in*/ );
#define X(a,b)  static cfgs_buf * b##_answer_to_xml( void* );
CFGS_API_EXPORTS
#undef X
#define X(a,b)  b##_answer_to_xml,
answer_to_xml_func *m_answer_to_xml[] = {
    CFGS_API_EXPORTS
};
#undef X
 


cfgsp_hosting_protocol m_hosting_protocols[] = {
    /*CFGSP_HOST_PROTO_HTTP*/
    { http_client_send, http_client_recv, http_server_send, http_server_recv, }, 
};



static CFGS_FUNC_INDEX
get_rq_index( const char *name )
{
    char             **n = m_request_strings;
    CFGS_FUNC_INDEX  idx;
    
    lassert( name != NULL );
    if ( !name )
        return INVALID_CFGS_FUNC_INDEX;
    
    for ( idx=0; *n && **n; n++, idx++ ) {
        if ( 0 == strcmp(*n, name) )
            return idx;
    }
    
    return INVALID_CFGS_FUNC_INDEX;
}


#if 0
/*FIXME: delete */
bool 
cfgsp_accept_request( int sock, csprq *rq )
{
    parse_header( rq );
    
    if (  rq->rqidx == INVALID_CFGS_FUNC_INDEX 
       || !rq->protocol 
       || 0 != strcmp( rq->protocol, CFGS_PROTOCOL" "CFGS_PROTOCOL_VERSION )) {
        return false;
    }
    
    return true;
}
#endif


/*----------------------------------------------------*/

static bool
xml_header( cfgs_buf *b )
{
    lassert( b );
    if ( !b || !cfgs_buf_cat_str( b,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
            "\r\n"
            "<" CFGS_TAG_CFGS 
                " " CFGS_EA_VERSION "=\"" CFGS_PROTOCOL_VERSION "\"" 
                " xmlns:cfgs=\"/\"" 
                ">\r\n" ) 
        ) 
        return false;
        
    return true;
}


static bool
xml_footer( cfgs_buf *b )
{
    lassert( b );
    if ( !b || !cfgs_buf_cat_str( b,
            "</" CFGS_TAG_CFGS ">\r\n"
            "\r\n") )
        return false;
    if ( !cfgs_buf_cat_ch(b, '\0') ) return false;
    
    return true;
}


/* FIXME: this belongs to cfgs_str.h ? */
static bool
xml_add_attr( cfgs_buf *b, const char* name, const char *val )
{
    lassert( b && name && val );
    /* FIXME: on malloc err, should reset b so that we do not propagate invalid tags */
    if ( !cfgs_buf_cat_str(b, name) )  { return false; }
    if ( !cfgs_buf_cat_str(b, "=\"") ) { return false; }
    if ( !cfgs_buf_cat_str(b, val) )   { return false; }
    if ( !cfgs_buf_cat_str(b, "\" ") ) { return false; }
    
    return true;
}


static cfgs_buf*
cfgs_getval_rq_to_xml( va_list ap )
{
    cfgs_buf   *brq     = cfgs_buf_new( NULL, 0 );
    const char *valname = va_arg( ap, char* );
    const char *layer   = va_arg( ap, char* );

    if ( !brq ) {
        return NULL;
    }
    
    xml_header( brq );
    
    if ( !cfgs_buf_cat_str(brq, "    " "<" CFGS_TAG_FUNC_CALL " ") ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    xml_add_attr( brq, CFGS_TA_FUNCTION, m_request_strings[CFGS_GETVAL] ); 
    if ( valname )
        xml_add_attr( brq, CFGS_EA_NAME, valname );
    if ( layer )
        xml_add_attr( brq, CFGS_EA_LAYER, layer ); 
    if ( !cfgs_buf_cat_str(brq, "/>\r\n") ) {
        cfgs_buf_free( brq );
        return NULL;
    }

    xml_footer( brq );

    return brq;
}

static cfgs_buf*
cfgs_getval_rqh_handler( 
    cfgs_tag       *tag, 
    CFGSP_CALLBACK *tag_callback,
    cfgsp_data     *cb_data 
    )
{
    cfgs_entry     *pv = NULL;

    lassert( cb_data->idx == CFGS_GETVAL );
    lassert( tag != NULL );
    if ( !tag || !cb_data || !tag_callback ) 
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_getval_rqh_handler %s\n", 
            SAFE(cfgs_tag_attr(tag, CFGS_EA_NAME))); );
TEST_ERROR    
    cb_data->attribs = tag; 
    pv = (*tag_callback)( cb_data ); 
TEST_ERROR
    
    return (*m_answer_to_xml[CFGS_GETVAL])( pv );
}

/* transform entry list into a string */
static cfgs_buf*
cfgs_getval_answer_to_xml( void *in )
{
    cfgs_entry *vals = (cfgs_entry*)in;
    cfgs_buf   *ret  = NULL; 
    cfgs_tag   *tags;
    
    if ( !vals )
        return NULL;
    
    tags = cs_tags_from_entries( vals );
    if ( tags ) {
        ret = cfgs_tags_to_cfgs_buf( tags );
        CFGST_DLIST_FREE( tags, cfgs_tag_free );
    }
    
    CFGST_DLIST_FREE( vals, cfgs_entry_free );
    
    return ret;
}

/*----------------------------------------------------*/

static bool
add_entry_as_xml( cfgs_buf* brq, cfgs_entry *val )
{
    lassert( brq && val );
    
    if ( !cfgs_buf_cat_str(brq, "    " "<" CFGS_TAG_FUNC_CALL " ") ) {
        return false;
    }
    xml_add_attr( brq, CFGS_TA_FUNCTION, m_request_strings[CFGS_SETVAL] ); 
    
    if ( val ) {
        cfgs_pair  *attr;
        /*FIXME: name/vals should be checked for forbidden contest, such as <,>,& etc. */
        for ( attr=val->attr; attr; attr=attr->next ) {
            if ( attr->first && attr->second ) {
                xml_add_attr( brq, attr->first, attr->second );
            }
        }
        /*FIXME: value type CFGS_EA_VALUE_TYPE */
    }
    
    if ( !cfgs_buf_cat_str(brq, "/>\r\n") ) {
        return false;
    }
    
    return true;
}


static cfgs_buf*
cfgs_setval_rq_to_xml( va_list ap )
{
    cfgs_buf   *brq  = cfgs_buf_new( NULL, 0 );
    cfgs_entry *val  = va_arg( ap, cfgs_entry* );

    if ( !brq ) {
        return NULL;
    }
    
    xml_header( brq );
    
    for ( ; val; val=val->next ) {
        if ( !add_entry_as_xml(brq, val) ) {
            cfgs_buf_free( brq );
            return NULL;
        }
    }
    
    xml_footer( brq );

    return brq;
}

static cfgs_buf*
cfgs_setval_rqh_handler( 
    cfgs_tag       *tag, 
    CFGSP_CALLBACK *tag_callback,
    cfgsp_data     *cb_data 
    )
{
    cfgs_entry     *pv = NULL;

    lassert( cb_data->idx == CFGS_SETVAL );
    lassert( tag != NULL );
    if ( !tag || !cb_data || !tag_callback ) 
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_setval_rqh_handler %s\n", 
            SAFE(cfgs_tag_attr(tag, CFGS_EA_NAME))); );
TEST_ERROR    
    cb_data->attribs = tag; 
    pv = (*tag_callback)( cb_data ); 
TEST_ERROR
    
    return (*m_answer_to_xml[CFGS_SETVAL])( pv );
}


#define SVAL_TAG \
    "<" CFGS_TAG_CALL_RETURN " " CFGS_EA_VALUE "=\"%d\" />\r\n" 
#define SVAL_TAG_LEN   ( strlen(SVAL_TAG) )
#define SVAL_BUF_LEN   ( 128 )
static bool
xml_call_return( cfgs_buf *buf, int val )
{
    char sval[ SVAL_BUF_LEN+1 ] = {0};
    int  snret;
    
    lassert( SVAL_BUF_LEN > 2*SVAL_TAG_LEN );

    snret = snprintf( sval, SVAL_BUF_LEN, SVAL_TAG, val ); 
    lassert( snret > 0 && snret <= SVAL_BUF_LEN ); 
    if ( snret < 0 || snret > SVAL_BUF_LEN ) {
        return false; 
    }
    if ( !cfgs_buf_cat_str(buf, sval) ) { 
        return false; 
    }
    
    if ( !cfgs_buf_cat_ch(buf, '\0') ) {
        return false;
    }

    return true; 
}


static cfgs_buf*
cfgs_setval_answer_to_xml( void *in )
{
    cfgs_buf   *brq  = cfgs_buf_new( NULL, 0 );
    int        ret   = (int)in; 
    
    if ( !brq ) {
        return NULL;
    }
    
    if ( !xml_call_return(brq, ret) ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    
    return brq;
}

/*----------------------------------------------------*/

static cfgs_buf*
cfgs_rmval_rq_to_xml( va_list ap )
{
    cfgs_buf   *brq     = cfgs_buf_new( NULL, 0 );
    const char *valname = va_arg( ap, char* );
    const char *layer   = va_arg( ap, char* );

    if ( !brq ) {
        return NULL;
    }
    
    xml_header( brq );
    
    if ( !cfgs_buf_cat_str(brq, "    " "<" CFGS_TAG_FUNC_CALL " ") ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    xml_add_attr( brq, CFGS_TA_FUNCTION, m_request_strings[CFGS_RMVAL] ); 
    if ( valname )
        xml_add_attr( brq, CFGS_EA_NAME, valname );
    if ( layer )
        xml_add_attr( brq, CFGS_EA_LAYER, layer ); 
    if ( !cfgs_buf_cat_str(brq, "/>\r\n") ) {
        cfgs_buf_free( brq );
        return NULL;
    }

    xml_footer( brq );

    return brq;
}

static cfgs_buf*
cfgs_rmval_rqh_handler( 
    cfgs_tag       *tag, 
    CFGSP_CALLBACK *tag_callback,
    cfgsp_data     *cb_data 
    )
{
    cfgs_entry     *pv = NULL;

    lassert( cb_data->idx == CFGS_RMVAL );
    lassert( tag != NULL );
    if ( !tag || !cb_data || !tag_callback ) 
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_rmval_rqh_handler %s\n", 
            SAFE(cfgs_tag_attr(tag, CFGS_EA_NAME))); );
TEST_ERROR    
    cb_data->attribs = tag; 
    pv = (*tag_callback)( cb_data ); 
TEST_ERROR
    
    return (*m_answer_to_xml[CFGS_RMVAL])( pv );
}

static cfgs_buf*
cfgs_rmval_answer_to_xml( void *in )
{
    return cfgs_setval_answer_to_xml( in ); 
}

/*----------------------------------------------------*/
#ifndef NBUFSZ
#  define NBUFSZ (15) 
#else
error NBUFSZ already defined 
#endif

static cfgs_buf*
cfgs_register_notif_rq_to_xml( va_list ap )
{
    cfgs_buf         *brq     = cfgs_buf_new( NULL, 0 );
    const cfgs_notif *notif   = va_arg( ap, cfgs_notif* );
    char             buf[ NBUFSZ+1 ] = {0};

    if ( !brq ) {
        return NULL;
    }
    
    xml_header( brq );
    
    if ( !cfgs_buf_cat_str(brq, "    " "<" CFGS_TAG_NOTIF_REG " ") ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    
    snprintf( buf, NBUFSZ, "%d", notif->pid );
    xml_add_attr( brq, CFGS_EA_PID, buf );
    
    snprintf( buf, NBUFSZ, "%d", notif->signal );
    xml_add_attr( brq, CFGS_EA_SIGNAL, buf );
    
    snprintf( buf, NBUFSZ, "%d", notif->type );
    xml_add_attr( brq, CFGS_EA_NOTIF_TYPE, buf );
    
    snprintf( buf, NBUFSZ, "%d", notif->port );
    xml_add_attr( brq, CFGS_EA_PORT, buf );
    
    xml_add_attr( brq, CFGS_EA_HOST,  SAFE_STR(notif->host) );
    xml_add_attr( brq, CFGS_EA_VALUE, SAFE_STR(notif->valname) );

    if ( !cfgs_buf_cat_str(brq, "/>\r\n") ) {
        cfgs_buf_free( brq );
        return NULL;
    }

    xml_footer( brq );

    return brq;
}

/* server called */
static cfgs_buf*
cfgs_register_notif_rqh_handler( 
    cfgs_tag       *tag, 
    CFGSP_CALLBACK *tag_callback,
    cfgsp_data     *cb_data 
    )
{
    cfgs_entry     *pv = NULL;

    lassert( cb_data->idx == CFGS_REG_NOTIF );
    lassert( tag != NULL );
    if ( !tag || !cb_data || !tag_callback ) 
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_register_notif_rqh_handler %s\n", 
            SAFE(cfgs_tag_attr(tag, CFGS_EA_VALUE))); );
TEST_ERROR    
    cb_data->attribs = tag; 
    pv = (*tag_callback)( cb_data ); 
TEST_ERROR
    
    return (*m_answer_to_xml[CFGS_REG_NOTIF])( pv );
}

static cfgs_buf*
cfgs_register_notif_answer_to_xml( void *in )
{
    /* Number of notification requests added to the list*/
    return cfgs_setval_answer_to_xml( in ); 
}

/*----------------------------------------------------*/
/*FIXME navigate*/

static cfgs_buf*
cfgs_getsubvals_rq_to_xml( va_list ap )
{
    cfgs_buf   *brq     = cfgs_buf_new( NULL, 0 );
    const char *valname = va_arg( ap, char* );
    const char *layer   = va_arg( ap, char* );

    if ( !brq ) {
        return NULL;
    }
    
    xml_header( brq );
    
    if ( !cfgs_buf_cat_str(brq, "    " "<" CFGS_TAG_FUNC_CALL " ") ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    xml_add_attr( brq, CFGS_TA_FUNCTION, m_request_strings[CFGS_GETSUBVALS] ); 
    if ( valname )
        xml_add_attr( brq, CFGS_EA_NAME, valname );
    if ( layer )
        xml_add_attr( brq, CFGS_EA_LAYER, layer ); 
    if ( !cfgs_buf_cat_str(brq, "/>\r\n") ) {
        cfgs_buf_free( brq );
        return NULL;
    }

    xml_footer( brq );

    return brq;
}

/* server called */
static cfgs_buf*
cfgs_getsubvals_rqh_handler( 
    cfgs_tag       *tag, 
    CFGSP_CALLBACK *tag_callback,
    cfgsp_data     *cb_data 
    )
{
    cfgs_entry     *pv = NULL;

    lassert( cb_data->idx == CFGS_GETSUBVALS );
    lassert( tag != NULL );
    if ( !tag || !cb_data || !tag_callback ) 
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_getsubvals_rqh_handler %s %s\n", 
            SAFE(cfgs_tag_attr(tag, CFGS_EA_NAME)),
	        SAFE(cfgs_tag_attr(tag, CFGS_EA_LAYER))); );
TEST_ERROR    
    cb_data->attribs = tag; 
    pv = (*tag_callback)( cb_data ); 
TEST_ERROR
    
    return (*m_answer_to_xml[CFGS_GETSUBVALS])( pv );
}

static cfgs_buf*
cfgs_getsubvals_answer_to_xml( void *in )
{
    cfgs_str   *subs = (cfgs_str*)in;
    cfgs_buf   *ret  = NULL; 
    cfgs_tag   *tags;
    
    if ( !subs )
        return NULL;
    
    tags = cs_subkeytags_from_strings( subs );
    if ( tags ) {
        ret = cfgs_tags_to_cfgs_buf( tags );
        CFGST_DLIST_FREE( tags, cfgs_tag_free );
    }
    
    CFGST_DLIST_FREE( subs, cfgs_str_free );
    
    return ret;
}



static cfgs_buf*
cfgs_getsublayers_rq_to_xml( va_list ap )
{
    cfgs_buf   *brq     = cfgs_buf_new( NULL, 0 );
    const char *layer   = va_arg( ap, char* );

    if ( !brq ) {
        return NULL;
    }
    
    xml_header( brq );
    
    if ( !cfgs_buf_cat_str(brq, "    " "<" CFGS_TAG_FUNC_CALL " ") ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    xml_add_attr( brq, CFGS_TA_FUNCTION, m_request_strings[CFGS_GETSUBLAYERS] ); 
    if ( layer )
        xml_add_attr( brq, CFGS_EA_NAME, layer ); 
    if ( !cfgs_buf_cat_str(brq, "/>\r\n") ) {
        cfgs_buf_free( brq );
        return NULL;
    }

    xml_footer( brq );

    return brq;
}

/* server called */
static cfgs_buf*
cfgs_getsublayers_rqh_handler( 
    cfgs_tag       *tag, 
    CFGSP_CALLBACK *tag_callback,
    cfgsp_data     *cb_data 
    )
{
    cfgs_entry     *pv = NULL;

    lassert( cb_data->idx == CFGS_GETSUBLAYERS );
    lassert( tag != NULL );
    if ( !tag || !cb_data || !tag_callback ) 
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_getsublayers_rqh_handler %s \n", 
            SAFE(cfgs_tag_attr(tag, CFGS_EA_NAME))); );
TEST_ERROR    
    cb_data->attribs = tag; 
    pv = (*tag_callback)( cb_data ); 
TEST_ERROR
    
    return (*m_answer_to_xml[CFGS_GETSUBLAYERS])( pv );
}

static cfgs_buf*
cfgs_getsublayers_answer_to_xml( void *in )
{
    cfgs_str   *subs = (cfgs_str*)in;
    cfgs_buf   *ret  = NULL; 
    cfgs_tag   *tags;
    
    if ( !subs )
        return NULL;
    
    tags = cs_subkeytags_from_strings( subs );
    if ( tags ) {
        ret = cfgs_tags_to_cfgs_buf( tags );
        CFGST_DLIST_FREE( tags, cfgs_tag_free );
    }
    
    CFGST_DLIST_FREE( subs, cfgs_str_free );
    
    return ret;
}

/*----------------------------------------------------*/

static cfgs_buf* 
cfgs_getinfos_rq_to_xml( va_list ap )
{
    cfgs_buf   *brq     = cfgs_buf_new( NULL, 0 );
    const char *layer   = va_arg( ap, char* );

    if ( !brq ) {
        return NULL;
    }
    
    xml_header( brq );
    
    if ( !cfgs_buf_cat_str(brq, "    " "<" CFGS_TAG_FUNC_CALL " ") ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    xml_add_attr( brq, CFGS_TA_FUNCTION, m_request_strings[CFGS_GETINFOS] ); 
    if ( !cfgs_buf_cat_str(brq, "/>\r\n") ) {
        cfgs_buf_free( brq );
        return NULL;
    }

    xml_footer( brq );

    return brq;
}

/* server called */
static cfgs_buf*  
cfgs_getinfos_rqh_handler( 
    cfgs_tag       *tag, 
    CFGSP_CALLBACK *tag_callback,
    cfgsp_data     *cb_data 
    )
{
    cfgs_entry     *pv = NULL;

    lassert( cb_data->idx == CFGS_GETINFOS );
    lassert( tag != NULL );
    if ( !tag || !cb_data || !tag_callback ) 
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_getinfos_rqh_handler \n"); );
TEST_ERROR    
    cb_data->attribs = tag; 
    pv = (*tag_callback)( cb_data ); 
TEST_ERROR
    
    return (*m_answer_to_xml[CFGS_GETINFOS])( pv );
}

static cfgs_tag  *
cs_infos_to_tag( cfgs_str *str )
{
    cfgs_tag   *t = NULL;
    cfgs_str   *v; 
    
    for ( v=str; v; v=v->next ) {
        cfgs_tag  *tt = cfgs_tag_new( CFGS_TAG_INFOS );
        if ( !tt ) {
             cfgs_tag_free( t );
             return NULL;
        }
    
        t = (cfgs_tag*)cfgs_dlist_add_tail( (cfgs_dlist*)t, (cfgs_dlist*)tt );
	
        if ( !cfgs_tag_add_attr(tt, CFGS_EA_NAME, v->name) ) {
             cfgs_tag_free( t );
             return NULL;
        }
    } /*for*/
    
    return t;
}

static cfgs_buf*  
cfgs_getinfos_answer_to_xml( void *in )
{
    cfgs_str   *subs = (cfgs_str*)in;
    cfgs_buf   *ret  = NULL; 
    cfgs_tag   *tags;
    
    if ( !subs )
        return NULL;
    
    tags = cs_infos_to_tag( subs );
    if ( tags ) {
        ret = cfgs_tags_to_cfgs_buf( tags );
        CFGST_DLIST_FREE( tags, cfgs_tag_free );
    }
    
    CFGST_DLIST_FREE( subs, cfgs_str_free );
    
    return ret;
}

/*----------------------------------------------------*/

static int
call_ret_value( cfgs_tag *tag )
{
    const char *sval; 
    
    lassert( tag );
    if ( !tag ) {
        return -1; 
    }
    
    sval = cfgs_tag_attr( tag, CFGS_EA_VALUE );
    if ( !sval ) {
        return -1; /*FIXME: protocol error.  This tag should have value attribute */
    }

    return atoi( sval ); 
}


/* return results in *out */
static void 
client_1_0( cfgs_session *sess, cfgs_tag *tags, void **out )
{
    cfgs_tag   *t;
    cfgs_entry *ret = *(cfgs_entry**)out; 
    
    lassert( sess && out );
    if ( !sess || !out )
        return;
    
    for ( t=tags; t; t=t->next ) {
        lassert( t->type );
    
        /* get entries */
        if ( 0 == strcmp(CFGS_TAG_ENTRY, t->type) ) {
            cfgs_entry *e = cfgs_entry_from_tag( t );
            if ( !e )
                continue;
            ret = (cfgs_entry*)cfgs_dlist_add_tail( (cfgs_dlist*)ret, (cfgs_dlist*)e );
        /* Numeric return type: add them if multiple such tags */
        } else if ( 0 == strcmp(CFGS_TAG_CALL_RETURN, t->type) ) {
            int iret = call_ret_value( t );
            if ( iret <= 0 ) {
                /*FIXME: error*/
            } else {
                /*FIXME: unnatural cast if int*/
                ret = (cfgs_entry*)( (long)ret + iret );
            }
        } else if ( 0 == strcmp(CFGS_TAG_NOTIF, t->type) ) {
            /*FIXME*/
        } else if ( 0 == strcmp(CFGS_TAG_ERROR, t->type) ) {
            cfgs_err_from_tag( cfgs_session_geterr(sess), t ); 
        } else if ( 0 == strcmp(CFGS_TAG_SUBKEY, t->type) ) {
            cfgs_str *sk = cfgs_str_from_tag( t, CFGS_TAG_SUBKEY ); 
            if ( !sk )
                continue;
            ret = (cfgs_entry*)cfgs_dlist_add_tail( (cfgs_dlist*)ret, (cfgs_dlist*)sk );
        } else if ( 0 == strcmp(CFGS_TAG_INFOS, t->type) ) {
            cfgs_str *sk = cfgs_str_from_tag( t, CFGS_TAG_INFOS ); 
            ret = (cfgs_entry*)cfgs_dlist_add_tail( (cfgs_dlist*)ret, (cfgs_dlist*)sk );
        } else {
            /*unknown tag*/
            lassert( false );
        } /*tag type*/
    }/*for*/
    
    *out = ret;
}


void *
cfgsp_send_rq( 
        cfgs_session     *sess, 
        int              sock, 
        CFGSP_HOST_PROTO hproto,
        CFGS_FUNC_INDEX  idx, 
        ... )
{
    cfgs_buf   *brq = NULL;
    cfgs_buf   *txt;        /* results */
    va_list     ap;
    void       *ret  = NULL;
    cfgs_tag   *tags = NULL;
    cfgsp_hosting_protocol   proto;

    lassert( sess );    
    lassert(  hproto >= 0 
           && hproto <= sizeof(m_hosting_protocols)/sizeof(cfgsp_hosting_protocol) ); 
    proto = m_hosting_protocols[ hproto ];


    /* turn request into xml */
    va_start( ap, idx );
    brq = (*m_rq_to_xml[idx])( ap );
    va_end( ap );
    if ( !brq ) {
        /*FIXME: report err*/
        return NULL;
    }
    LOG( cfgs_log(CFGST_LL_INFO, "cfgsp_send_rq\n%s", brq->buf); );
    
    
    /* Send it */
    if ( !(proto.client_send)(sock, brq->buf, brq->len-1, cfgs_session_geterr(sess)) ) {
        cfgs_buf_free( brq );
        return NULL;
    }
    
    /* Read xml answer */
    txt = (proto.client_recv)( sock, cfgs_session_geterr(sess) );
    if ( !txt ) {
        cfgs_buf_free( brq );
        /* assume http_client_recv has set the proper error */ 
        return NULL;
    }
    tags = cfgs_tags_from_str( txt->buf, txt->len-1 );
    if ( !tags ) {
        cfgs_buf_free( brq );
        cfgs_buf_free( txt );
        /*FIXME: report err*/
        return NULL;
    }

    if ( 0 == strcmp(CFGS_TAG_CFGS, tags->type) ) {
        const char *version = cfgs_tag_attr( tags, CFGS_EA_VERSION );
    
        tags = tags->next; /* first tag is <cfgs> */
        if ( version && tags ) {
            if ( 0 == strcmp(version, CFGS_PROTOCOL_VERSION_1_0 ) ) {
                client_1_0( sess, tags, &ret ); 
            } else {
                /*unsupported version*/
            }
        } else {
            /*unknown version*/
        }
    } /*if <cfgs>*/


    CFGST_DLIST_FREE( tags, cfgs_tag_free );
    cfgs_buf_free( brq );
    cfgs_buf_free( txt );
    return ret;
}


/* process 'tags' and return results 'rez' */
static void 
server_1_0(
    cfgs_tag *tags,  cfgs_buf *rez,
    CFGSP_CALLBACK *tag_callback, cfgsp_data *cb_data 
    )
{
    lassert( rez && cb_data );
    
    for ( ; tags; tags=tags->next ) {
        cfgs_buf  *buf = NULL;
    
        lassert( tags->type );
    
        cb_data->idx = INVALID_CFGS_FUNC_INDEX;  
        if ( 0 == strcmp(CFGS_TAG_FUNC_CALL, tags->type) ) {
            const char *func = cfgs_tag_attr( tags, CFGS_TA_FUNCTION );
        
            if ( !func ) {
                break;
            } 
            cb_data->idx = get_rq_index( func );

            lassert( cb_data->idx != INVALID_CFGS_FUNC_INDEX );
            /*unknown API func*/
            if ( cb_data->idx == INVALID_CFGS_FUNC_INDEX ) {
                cfgs_session_store_error( cb_data->sess, CFGS_ERRT_INTERNAL, 
                        CFGSP_ERR_FUNC_CALL, func );
                break;
            }
TEST_ERROR            
            buf = (*m_rqh_handler[cb_data->idx])( tags, tag_callback, cb_data ); 
TEST_ERROR   /* errno 2 no such file or dir */         
        } else if ( 0 == strcmp(CFGS_TAG_NOTIF_REG, tags->type) ) {
            cb_data->idx = CFGS_REG_NOTIF; 
            /*cfgs_register_notif_rqh_handler*/
            buf = (*m_rqh_handler[CFGS_REG_NOTIF])( tags, tag_callback, cb_data ); 
        } else if ( 0 == strcmp(CFGS_TAG_NOTIF_UNREG, tags->type) ) {
            /*FIXME*/
        } else if ( 0 == strcmp(CFGS_TAG_ERROR, tags->type) ) {
            /* actually, there should be no error tag in the request */ 
            cfgs_err_from_tag( cfgs_session_geterr(cb_data->sess), tags );
        } else {
            /*unknown tag*/
            lassert( false );
        } /*tag type*/
    
        if ( buf ) {
            cfgs_buf_cat_str( rez, buf->buf );
            LOG( cfgs_log(CFGST_LL_INFO, "cfgsp_process_request (%p): \n%s\n", buf, 
                    buf&&buf->buf ? buf->buf : "NULL"); );
            cfgs_buf_free( buf );
        }
    } /*for*/
}


/** Server side.  Free pointer.  */
static cfgs_buf *
process_request( 
    cfgs_buf       *xreq,  /**< xml formatted request to be parsed */
    CFGSP_CALLBACK *tag_callback, 
    cfgsp_data     *cb_data 
    )
{
    cfgs_buf *rez  = cfgs_buf_new( NULL, 128 );
    cfgs_tag *tags; 
    
    lassert( xreq && cb_data && cb_data->sess && tag_callback ); 
    
    if ( !rez ) {
        return NULL;
        /*FIXME: report err*/
    }
TEST_ERROR    
    tags = cfgs_tags_from_str( xreq->buf, xreq->len-1 );
    if ( !tags ) {
        cfgs_buf_free( rez );
        return NULL;
    }

    xml_header( rez );
        
    if ( tags && tags->type && 0 == strcmp(CFGS_TAG_CFGS, tags->type) ) { 
        const char *version = cfgs_tag_attr( tags, CFGS_EA_VERSION );
    
        tags = tags->next; /* first tag is <cfgs> */
        if ( version ) {
            if ( 0 == strcmp(version, CFGS_PROTOCOL_VERSION_1_0 ) ) {
TEST_ERROR            
                server_1_0( tags, rez, tag_callback, cb_data ); 
TEST_ERROR                
            } else {
                cfgs_session_store_error( cb_data->sess, CFGS_ERRT_INTERNAL, 
                        CFGSP_ERR_VERSION, NULL );
            }
        } else {
            cfgs_session_store_error( cb_data->sess, CFGS_ERRT_INTERNAL, 
                    CFGSP_ERR_VERSION, NULL );
        }
    } /*if <cfgs>*/

    if ( !cfgs_buf_cat_str(rez, cfgs_session_xml_err(cb_data->sess)) ) {
        cfgs_tag_free( tags );
        cfgs_buf_free( rez );
        return NULL;
    }
    xml_footer( rez );
    
    cfgs_tag_free( tags );
    return rez;
}


/** Report err to client, close connection */
static inline void 
report_error( int sock, CFGS_ERR err )
{
    /*FIXME*/
    close( sock ); /*shutdown(,2)*/
}


bool 
cfgsp_process_rq( 
    int               csock,
    CFGSP_HOST_PROTO  hproto,
    CFGSP_CALLBACK    *tag_callback, 
    cfgsp_data        *cb_data 
    )
{
    cfgs_buf     *body = NULL;
    cfgs_buf     *results = NULL;
    bool         ret = false;
    cfgsp_hosting_protocol   proto;
    
    lassert( cb_data );
    lassert(  hproto >= 0 
           && hproto <= sizeof(m_hosting_protocols)/sizeof(cfgsp_hosting_protocol) ); 
          
    proto = m_hosting_protocols[ hproto ];
TEST_ERROR
    /*only http supported now*/ 
    body = (proto.server_recv)( csock, cfgs_session_geterr(cb_data->sess) );
    if ( errno ) {
        /* might happen because of a timeout, errnoneous content-length, etc */
        /* FIXME: report error */
        errno = 0;
    }
    if ( !body ) {
        report_error( csock, CFGSP_ERR_VERSION );
        return false;
    }
TEST_ERROR    /*errno 11*/
    
    results = process_request( body, tag_callback, cb_data ); 
    if ( !results ) {
        report_error( csock, CFGSP_ERR_SERVER );
        cfgs_buf_free( body ); 
        cfgs_buf_free( results );  
        return false;
    }
TEST_ERROR
    ret = (proto.server_send)( csock, results->buf, results->len-1, 
            cfgs_session_geterr(cb_data->sess) ); 
TEST_ERROR    
    return ret;
}

