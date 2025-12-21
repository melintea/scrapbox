
/*
 *  $Revision: 1.6 $
 *  $Date: 2004/03/31 21:20:42 $
 *
 *  Value/layer/scheme etc. manipulations
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

#include "cfgs_log.h"
#include "cfgs_val.h"
#include "cfgs_mem.h"
#include "cfgs_dlist.h"
#include "cfgs_error.h"
#include "cfgs_tags.h"


#if CFGS_VT_VERSION != 1
#  error please inspect the table below
#endif
#define X(a,b)  b,
const char *m_value_types[] = {
    CFGS_VALUE_TYPES
};
#undef X

static const int MAX_VT = sizeof(m_value_types) / sizeof(char*);


#if CFGS_ET_VERSION != 1
#  error please inspect the table below
#endif
#define X(a,b)  b,
const char *m_entry_types[] = {
    CFGS_ENTRY_TYPES
};
#undef X

static const int MAX_ET = sizeof(m_entry_types) / sizeof(char*);



char *
cfgs_entry_attr( cfgs_entry *v, const char *name )
{
    cfgs_pair *p;
    char      *val = NULL;
    
    lassert( v && name );
    
    p = cfgs_hash_find( v->attr_hash, name );
    if ( p ) {
        lassert( 0 == strcmp(p->first, name) );
        val = p->second;
    }

    return val;
}



#define CFGS_ENTRY_HASH_SIZE  (10)

cfgs_entry * 
cfgs_entry_new( void )
{
    cfgs_entry *e = XCALLOC( cfgs_entry, 1 );
    if ( e ) {
        e->attr_hash = cfgs_hash_new( CFGS_ENTRY_HASH_SIZE );
        if ( !e->attr_hash )
            cfgs_entry_free(e), e = NULL;
    }
    
    return e;
}


void    
cfgs_entry_free( cfgs_entry *v )
{
    if ( !v )
        return; 
    CFGST_DLIST_FREE( v->attr, cfgs_pair_free );
    if ( v->attr_hash )
        cfgs_hash_free( v->attr_hash, NULL );
    xfree( v );
}


bool  
cfgs_entry_add_attr( cfgs_entry* v, const char *attr_name, const char *attr_val )
{
    cfgs_pair  *att;
    int        idx;
    const char *aval;
    char       *n;
    
    lassert( v != NULL && attr_name );
    
    if ( !v->attr_hash )
        v->attr_hash = cfgs_hash_new( CFGS_ENTRY_HASH_SIZE );
    if ( !v->attr_hash )
        return false;
    
    /* avoid inserting duplicate attributes */
    aval = cfgs_entry_attr( v, attr_name );
    if ( aval )
        return true;
    
    att = cfgs_pair_new( attr_name, attr_val );
    if ( !att )
        return false;
	
	/*FIXME: return err instead of replacing spaces */
	for ( n=att->first; *n; n++ ) {
	    if ( *n == ' ' )
	        *n = '_'; 
	}
    
    v->attr = (cfgs_pair*)cfgs_dlist_add_tail( (cfgs_dlist*)v->attr, (cfgs_dlist*)att );
    
    idx = cfgs_hash_insert( v->attr_hash, att->first, att, 1/*allow dup*/ ); 

    return true;
}


cfgs_entry *
cfgs_entry_from_tag( cfgs_tag *t )
{
    cfgs_entry *vv; 
    cfgs_pair  *p;
    
    lassert( t );
    
    /*skip non-CFGS_TAG_ENTRY tags*/
    if ( !t || !t->type || 0 != strcmp(CFGS_TAG_ENTRY, t->type) ) 
        return NULL;

    vv = cfgs_entry_new();
    if ( !vv ) {
        return NULL;
    }

    /* copy attributes */        
    for ( p=t->attr; p; p=p->next ) {
        cfgs_entry_add_attr( vv, p->first, p->second );
    }
    
    return vv;
}


cfgs_entry *
cfgs_entries_from_tags( cfgs_tag *tag )
{
    cfgs_tag   *t;
    cfgs_entry *v = NULL;
    
    for ( t=tag; t; t = t->next ) {
        cfgs_entry *vv;
    
        vv = cfgs_entry_from_tag( t );
        if ( !vv ) 
            continue;
        
        v = (cfgs_entry*)cfgs_dlist_add_tail( (cfgs_dlist*)v, (cfgs_dlist*)vv );
    }
    
    return v;
}


