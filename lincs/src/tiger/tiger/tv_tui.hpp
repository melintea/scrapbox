
/**
 *  $Revision: 1.19 $
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

#ifndef TVTUI_HPP
#define TVTUI_HPP

  
// SET: moved the standard headers before tv.h
#include <stdio.h>
#define Uses_string

#define Uses_TApplication
#define Uses_TBackground
#define Uses_TDialog
#define Uses_TDeskTop
#define Uses_TListViewer
#define Uses_TListBox
#define Uses_TInputLine
#define Uses_TScrollBar
#define Uses_TStaticText
#define Uses_MsgBox
#define Uses_TButton
#define Uses_TKeys
#define Uses_TVCodePage
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TView
#define Uses_TRect
#define Uses_TStatusLine
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_MsgBox
#define Uses_fpstream
#define Uses_TEvent
#define Uses_TWindow
#define Uses_TScreen
#define Uses_IOS_BIN
#define Uses_THelpWindow
// Needed to remap the "system" menu character
#define Uses_TVCodePage
#define Uses_TStreamableClass

#include <tv.h>
using namespace std;

#include "tiger/lincs.hpp"
#include "tiger/tiger.hpp"
using namespace lincs;
using namespace lincs::tiger;


#define HELP_FILENAME "tiger.h32"

/*
 *
 */
const int cmAboutCmd    = 100;
const int cmLoginDlg    = 101;
const int cmEntryDlg    = 102;
const int cmPolicyDlg   = 103;
const int cmInfosDlg    = 104;
const int cmOpenCmd     = 105;
const int cmChDirCmd    = 106;
// SET: Use cmCallShell instead
//const int cmDOS_Cmd     = 107;
const int cmCommitLayer = 108;
const int cmCommitValue = 109;
//
const int cmAddLayer    = 110;
const int cmDeleteLayer = 111;
const int cmRenameLayer = 112;
const int cmGetLayer    = 113;
const int cmSetLayer    = 114;
const int cmAddValue    = 115;
const int cmDeleteValue = 116;
const int cmRenameValue = 117;
const int cmGetValue    = 118;
const int cmSetValue    = 119;


