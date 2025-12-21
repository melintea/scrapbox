
/*
 *  $Revision: 1.4 $
 *  $Date: 2004/03/30 21:00:59 $
 *
 *  Client API
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

#ifndef APIC_H
#define APIC_H


#ifdef __cplusplus
extern "C" {
#endif

#include "cfgs_val.h"
#include "cfgs_str.h"
#include "cfgs_error.h"

/**
 *  Exports: keep in sync with backend structures and
 *  with protocol structures.  See CFGS_CRT_REV notes.  
 */
#ifdef X
#  error X already defined !
#endif
/*     index        API func */
#define CFGS_API_EXPORTS \
    X( CFGS_GETVAL, cfgs_getval )  \
    X( CFGS_SETVAL, cfgs_setval )  \
    X( CFGS_RMVAL,  cfgs_rmval )   \
    X( CFGS_REG_NOTIF,     cfgs_register_notif )   \
    X( CFGS_GETSUBVALS,    cfgs_getsubvals )   /* key/layer namespace navigation */ \
    X( CFGS_GETSUBLAYERS,  cfgs_getsublayers )   \
    X( CFGS_GETINFOS,      cfgs_getinfos )   \
    /**/
/**
 *  \def CFGS_CRT_REV
 *  Increment it each time CFGS_API_EXPORTS changes and inspect
 *  code where compiler fails.  
 */
#define CFGS_CRT_REV      4
/* increment when API changes */
#define CFGS_API_VERSION  "1.0"



/* FIXME sécurité: a définir modalités d'authentification: c'est quoi credentials/session */
cfgs_session *cfgs_connect( void );
bool         cfgs_disconnect( cfgs_session *s );


/** 
 * @return a list of entries or NULL
 * Root value: "/" .  
 * Free the list after usage.  
 * If attribute 'layer' is NULL, it defaults to CFGS_DEFAULT_LAYER.  
 */
cfgs_entry *cfgs_getval( cfgs_session *s, const char *name, const char *layer );

/** 
 * Set value(s) - @param v is a chained list. @return number of stored values or
 * -1 on error. 
 */
int cfgs_setval( cfgs_session *s, cfgs_entry *v );
/**
 * @return number of deleted values or -1 on error. 
 */
int cfgs_rmval( cfgs_session *s, const char *name, const char *layer );



/*  FIXME: on peut se debarasser des fonctions ...layer et ...scheme.  En fin de 
 *  compte, ce sont des valeurs gerees par le systeme et on peut en avoir trois
 *  "espaces de noms" (namespaces): /values, /schemes, /layers.  La politique de
 *  gestion des layers/schemes versus valeurs a isoler dans un fichier de regles
 *  (parseur special?)
 */
 
cfgs_entry *cfgs_getlayer( cfgs_session *s, const char *name );
bool       cfgs_setlayer( cfgs_session *s, cfgs_entry *l );
bool       cfgs_rmlayer( cfgs_session *s, const char *name );


/**
 *  If valname is not null, get the scheme for valname, otherwise use
 *  scheme to look for the proper cfgs_scheme.  
 */
cfgs_entry  *cfgs_getscheme( cfgs_session *ss, const char *valname, const char *scheme );
bool        cfgs_setscheme( cfgs_session *ss, cfgs_entry *s );
bool        cfgs_rmscheme( cfgs_session *ss, const char *name );


/**
 *  Navigate in the value's/layer's tree.  Get first level child keys (i.e. dirs).  
 */
cfgs_str *cfgs_getsubvals( cfgs_session *s, const char *valname, const char *layer );
cfgs_str *cfgs_getsublayers( cfgs_session *s, const char *layername );

/**
 *  Gather informations about LinCS.  
 */
cfgs_str *cfgs_getinfos( cfgs_session *s );


/* 
 * Explication + error code. 
 */
const cfgs_err *cfgs_geterror( cfgs_session *s );



/* 
 * Returns a list of available backends.  Must free it.  
 * FIXME: cfgs_session *s 
 */
cfgs_str   *cfgs_get_backends( void );

/* FIXME: registering for notifs should be done without a session? */ 
/**
 * Returns the number of notification requests successfully added to the list
 * or -1 if error on client' side.  
 */
int     cfgs_register_notif( cfgs_session *s, cfgs_notif *notif );

/* free returned pointer */
cfgs_stats *cs_getstats( cfgs_session *s );



#ifdef __cplusplus
}
#endif

#endif /*APIC_H*/