cfgs_tag  *
cs_tags_from_entries( cfgs_entry *val )
{
    cfgs_tag   *t = NULL;
    cfgs_entry *v; 
    
    for ( v=val; v; v=v->next ) {
        cfgs_tag  *tt = cfgs_tag_new( CFGS_TAG_ENTRY );
        if ( !tt ) {
             cfgs_tag_free( t );
             return NULL;
        }
    
        t = (cfgs_tag*)cfgs_dlist_add_tail( (cfgs_dlist*)t, (cfgs_dlist*)tt );
        
        tt->attr = cfgs_pair_dup( v->attr );
        if ( !tt->attr ) {
             cfgs_tag_free( t );
             return NULL;
        }
    } /*for*/
    
    return t;
}


cfgs_tag  *
cs_subkeytags_from_strings( cfgs_str *str )
{
    cfgs_tag   *t = NULL;
    cfgs_str   *v; 
    
    for ( v=str; v; v=v->next ) {
        cfgs_tag  *tt = cfgs_tag_new( CFGS_TAG_SUBKEY );
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


cfgs_str *
cfgs_str_from_tag( cfgs_tag *t, const char *tag )
{
    cfgs_str   *vv; 
    const char *n = NULL;
    
    lassert( t && tag );
    
    /*skip non-CFGS_TAG_SUBKEY tags*/
    if ( !t || !t->type || 0 != strcmp(tag, t->type) ) 
        return NULL;
	
	n = cfgs_tag_attr( t, CFGS_EA_NAME ); 
	if ( !n )
	    return NULL;

    vv = cfgs_str_new( n );

    return vv;
}


struct _cfgs_session {
    cfgs_err     err;
    struct ucred ucreds;  /* "client" credentials */
};

cfgs_session *
cfgs_session_new( void )
{
    cfgs_session *s =  XCALLOC( cfgs_session, 1 );
    
    if ( s ) {
        cfgs_seterr( &s->err, CFGS_ERRT_NO_ERROR, CFGS_ERR_OK, NULL ); 
        s->ucreds.uid = (uid_t)CFGS_UID_NOBODY; 
        s->ucreds.gid = (gid_t)CFGS_GID_NOBODY;  
    }
    
    return s;
}


void      
cfgs_session_free( cfgs_session* s )
{
    xfree( s );
}


void  
cfgs_session_store_error( 
        cfgs_session* s, 
        CFGS_ERRT     type, 
        CFGS_ERR      errcode,
        const char    *custom_info,
        ... )
{
    va_list    ciargs;
   
    lassert( s );
    va_start( ciargs, custom_info );
    cfgs_vseterr( &s->err, type, errcode, custom_info, ciargs );
    va_end( ciargs );
}


cfgs_err*
cfgs_session_geterr( cfgs_session* s )
{
    lassert( s );
    return &s->err;
}


const char* 
cfgs_session_xml_err( cfgs_session* s )
{
    lassert( s );
    return s->err.xml;
}


struct ucred *
cfgs_session_get_creds( cfgs_session* s )
{
    return &s->ucreds;
}


void         
cfgs_session_set_creds( cfgs_session* s, struct ucred *creds )
{
    s->ucreds.pid = creds->pid;
    s->ucreds.gid = creds->gid;
    s->ucreds.uid = creds->uid;
}


cfgs_notif *
cfgs_notif_local_new( const char *val, pid_t pid, int sig )
{
    cfgs_notif *n = XCALLOC( cfgs_notif, 1 );
    
    if ( !n )
        return NULL;
    
    n->valname = xstrdup( val );
    if ( !n->valname ) {
        cfgs_notif_free( n );
        return NULL;
    }
    
    n->type   = CSNT_LOCAL; 
    n->pid    = pid;
    n->signal = sig;
    
    return n;
}


cfgs_notif *
cfgs_notif_remote_new( const char *val, const char *host, int port )
{
    cfgs_notif *n = XCALLOC( cfgs_notif, 1 );
    
    if ( !n )
        return NULL;
    
    n->valname = xstrdup( val );
    if ( !n->valname ) {
        cfgs_notif_free( n );
        return NULL;
    }
    
    n->host = xstrdup( host );
    if ( !n->host ) {
        cfgs_notif_free( n );
        return NULL;
    }
    
    n->type   = CSNT_REMOTE; 
    n->port   = port;
    
    return n;
}


void       
cfgs_notif_free( cfgs_notif* n )
{
    if ( n->valname ) xfree( n->valname );
    if ( n->host )    xfree( n->host );
    xfree( n );
}


cfgs_notif *
cfgs_notif_from_tag( cfgs_tag *t )
{
    cfgs_notif *vv = NULL; 
    const char *att = NULL;
    const char *host=NULL, *val=NULL;
    int        port, sig, type;
    pid_t      pid;
    
    lassert( t && t->type );
    if (  !t || !t->type )
        return NULL;
    
    LOG( cfgs_log(CFGST_LL_INFO, "cfgs_notif_from_tag '%s' \n", t->type); );
    
    /*skip non-CFGS_TAG_NOTIF tags.  FIXME: should have three different funcs */
    if (  0 != strncmp(CFGS_TAG_NOTIF_UNREG, t->type, strlen(CFGS_TAG_NOTIF_UNREG)) 
       && 0 != strncmp(CFGS_TAG_NOTIF_REG, t->type, strlen(CFGS_TAG_NOTIF_REG))
       && 0 != strncmp(CFGS_TAG_NOTIF, t->type, strlen(CFGS_TAG_NOTIF))
       ) 
        return NULL;

    att = cfgs_tag_attr( t, CFGS_EA_NOTIF_TYPE );
    if ( !att ) {
        return NULL;
    }
    type = atoi( att );
    
    switch ( type ) { 
    case CSNT_LOCAL:  
        val = cfgs_tag_attr( t, CFGS_EA_VALUE );
        att = cfgs_tag_attr( t, CFGS_EA_PID );
        pid = atoi( att ); /*FIXME pid_t*/
        att = cfgs_tag_attr( t, CFGS_EA_SIGNAL );
        sig = atoi( att );
        vv = cfgs_notif_local_new( val, pid, sig );  
        break;
    case CSNT_REMOTE: 
        val  = cfgs_tag_attr( t, CFGS_EA_VALUE );
        host = cfgs_tag_attr( t, CFGS_EA_HOST );
        att  = cfgs_tag_attr( t, CFGS_EA_PORT );
        port = atoi( att );
        vv = cfgs_notif_remote_new( val, host, port ); 
        break;
    default: 
        lassert( false ); 
        vv = NULL; 
        break;
    }

    return vv;
}


cfgs_notif *
cfgs_notifs_from_tags( cfgs_tag *tag )
{
    cfgs_tag   *t;
    cfgs_notif *v = NULL;
    
    for ( t=tag; t; t = t->next ) {
        cfgs_notif *vv = NULL;
    
        LOG( cfgs_log(CFGST_LL_INFO, "cfgs_notifs_from_tags '%s'\n", t->type); ); 
        vv = cfgs_notif_from_tag( t );
        if ( !vv ) 
            continue;
        
        v = (cfgs_notif*)cfgs_dlist_add_tail( (cfgs_dlist*)v, (cfgs_dlist*)vv );
    }
    
    return v;
}

#ifndef NBUFSZ
#  define NBUFSZ (15) 
#else
error NBUFSZ already defined 
#endif
cfgs_tag   *
cs_tags_from_notifs( cfgs_notif *ns )
{
    cfgs_tag   *t = NULL;
    cfgs_notif *n; 
    char       buf[ NBUFSZ+1 ] = {0};
    
    for ( n=ns; n; n=n->next ) {
        cfgs_tag  *tt = cfgs_tag_new( CFGS_TAG_NOTIF );
        if ( !tt ) {
             cfgs_tag_free( t );
             return NULL;
        }
    
        t = (cfgs_tag*)cfgs_dlist_add_tail( (cfgs_dlist*)t, (cfgs_dlist*)tt );
        
        snprintf( buf, NBUFSZ, "%d", n->pid ); /*FIXME pid_t*/
        cfgs_tag_add_attr( tt, CFGS_EA_PID, buf );
        
        snprintf( buf, NBUFSZ, "%d", n->signal );
        cfgs_tag_add_attr( tt, CFGS_EA_SIGNAL, buf );
        
        snprintf( buf, NBUFSZ, "%d", n->type );
        cfgs_tag_add_attr( tt, CFGS_EA_NOTIF_TYPE, buf );
        
        snprintf( buf, NBUFSZ, "%d", n->port );
        cfgs_tag_add_attr( tt, CFGS_EA_PORT, buf );
        
        cfgs_tag_add_attr( tt, CFGS_EA_HOST,  SAFE_STR(n->host) );
        cfgs_tag_add_attr( tt, CFGS_EA_VALUE, SAFE_STR(n->valname) );
        
        if (  !cfgs_tag_attr(tt, CFGS_EA_PID) 
           || !cfgs_tag_attr(tt, CFGS_EA_SIGNAL) 
           || !cfgs_tag_attr(tt, CFGS_EA_NOTIF_TYPE) 
           || !cfgs_tag_attr(tt, CFGS_EA_PORT) 
           || !cfgs_tag_attr(tt, CFGS_EA_HOST) 
           || !cfgs_tag_attr(tt, CFGS_EA_VALUE) 
           ) {
             cfgs_tag_free( t );
             return NULL;
        }
    } /*for*/
    
    return t;
}



