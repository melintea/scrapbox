
/**
 *  $Revision: 1.2 $
 *  $Date: 2004/03/17 19:19:23 $
 *
 *  \file 
 *  \brief Documentation only file.
 *
 *  This file exists only to have a place to hold documentation on various topics. 
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
 
/**
   \mainpage 
 
 
   <h3 align="center"> A novel configuration system  </h3>  
   
   <p>This is the API documentation, for any other documents, see 
       <a href="../index.html">the main documentation page</a> 
       or <a href="/">the site</a> .  
 
   You can start by reading the <i>cfgs_config.h</i> and 
   <i>cfgs_client_api.h</i> header.  
 
   <p><b>Notes: </b>
   <ul>
       <li> The <i>xmalloc/xfree</i> family of function might return NULL 
            pointers - you have to explicitely check.  All other functions that 
            return a pointer, 
            because  internally using those memory functions might return NULL 
            too.  The reason is that the daemon is using the tool library and 
            we cannot simply abort, as a normal application would do. 
            See the RANDOM_NULL define below. </li>
       <li> Lists are heavily used.  This gives flexibility but also extensively
            uses the <i>xmalloc/xfree</i> mechanism.  Always check for NULL.  </li>
       <li> Since the message passing protocol and <i>cfgs_fs_bk</i> are xml based, 
            lists of <i>cfgs_tag</i> are heavily used.  </li> 
       <li> If using mutexes, use the cfgs_mutex.h API.  </li>
   </ul>
 
   The code is structured into the following <b>modules</b>
   <ul>
       <li> Client API library: <i>libcsc</i> under folder <i>lib_cfgs_client/</i> </li>
       <li> /etc file emulation library: <i>cfgs_emul</i>  under folder <i>lib_cfgs_emul/</i> </li>
       <li> Tools library, used by all other modules, except the emulation, 
            <i>libcst</i> under folder <i>lib_cfgs_tools/</i> </li>
       <li> The <i>cfgs_configd</i> daemon  under folder <i>daemon/</i> </li>
       <li> Executables: </li>
            <ul>
                <li> The command line <i>cfgs_tool</i> to manupulate entries, 
                     under folder <i>clients/</i> </li>
                <li> The command line <i>cfgs_info</i> to get infos about  
                     the config system itself.  
                     Under folder <i>clients/</i> </li>
                <li> Command line <i>cfgs_cat</i> to extract on the fly <i>/etc</i>  
                     files, under folder <i>clients/</i> </li>
            </ul>
       <li> Backends: </li>
            <ul>
                <li> <i>cfgs_stacker</i> </li>
                <li> <i>cfgs_fs_bk</i>: handling file system based repositories. </li>
            </ul>
       <li> An editor ?? </li>
   </ul>
 
   <b>defines</b>
   <ul>
       <li> <i>RANDOM_NULL</i> for testing purposes <i>xmalloc/</i> & co.
            return NULL once in a while.  Check also FAIL_FREQ. </li>
   </ul>
   
 */

