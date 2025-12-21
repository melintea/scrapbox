
/*
 *  $Revision: 1.9 $
 *  $Date: 2004/06/08 16:34:46 $
 *
 *  \brief Value/layer/scheme (entry) manipulations
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

#ifndef VALUE_H
#define VALUE_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cfgs/cfgs_config.h"

#include <sys/types.h>
#include <sys/socket.h>  /* struct ucred*/

#include "cfgs_str.h"
#include "cfgs_hash.h" 
#include "cfgs_error.h"
#include "cfgs_tags.h"


/*FIXME: not used*/
union _value {
    unsigned char  *s;
    signed short   ss;
    unsigned short us;
    signed long    sl;
    unsigned long  ul;
};
typedef union _value value;


/** 
 * \enum CFGS_ET
 * An entry is in one of those cathegories.  entry_type attribute.  
 */
#define CFGS_ENTRY_TYPES \
    X( CFGS_ET_VALUE,   "value" ) \
    X( CFGS_ET_SCHEME,  "scheme" ) \
    X( CFGS_ET_LAYER,   "layer" ) \
    /*X( CFGS_ET_META,    "??" )*/
    /* Increment CFGS_ET_VERSION if you add/remove or change order.*/
#define CFGS_ET_VERSION (1)
#define CFGS_ET_NONE  (-1) 

#define X(a,b)  a,
typedef enum {
    CFGS_ENTRY_TYPES
} CFGS_ET;
#undef X


/** 
 * \enum CFGS_VT
 * value_type attribute.  Do not confound with entry_type. 
 */
#define CFGS_VALUE_TYPES \
    X( CFGS_VT_UCPTR,   "8bit_string" ) \
    X( CFGS_VT_SSHORT,  "signed16" ) \
    X( CFGS_VT_USHORT,  "unsigned16" ) \
    X( CFGS_VT_SLONG,   "signed32" ) \
    X( CFGS_VT_ULONG,   "unsigned32" ) \
    /* Increment CFGS_VT_VERSION each time you add/remove or change order.  */
#define CFGS_VT_VERSION (1)
        
#define X(a,b)  a,
typedef enum {
    CFGS_VALUE_TYPES
} CFGS_VT;
#undef X


/** 
 * Default layers and their priorities, etc.
 */
#define CFGS_DEFAULT_LAYERS \
    X( "lincs",   1 ) \
    X( "user",    20 ) \
    X( "tests",   50 ) /**/ \
    X( "system",  100 ) \
    /**/


typedef struct _cfgs_entry cfgs_entry;

/**  
 *  Attribute 'name' is absolute, ex:  '/network/eth0/ip/1' .  
 *  If NULL or empty string we have an error.  
 *  See cfgs_tags.h for attributes names.  
 *  Eventually, layers will live within other layers, etc.  
 */
struct _cfgs_entry {
    cfgs_entry  *next;
    cfgs_entry  *prev;

    /* long, string, etc */
    CFGS_VT value_type;
    /*value   val;*/ /*FIXME: not used*/
    
    CFGS_ET entry_type;

    cfgs_pair  *attr; 
    cfgs_hash  *attr_hash; 
};


cfgs_entry *cfgs_entry_new( void );
void       cfgs_entry_free( cfgs_entry* );
/** Free returned pointer when done */ 
cfgs_entry *cfgs_entry_from_tag( cfgs_tag *tag ); 
/**
 * @param tag is a parse of the xml tag into a cfgs_tag structure:  
 * &lt;cfgs:entry name=".." value=".."/&gt; 
 * Free returned pointer when done. 
 */
cfgs_entry *cfgs_entries_from_tags( cfgs_tag *tag );
cfgs_tag   *cs_tags_from_entries( cfgs_entry *val );
/* Used by namespace navigation */
cfgs_tag   *cs_subkeytags_from_strings( cfgs_str *str ); 
cfgs_str   *cfgs_str_from_tag( cfgs_tag *t, const char *tagname ); 

/** Note: @param attr_name cannot contain spaces - those are replaced by '_'.  FIXME:ret err */
bool       cfgs_entry_add_attr( cfgs_entry* v, const char *attr_name, const char *attr_val );
/** @return value of attribute 'name' */
char       *cfgs_entry_attr( cfgs_entry *v, const char *name );


/* FIXME: could not find nobody value on Linux.  Use getpwnam? */
#define CFGS_UID_NOBODY  (-2)  /* canonically "nobody" */
#define CFGS_GID_NOBODY  (-2)  

typedef struct _cfgs_session cfgs_session;

cfgs_session *cfgs_session_new( void );
void         cfgs_session_free( cfgs_session* );
void         cfgs_session_store_error( 
                cfgs_session* s, 
                CFGS_ERRT     type, 
                CFGS_ERR      errcode,
                const char    *custom_info,
                ... );
cfgs_err     *cfgs_session_geterr( cfgs_session* );
/** @return error in an xml format ( the cfgs_err strerror buffer) */
const char   *cfgs_session_xml_err( cfgs_session* s ); 

struct ucred *cfgs_session_get_creds( cfgs_session* s ); 
void         cfgs_session_set_creds( cfgs_session* s, struct ucred *creds ); 



typedef enum {
    CSNT_LOCAL = 0,
    CSNT_REMOTE,
} CSNT;

/** \struct _cfgs_notif
 *  Register for notification: local process 'pid' will 
 *  receive signal if any value under 'valname' changes. 
 *  Remote process on 'host' will be contacted by connecting to 'port'.  
 */
typedef struct _cfgs_notif cfgs_notif; 
struct _cfgs_notif {
    cfgs_notif  *next;
    cfgs_notif  *prev;
    /**< Full/pattern name of the entry we register for notification */
    char        *valname; 
    CSNT        type;
    /* local processes */
    pid_t       pid;
    int         signal;
    /* remote */
    char        *host;
    int         port;
};

cfgs_notif *cfgs_notif_local_new( const char *val, pid_t pid, int sig );
cfgs_notif *cfgs_notif_remote_new( const char *val, const char *host, int port ); 
void       cfgs_notif_free( cfgs_notif* n );
cfgs_notif *cfgs_notif_from_tag( cfgs_tag *t ); 
cfgs_notif *cfgs_notifs_from_tags( cfgs_tag *tag ); 
cfgs_tag   *cs_tags_from_notifs( cfgs_notif *n );


typedef struct _cfgs_stats {
    int dummy;
} cfgs_stats;

cfgs_stats *cfgs_stats_new( void );
void       cfgs_stats_free( cfgs_stats* n );



#ifdef __cplusplus
}
#endif

#endif /*VALUE_H*/

