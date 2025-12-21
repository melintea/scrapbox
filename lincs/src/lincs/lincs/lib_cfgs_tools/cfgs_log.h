
/*
 *  $Revision: 1.3 $
 *  $Date: 2004/03/19 16:36:24 $
 *
 *  \file 
 *  \brief Log tool.  
 *
 *  Set logging level with the CFGS_LOGLEVEL=0...255.  
 *  Use cfgs_test_error/is_err as a breakpoint.  
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

#ifndef CSLOG_H
#define CSLOG_H

#ifdef __cplusplus
extern "C" {
#endif


#include <assert.h>


#if 1
#  define LOG( a )    {a}
#else
#  define LOG( a )
#endif

#define SAFE( n )      n ? n : "NULL" 
#define SAFE_STR( s )  s ? s : ""


/*
 *   #define CFGST_LOG_MULTIPROCESS  (1)
 */


#ifndef _NDEBUG
#  define lassert(cond) \
    { if (!(cond)) { \
         LOG(cfgs_log(CFGST_LL_CRITIC, \
             "Failed assertion (%s/%d): "#cond"\n", __FILE__, __LINE__););  \
         assert(cond);} \
    }
#  define lwarning(cond) \
    { if (!(cond)) { \
         LOG(cfgs_log(CFGST_LL_CRITIC, \
             "Warning: (%s/%d) FAILED: "#cond"\n", __FILE__, __LINE__);); } \
    }
#else
#  define lassert(cond)  {assert(cond);}
#  define lwarning(cond) {}
#endif



typedef enum
{
    CFGST_LL_NONE = 0,
    CFGST_LL_CRITIC,
    CFGST_LL_INFO,
    CFGST_LL_DEBUG,
    CFGST_LL_ALL = 255,
} CFGST_LOGLEVEL;

/*\def CFGS_LOGLEVEL. Environment variable to set current logging level, which is
  read only at init time. Value range 0...255 */
#define CFGS_LOGLEVEL "CFGS_LOGLEVEL"


#ifndef CFGST_LL_DEFAULT
#  define CFGST_LL_DEFAULT   CFGST_LL_ALL /* CFGST_LL_NONE */
#endif
#ifndef CFGST_LOGFILE_DEFAULT
#  define CFGST_LOGFILE_DEFAULT    "lincs.log" /*FIXME*/
#endif
#ifndef CFGST_LOGFILE_DIR
#  define CFGST_LOGFILE_DIR        "/tmp"
#endif



/*
 *  Use with care, it is protected with a mutex and a semaphore, to
 *  be multithread and multiprocess safe.  Might cause deadlocks - 
 *  always embed the call within a LOG() macro.
 */
extern void cfgs_log( CFGST_LOGLEVEL level, const char* fmt, ... );
extern void cfgs_log_bin( CFGST_LOGLEVEL level, const char *msg, 
                const char* buf, long len );
extern void cfgs_close_log( void );


/** Exposed only to allow mutex table dumping.  
    @return -1 if log file not open.  */
extern int _cfgs_logfd( void );


/* FIXME: get rid of later */
void cfgs_test_error( const char *file, int line ); 
#define TEST_ERROR  cfgs_test_error( __FILE__, __LINE__ );


#ifdef __cplusplus
}
#endif

#endif /*CSLOG_H*/