const int
  hcEntryDlg             = 6,
  hcInfosDlg             = 4,
  hcCancelBtn            = 35,
  hcFCChDirDBox          = 37,
  hcFChangeDir           = 15,
  hcFDosShell            = 16,
  hcFExit                = 17,
  hcFOFileOpenDBox       = 31,
  hcFOFiles              = 33,
  hcFOName               = 32,
  hcFOOpenBtn            = 34,
  hcFOpen                = 14,
  hcFile                 = 13,
  hcNocontext            = 0,
  hcOpenBtn              = 36,
  hcOptions              = 26,
  hcSAbout               = 8,
  hcPolicyDlg            = 11,
  hcLoginDlg             = 9,
  hcSystem               = 7,
  hcViewer               = 2,
  hcWCascade             = 22,
  hcWClose               = 25,
  hcWNext                = 23,
  hcWPrevious            = 24,
  hcWSizeMove            = 19,
  hcWTile                = 21,
  hcWZoom                = 20,
  hcWindows              = 18,
  hcAddDlg               = 38 
  ;


      // See tv_entrydlg.hpp
      //class TV_EntryDlg : public IEntryDlg, public TDialog
      
      // See tv_policydlg.hpp
      //class TV_PolicyDlg : public IPolicyDlg, public TDialog

      // See tv_infosdlg.hpp
      //class TV_InfosDlg : public IInfosDlg, public TDialog

      // See tv_logindlg.hpp
      //class TV_LoginDlg : public ILoginDlg, public TDialog


      /*
       * With TV, the app is the mainframe.  This class is a dummy.  
       */
      class TV_MainFrame : public IMainFrame
      {
          public: 
              virtual void Show( void )     {};  
              virtual void Hide( void )     {};
              virtual void Refresh( void )  {};
	          TV_MainFrame( ICommandHandler *parent, int argc, char **argv, char **envp ) : 
		           IMainFrame(parent, argc, argv, envp) {};
		      virtual bool Run( void )      {}; 
		      virtual bool Shutdown( void ) {}; 
              //FIXME: def constr, copy constr, assign op, destr
      };

            
     /*
      *
      */
      class TStatusLine;
      class TMenuBar;
      class TEvent;
      class TPalette;
      class TClockView;
      class fpstream;
      class TvTUIImplem : public TApplication, public IUI
      {
        //IUI
        protected:
              virtual IMainFrame *createMainFrame( void ); 
	          virtual IEntryDlg  *createEntryDlg( void );
	          virtual IPolicyDlg *createPolicyDlg( void );
	          virtual IInfosDlg  *createInfosDlg( void );
	          virtual ILoginDlg  *createLoginDlg( void );
		  
		public: 
		      virtual bool Run( void ); 
		      virtual bool Shutdown( void ); 
		      virtual bool Init( void ); 
	          virtual void Message( const char *msg, const char *title="Message" ); 
		      
	          TvTUIImplem( ICommandHandler *parent, int argc, char **argv, char **envp );
		      virtual ~TvTUIImplem(); 
              //FIXME: def constr, copy constr, assign op
		      
        //TApplication
        public:
            static TStatusLine *initStatusLine( TRect r );
            static TMenuBar *initMenuBar( TRect r );
            virtual void handleEvent(TEvent& Event);
            virtual void getEvent(TEvent& event);
            virtual void idle();              // Updates heap and clock views
	
        private:
            TClockView *_clock;               // Clock view
	        TRect      _rect;                 // surface

            void aboutDlgBox();               // "About" box
            void helpMessageBox( const char *msg );   
	
            void tile();                      // Tile windows
            void cascade();                   // Cascade windows
            //void mouse();                     // Mouse control dialog box
            void outOfMemory();               // For validView() function

            static uchar systemMenuIcon[];    // Menu name for the "system menu"
                                              // encoded in current code page
            static uchar osystemMenuIcon[];   // Same encoded in CP 437, used as
                                              // reference.
        public:
                                              // Previous callback in the code page chain
            static TVCodePageCallBack oldCPCallBack;
            static void cpCallBack( ushort *map ); // That's our callback
      }; 


     /*
      * Have to wrap TvTUIImplem/TApplication in a singleton for a clean exit.  
      */
      class TvTUIImplemSingleton : public IUI
      {
        //IUI
		public: 
		      virtual bool Run( void )       { return _theTUIImplem->Run(); }; 
		      virtual bool Shutdown( void )  { return _theTUIImplem->Shutdown(); }; 
		      virtual bool Init( void )      { return _theTUIImplem->Init(); };  
	          virtual void Message( const char *msg, const char *title="Message" )
		      {
		          _theTUIImplem->Message( msg, title );
		      }
		      
		      static TvTUIImplem *Instance( ICommandHandler *parent, int argc, char **argv, char **envp )
		      {
		          if ( !_theTUI ) {
			          _theTUI       = new TvTUIImplemSingleton( parent, argc, argv, envp );
				  }
				  return _theTUIImplem;
		      }
		      
		      ~TvTUIImplemSingleton()
		      {
                  TObject::CLY_destroy( dynamic_cast<TApplication*>(_theTUIImplem) );
		      }
		      
        private:
	          static TvTUIImplemSingleton   *_theTUI;
	          static TvTUIImplem            *_theTUIImplem;
		  
	          TvTUIImplemSingleton( ICommandHandler *parent, int argc, char **argv, char **envp )
		          : IUI::IUI( parent, argc, argv, envp ) 
		      {
                  TDisplay::setArgv( argc, argv, envp );
                  //TDisplay::setShowCursorEver( True );
                  TvTUIImplem::oldCPCallBack = TVCodePage::SetCallBack( TvTUIImplem::cpCallBack );
		  
   		          _theTUIImplem = new TvTUIImplem( parent, argc, argv, envp );
		      }
		      
              virtual IMainFrame *createMainFrame( void ) {}; 
	          virtual IEntryDlg  *createEntryDlg( void )  {};
	          virtual IPolicyDlg *createPolicyDlg( void ) {};
	          virtual IInfosDlg  *createInfosDlg( void )  {};
	          virtual ILoginDlg  *createLoginDlg( void )  {};
              //FIXME: def constr, copy constr, assign op
      };
		      

    class TClockView : public TView
    {
    public:
        TClockView( TRect& r );
        virtual void draw();
        virtual void update();

    private:
        char lastTime[9];
        char curTime[9];

    };


#endif //TVTUI_HPP

