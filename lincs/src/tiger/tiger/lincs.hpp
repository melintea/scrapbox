
/**
 *  $Revision: 1.17 $
 *  $Date: 2004/08/13 14:23:48 $
 *
 *  \file 
 *  \brief   
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
 
#ifndef LINCS_HPP
#define LINCS_HPP

#include "cfgs/cfgs_config.h"
#include "cfgs/cfgs_log.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace lincs {

      using namespace std; 
      
      class lincs_exception : public std::exception
          {};

      class entry 
      {
          public:
        	  typedef pair<string, string>          str_pair;
	          typedef map<string, string>           attr_map;
		  
		      /*
		       * Attribute iterator. 
		       */
    	      typedef map<string, string>::iterator attr_map_iter;
	          attr_map_iter begin() { return _attribs.begin(); }
	          attr_map_iter end()   { return _attribs.end(); }
	      
	          virtual void    clear( void ) 
		      {
		          _attribs.clear();
		      }
		  
	          virtual /*size_type*/int size() const 
		      {
		          return _attribs.size();
		      }
		  
	          virtual str_pair find( const string &key ) 
		      {
		          attr_map_iter it = _attribs.find( key );
			      if ( it != _attribs.end() ) 
			          return str_pair( it->first, it->second ); 
		      }
		  
	          virtual string &getVal( const string &key ) 
		      {
		          attr_map_iter it = _attribs.find( key );
			      if ( it != _attribs.end() ) 
			          return it->second; //value
		      }
		  
	          virtual string &getAttrib( const string &key ) 
                  { return getVal(key); }
                  
	          virtual void   delAttrib( const string &key ) 
		      {
		          attr_map_iter it = _attribs.find( key );
			      if ( it != _attribs.end() ) {
			          _attribs.erase( it ); 
			      }
		      }
		      
	          virtual bool   setAttrib( const string &key, const string &val ) 
		      {
		          delAttrib( key );
		          pair<attr_map_iter, bool> ret = _attribs.insert( str_pair(key, val) ); 
			      return ret.second;
		      }
		      
		      ostream &operator<<( ostream &os )
		      {
		          dump( os );
		          return os;
		      }
		      
	          virtual void   dump( ostream &os ) 
		      {
			      for ( attr_map_iter it = _attribs.begin(); 
			            it != _attribs.end(); 
				        it++ ) {
				      os << "    key='" << it->first << "', val='"  << it->second << "' \n"; 
			      }
			      os << endl;
		      }
		      
          protected:
	          attr_map  _attribs;
      };
	
      class value : public entry 
          {};
	
      class layer : public entry 
          {};


	  /*
	   *
	   */	
	  class error 
	  {
	  public:
	      string strerr;
	      int    type;
	      int    errnum; //errno is a func
	  }; 
	   
	
	  /*
	   *
	   */	
      class ILog
      {
      public:
          virtual void log( CFGST_LOGLEVEL ll, const char *msg ) = 0;
          virtual void log( CFGST_LOGLEVEL ll, const void *buf, int size ) = 0;
      };
    
      class file_log : public ILog
      {
      public:
          virtual void log( CFGST_LOGLEVEL ll, const char *msg ) {};
          virtual void log( CFGST_LOGLEVEL ll, const void *buf, int size ) {};
      };
      
      template< typename TLog >
      class Log
      {
      public:
          static Log *Instance( void )
          {
              if ( !_theLog ) _theLog = new Log();
              return _theLog;
          }
          
          static void log( CFGST_LOGLEVEL ll, const char *msg ) 
              { Instance(); _theLogImplem->log( ll, msg ); };
          
          static void log( CFGST_LOGLEVEL ll, const void *buf, int size ) 
              { Instance(); _theLogImplem->log( ll, buf, size ); };
          
      private:
          static ILog *_theLogImplem;
          static Log *_theLog; 
          
          Log( void )
              { if ( !_theLogImplem ) _theLogImplem = new TLog(); }
          ~Log( void )
              { delete _theLogImplem; }
          //FIXME: copy constr, assign op
      };
      
	  /*
	   *
	   */
	  //FIXME: this should be integrated in the LinCS class	
      class navigator 
      {
          public:
        	  typedef vector<string>          subkeys;
              
              string  _value; //FIXME: hide
              string  _layer;
		  
		      /*
		       * subkeys iterator. 
		       */
    	      typedef vector<string>::iterator  subkeys_iter;
	          subkeys_iter begin() { return _subkeys.begin(); }
	          subkeys_iter end()   { return _subkeys.end(); }
	      
	          virtual void    clear( void ) 
		      {
		          _subkeys.clear();
		      }
		      
	          virtual void  add_subkey( const char *subk ) 
		      {
                  _subkeys.push_back( subk );
		      }
		      
              int size() { return _subkeys.size(); }
		  
          protected:
	          subkeys  _subkeys;
      };
	
      /*
       * Command & data flow
       */
      //FIXME: stupid class hierarchy, rework
      class ICommandHandler;
      class ICommand
      {
      public:
          typedef enum { 
              evGET_VALUE, evSET_VALUE, 
              evGET_LAYER, evSET_LAYER, 
              evGET_LAYER_SUBKEYS, evGET_VALUE_SUBKEYS, 
	          evPRINT_MESSAGE, evGET_INFOS,
              } cmd_type;
          cmd_type type;
          ICommandHandler *sender;
          ICommandHandler *receiver;
      };
	  
      class LincsCommand : public ICommand
      {
	  public: //FIXME
          error      err;
          entry      *e;
          navigator  *n;
	      string     *msg;
	      string     *i; //infos
      };
      
      
      class ICommandHandler
      {
          protected:
	          ICommandHandler *_upstream;
          public:
              virtual void Handle( ICommand& c ) { _upstream->Handle(c); };
	          virtual void SetHandler( ICommandHandler *h ) { _upstream = h; };
		      ICommandHandler( ICommandHandler *parent ) : _upstream(parent) {};
              //FIXME: def constr, copy constr, assign op, destr
      }; 
      
      /*
       * 
       */
      class LinCS : public ICommandHandler
      {
          private: 
              int    _argc;
              char   **_argv;
              char   **_envp;
      
	          static LinCS *_theLinCS; 
    	      LinCS( ICommandHandler *parent, int argc, char **argv, char **envp ) :
	              ICommandHandler::ICommandHandler(parent),
		          _argc(argc), _argv(argv), _envp(envp)
		      {
		      }
              //FIXME: def constr, copy constr, assign op
		      
		  public:
   		      virtual bool Init( void );

   		      virtual bool Shutdown( void )
		      {
		          return true;
		      }
		      
		      static LinCS *Instance( ICommandHandler *parent, int argc, 
                  char **argv, char **envp );
		      
              virtual void Handle( ICommand& c );
		      
		      virtual ~LinCS()
		          {}  
                  
          private: 
              void PrintMessage( const char *msg );
              void PrintMessage( const string &msg ); 
              void GetValue( ICommand& c );
              void SetValue( ICommand& c );
              void GetValueSub( ICommand& c );
              void GetLayer( ICommand& c );
              void SetLayer( ICommand& c );
              void GetLayerSub( ICommand& c );
              void GetInfos( ICommand& c );
      };
            
} //lincs


#endif //LINCS_HPP
