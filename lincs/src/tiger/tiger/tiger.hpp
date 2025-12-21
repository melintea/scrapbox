
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
 
#ifndef TIGER_HPP
#define TIGER_HPP


#include <lincs.hpp>


namespace lincs {

  namespace tiger {
  using namespace lincs;
  
      /*
       * Dialogs 
       */
      class IUI;
      class IDlg
      {
          protected:
              bool _visible;
	          bool _changed;
		      IUI  *_theManager;
          public:
              virtual void Show( void ) 
	              { _visible = true; if (_changed) Refresh(); };
              virtual void Hide( void ) { _visible = false; };
              virtual void Refresh( void ) { if (!_visible) return; }
      }; 
      
      // Container of other dialogs
      class IMainFrame : public IDlg, public ICommandHandler
      {
          protected:
              int    _argc;
              char   **_argv;
              char   **_envp;
          public:
    	      IMainFrame( ICommandHandler *parent, int argc, char **argv, char **envp ) :
	              ICommandHandler::ICommandHandler(parent), 
		          _argc(argc), _argv(argv), _envp(envp) {};
              //FIXME: def constr, copy constr, assign op, destr
		      virtual bool Run( void ) = 0;
		      virtual bool Shutdown( void ) = 0;
      };
      
      class IEntryDlg : public IDlg, public ICommandHandler
      {
          public:
    	      IEntryDlg( ICommandHandler *parent );
              //FIXME: def constr, copy constr, assign op, destr
		      
              virtual bool SetLayer( const char *name ); //uses _layer
	          virtual bool GetLayer( const char *name );
		      virtual bool RemoveLayer( const char *name );
	          virtual bool SetValue( const char *name ); 
	          virtual bool GetValue( const char *name ); //uses _value
		      virtual bool RemoveValue( const char *name );
		      virtual int  Navigate( const char *layer, const char *value ); 
		  
		  protected:
		      value      _value;
		      layer      _layer;
		      navigator  _navigator;
      };
      
      class IPolicyDlg : public IDlg, public ICommandHandler
      {
          public:
    	      IPolicyDlg( ICommandHandler *parent ) :
	              ICommandHandler::ICommandHandler(parent) {};
              //FIXME: def constr, copy constr, assign op, destr
      };

      class IInfosDlg : public IDlg, public ICommandHandler
      {
          public:
    	      IInfosDlg( ICommandHandler *parent ) :
	              ICommandHandler::ICommandHandler(parent) {};
              virtual bool GetInfos( void ); //uses _infos
              //FIXME: def constr, copy constr, assign op, destr
          protected: 
              string _infos;
      };

      class ILoginDlg : public IDlg, public ICommandHandler
      {
          public:
    	      ILoginDlg( ICommandHandler *parent ) :
	              ICommandHandler::ICommandHandler(parent) {};
              //FIXME: def constr, copy constr, assign op, destr
      };

      /*
       *  UI 
       */
      class IMainFrame;
      class IEntryDlg;
      class IPolicyDlg;
      class IInfosDlg;
      class ILoginDlg;
      class IUI : public ICommandHandler
      {
          protected:
	          IMainFrame  *_mainFrame;
	          IEntryDlg   *_entryDlg;
	          IPolicyDlg  *_policyDlg;
    		  IInfosDlg   *_infosDlg;
    		  ILoginDlg   *_loginDlg;
              int    _argc;
              char   **_argv;
              char   **_envp;
	      
	          virtual IMainFrame *createMainFrame( void )  = 0;
		      // Create and register with _mainFrame
	          virtual IEntryDlg  *createEntryDlg( void )  = 0;
	          virtual IPolicyDlg *createPolicyDlg( void ) = 0;
	          virtual IInfosDlg  *createInfosDlg( void )  = 0;
	          virtual ILoginDlg  *createLoginDlg( void )  = 0;
		  
		  public:
    	      IUI( ICommandHandler *parent, int argc, char **argv, char **envp ) :
	              ICommandHandler::ICommandHandler(parent),
		          _argc(argc), _argv(argv), _envp(envp) 
			  {
			  }
              //FIXME: def constr, copy constr, assign op
		      
		      virtual bool Init( void )
		      {
		          _mainFrame = createMainFrame();
		          _entryDlg  = createEntryDlg();
		          _policyDlg = createPolicyDlg();
		          _infosDlg  = createInfosDlg();
		          _loginDlg  = createLoginDlg();
			      if ( _mainFrame && _entryDlg && _infosDlg && _policyDlg && _loginDlg )
			          return true;
				  
				  return false;
		      }
		      
		      virtual bool Run( void ) { _loginDlg->Show(); };
   		      virtual bool Shutdown( void ) = 0;
		      
		      // Display a message to the user
	          virtual void Message( const char *msg, const char *title="Message" )  = 0;
		      
		      virtual ~IUI() 
		      { 
		          delete _entryDlg; delete _policyDlg; 
			      delete _infosDlg; delete _loginDlg; 
			  }
      }; 
      
  } //tiger

} //lincs


#endif //TIGER_HPP

