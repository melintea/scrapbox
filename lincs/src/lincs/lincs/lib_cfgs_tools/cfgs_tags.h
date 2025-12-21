
/*
 *  $Revision: 1.5 $
 *  $Date: 2004/03/31 21:20:42 $
 *
 *  
 *  Tags and attributes, used by protocol, text parsing, etc. 
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

#ifndef CFGS_TAGS_H
#define CFGS_TAGS_H

#ifdef __cplusplus
extern "C" {
#endif


/** protocol tags */
#define CFGS_TAG_CFGS        "cfgs"
#define CFGS_TAG_FUNC_CALL   "cfgs:function_call"
#define CFGS_TAG_ENTRY       "cfgs:entry"
#define CFGS_TAG_ERROR       "cfgs:error"
#define CFGS_TAG_NOTIF_REG   "cfgs:notif_register"
#define CFGS_TAG_NOTIF_UNREG "cfgs:notif_deregister"
#define CFGS_TAG_NOTIF       "cfgs:notif"
#define CFGS_TAG_CALL_RETURN "cfgs:call_return"
#define CFGS_TAG_SUBKEY      "cfgs:subkey" /* namespace navigation */
#define CFGS_TAG_INFOS       "cfgs:infos"
/** namespace navigation */
#define CFGS_EA_FCALL_SUBVALS     "cfgs_getsubvals"    /*CFGS_TAG_FUNC_CALL*/
#define CFGS_EA_FCALL_SUBLAYERS   "cfgs_getsublayers"  /*CFGS_TAG_FUNC_CALL*/
#define CFGS_TAG_STR              "cfgs:cfgs_str" /*returned, with CFGS_EA_VALUE attribute*/
/** cfgs attributes */
#define CFGS_EA_VERSION     "protocol_version"
/** cfgs:entry attributes */
#define CFGS_EA_NAME        "name"
#define CFGS_EA_LAYER       "layer"
#define CFGS_EA_SCHEME      "scheme"
#define CFGS_EA_VALUE       "value"
#define CFGS_EA_ENTRY_TYPE  "entry_type"
#define CFGS_EA_VALUE_TYPE  "value_type"
/** cfgs:error attributes */
#define CFGS_EA_ERR_TYPE         "type"
#define CFGS_EA_ERR_EXPLANATION  "explanation"
#define CFGS_EA_ERR_CODE         "code"
#define CFGS_EA_ERR_STRERROR     "strerror"
#define CFGS_EA_ERR_INFO         "extra_info"
/** cfgs notif attributes */
#define CFGS_EA_PID           "pid"
#define CFGS_EA_SIGNAL        "signal"
#define CFGS_EA_IP            "ip"
#define CFGS_EA_HOST          CFGS_EA_IP
#define CFGS_EA_PORT          "port"
#define CFGS_EA_NOTIF_TYPE    "type"
#define CFGS_EA_ON_CHANGE_VAL "on_change_value"
/** other tag attributes */
#define CFGS_TA_FUNCTION    "function"



#ifdef __cplusplus
}
#endif

#endif /*CFGS_TAGS_H*/

