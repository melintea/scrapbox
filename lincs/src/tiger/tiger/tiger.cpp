
/**
 *  $Revision: 1.20 $
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
 

#include <stdexcept>
#include <iostream>
using namespace std;

#include "cfgs/cfgs_config.h"
#include "cfgs/cfgs_tags.h"

#include "tiger/lincs.hpp"
#include "tiger/tiger.hpp"
using namespace lincs;
using namespace lincs::tiger;

#include "tiger/tv_tui.hpp"


/*
 *
 */
template <typename UIImplem>
class Tiger : public ICommandHandler
{
  private: 
      IUI    *_theUI;
      LinCS  *_theLinCS;
      int    _argc;
      char   **_argv;
      char   **_envp;
      
	  static Tiger *_theTiger; 
      Tiger( int argc, char **argv, char **envp ) : 
          ICommandHandler::ICommandHandler(NULL), 
	      _argc(argc), _argv(argv), _envp(envp)
	  {
          _theUI    = UIImplem::Instance( this, argc, argv, envp );
          _theLinCS = LinCS::Instance( this, argc, argv, envp );
	  }
      //FIXME: def constr, copy constr, assign op
	  
  public:
	  static Tiger *Instance( int argc, char **argv, char **envp )
	  {
		  if ( !_theTiger )
			  _theTiger = new Tiger( argc, argv, envp );
			  
		  return _theTiger;
	  }
	  
      virtual bool Init()
      {
		  if ( !_theTiger ) {
	          return false;
		  }
		  
          if ( !_theUI->Init() )
	          return false;
          if ( !_theLinCS->Init() )
	          return false;
		  
          return true;
      }
			  
   	  virtual bool Shutdown( void )
	  {
	      _theLinCS->Shutdown();
	      _theUI->Shutdown();
		  return true;
	  }
		      
      virtual bool Run( void )
      {
          if ( !_theUI || !_theLinCS ) {
	          return false;
		  }
		  
	      return _theUI->Run();
      }
      
      virtual void Handle( ICommand& c ) 
      { 
          switch ( c.type ) {
          case ICommand::evGET_VALUE: case ICommand::evSET_VALUE:
          case ICommand::evGET_LAYER: case ICommand::evSET_LAYER:
          case ICommand::evGET_LAYER_SUBKEYS: case ICommand::evGET_VALUE_SUBKEYS:
          case ICommand::evGET_INFOS: 
              _theLinCS->Handle( c );
              break;
          case ICommand::evPRINT_MESSAGE: 
              _theUI->Message( static_cast<LincsCommand*>(&c)->msg->c_str() );
              break;
          default:
              throw logic_error( "Invalid event type" );
          }
      }
      
	  virtual ~Tiger() 
          {}
};


Tiger<TvTUIImplemSingleton> *Tiger<TvTUIImplemSingleton>::_theTiger = 0;



int IEntryDlg::Navigate( const char *layer, const char *value )
{
    LincsCommand lc;
    
    _navigator.clear();
    _navigator._layer = layer;
    if ( value ) {
        _navigator._value = value;
        lc.type = LincsCommand::evGET_VALUE_SUBKEYS;
    } else {
        lc.type = LincsCommand::evGET_LAYER_SUBKEYS;
    }
    lc.n    = &_navigator;
    
    _upstream->Handle( lc );
    return _navigator.size();
}

bool IEntryDlg::SetLayer( const char *name )
{
    LincsCommand lc;
    
    lc.type = LincsCommand::evSET_LAYER;
    lc.e    = &_layer;
    
    _upstream->Handle( lc );
    return true;
}

bool IEntryDlg::GetLayer( const char *name )
{
    LincsCommand lc;
    
    _layer.clear();
    _layer.setAttrib( CFGS_EA_NAME, name );
    lc.type = LincsCommand::evGET_LAYER;
    lc.e    = &_layer;
    
    _upstream->Handle( lc );
    return true;
}

bool IEntryDlg::RemoveLayer( const char *name )
{
    _theManager->Message( "FIXME IEntryDlg::removeLayer" );
    return true;
}

bool IEntryDlg::SetValue( const char *name ) 
{
    LincsCommand lc;
    
    lc.type = LincsCommand::evSET_VALUE;
    lc.e    = &_value;
    
    _upstream->Handle( lc );
    return true;
}

bool IEntryDlg::GetValue( const char *name )  
{
    LincsCommand lc;
    
    _value.clear();
    _value.setAttrib( CFGS_EA_NAME, name );
    lc.type = LincsCommand::evGET_VALUE;
    lc.e    = &_value;
    
    _upstream->Handle( lc );
    return true;
}

bool IEntryDlg::RemoveValue( const char *name )
{
    _theManager->Message( "FIXME IEntryDlg::removeValue" );
}

IEntryDlg::IEntryDlg( ICommandHandler *parent ) :
    ICommandHandler::ICommandHandler(parent) 
{
    _theManager = dynamic_cast<IUI*>( parent );
}

            
bool IInfosDlg::GetInfos( void )
{
    LincsCommand lc;
    
    _infos = "";
    lc.type = LincsCommand::evGET_INFOS;
    lc.i    = &_infos;
   
    _upstream->Handle( lc );
    return true;
}

/*
 *
 */
static void 
fatal( const char *msg )
{
    perror( msg );
    Log<file_log>::log( CFGST_LL_CRITIC, msg );
	exit( EXIT_FAILURE );
}


int
main( int argc, char **argv, char **envp )
{
    try {
        Tiger<TvTUIImplemSingleton>  *theTiger = 
	            Tiger<TvTUIImplemSingleton>::Instance( argc, argv, envp );
    
        if ( !theTiger->Init() ) {
            fatal( "theTiger->Init()\n" );
        }
	
        if ( !theTiger->Run() ) {
            fatal( "theTiger->Run()\n" );
        }

        theTiger->Shutdown();    
        return EXIT_SUCCESS;
	
    } catch( exception &e ) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    } catch( ... ) {
        cerr << "Unknown exception" << endl;
        return EXIT_FAILURE;
    }
}


