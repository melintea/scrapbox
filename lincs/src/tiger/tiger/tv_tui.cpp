
/**
 *  $Revision: 1.25 $
 *  $Date: 2004/03/16 20:24:41 $
 *
 *  \file 
 *  \brief   
 *
 *  Heavily inspired from the examples/demo. 
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

 
#include <cassert>

#include "tv_tui.hpp"
#include "tv_entrydlg.hpp"
#include "tv_adddlg.hpp"
#include "tv_infosdlg.hpp"
#include "tv_logindlg.hpp"
#include "tv_policydlg.hpp"


  
/*
 *
 */
TvTUIImplemSingleton *TvTUIImplemSingleton::_theTUI       = 0;
TvTUIImplem          *TvTUIImplemSingleton::_theTUIImplem = 0;


unsigned char      TvTUIImplem::systemMenuIcon[]  = "~\360~";
unsigned char      TvTUIImplem::osystemMenuIcon[] = "~\360~";
TVCodePageCallBack TvTUIImplem::oldCPCallBack     = NULL;


static unsigned short executeDialog( TDialog* pD, void* data=0 ); 


TvTUIImplem::TvTUIImplem( ICommandHandler *parent, int argc, char **argv, char **envp ) :
    TProgInit( &TvTUIImplem::initStatusLine,
               &TvTUIImplem::initMenuBar,
               &TvTUIImplem::initDeskTop ),
	IUI::IUI( parent, argc, argv, envp ) 
{
    _rect = getExtent(); 
    
    TRect r = _rect;           // Create the clock view.
    r.a.x = r.b.x - 9;      r.b.y = r.a.y + 1;
    _clock = new TClockView( r );
    insert(_clock);
}


TvTUIImplem::~TvTUIImplem()
{
}


bool TvTUIImplem::Init( void )
{
    // Have to create frame first and then dialogs
    if ( !IUI::Init() ) {
        return false;
	}
	
    // ->select(); for modeless
	//TGroup::insert( dynamic_cast<TDialog*>(_mainFrame) );
	//TGroup::insert( dynamic_cast<TDialog*>(_policyDlg) );
	//TGroup::insert( dynamic_cast<TDialog*>(_infosDlg) );
	//TGroup::insert( dynamic_cast<TDialog*>(_entryDlg) );
	//TGroup::insert( dynamic_cast<TDialog*>(_loginDlg) );
    
	return true;
}


bool TvTUIImplem::Run( void )
{
    //IUI::Run();
    executeDialog( dynamic_cast<TDialog*>(_loginDlg) );
    run(); 
    return true;
}


bool TvTUIImplem::Shutdown( void )
{
    return true;
}


static unsigned short 
executeDialog( TDialog* pD, void* data )
{
    ushort c=cmCancel;

    if ( TProgram::application->validView(pD) ) {
        if (data)
            pD->setData(data);
        c = TProgram::deskTop->execView( pD );
        if ( (c != cmCancel) && (data) )
            pD->getData(data);
    }

    return c;
}


void TvTUIImplem::Message( const char *msg, const char *title )
{
    TDialog *box = new TDialog( TRect(0, 0, 50, 16), title?title:"Message" );

    box->insert( new TStaticText( TRect(5, 2, 45, 10), msg ) );

    box->insert(
        new TButton(TRect(19, 12, 28, 14), " OK", cmOK, bfDefault) );
    box->options |= ofCentered;

    executeDialog( box );
    destroy( box );
}


//
// help messages
//
static const char help_EntryDlg[] = 
  "Move from control to control with TAB. \n"
  "Press \n"
  "- ENTER to navigate the control \n"
  "- A, a, INS to add \n"
  "- D, d, DEL to delete \n"
  "- N, n to reName \n"
  ;
void TvTUIImplem::helpMessageBox( const char *msg )
{
    Message( msg, "Help" );
}

void TvTUIImplem::getEvent( TEvent &event )
{
    TWindow   *w;
    THelpFile *hFile;
    fpstream  *helpStrm;
    static Boolean helpInUse = False;

    TApplication::getEvent( event );
    switch (event.what) {
        case evCommand:
            if ( (event.message.command == cmHelp) && ( helpInUse == False) ) {
	            ushort help_ctx = getHelpCtx();
                helpInUse = True;
		        if ( help_ctx == hcEntryDlg ) {
			        helpMessageBox( help_EntryDlg );
			    } else {
                    helpStrm = new fpstream( HELP_FILENAME, CLY_IOSIn|CLY_IOSBin );
                    hFile = new THelpFile(*helpStrm);
                    if ( !helpStrm ) {
                        messageBox( "Could not open help file", mfError | mfOKButton );
                        delete hFile;
                    } else {
                        w = new THelpWindow( hFile, getHelpCtx() );
                        if (validView(w) != 0) {
                            execView(w);
                            destroy( w );
                        }
                        clearEvent(event);
                    }
			    }
                helpInUse = False;
            }
            break;
        case evMouseDown:
            if ( event.mouse.buttons != 1 )
                event.what = evNothing;
            break;
    }
}  


//
//  Event loop to distribute the work.
//
void TvTUIImplem::handleEvent( TEvent &event )
{
    TApplication::handleEvent( event );

    if ( event.what == evCommand ) {
        switch( event.message.command ) {
            case cmAboutCmd:            //  About Dialog Box
                aboutDlgBox();
                break;
            case cmTile:             //  Tile current file windows
                tile();
                break;
            case cmCascade:          //  Cascade current file windows
                cascade();
                break;
            case cmEntryDlg: 
                executeDialog( dynamic_cast<TDialog*>(_entryDlg) );
                break;
            case cmPolicyDlg:  
                executeDialog( dynamic_cast<TDialog*>(_policyDlg) );
                break;
            case cmInfosDlg:   
                executeDialog( dynamic_cast<TDialog*>(_infosDlg) );
                break;
            case cmLoginDlg:  
                executeDialog( dynamic_cast<TDialog*>(_loginDlg) );
                break;
            case cmOpenCmd:             //  View a file
            case cmChDirCmd:            //  Change directory
            case cmCallShell:           //  DOS shell
            default:                    //  Unknown command
                return;
        }
        clearEvent( event );
    }
}


TStatusLine *TvTUIImplem::initStatusLine( TRect r )
{
    r.a.y = r.b.y - 1;

    return (new TStatusLine( r,
      *new TStatusDef( 0, 0xffff ) +
        //*new TStatusItem( 0, kbAltF3, cmClose ) +
        *new TStatusItem( "~F10~ Menu ", kbF10, cmMenu ) +
        *new TStatusItem( "~F12~ Exit",  kbF12, cmQuit ) +
        //*new TStatusItem( 0, kbF5, cmZoom ) +
        //*new TStatusItem( 0, kbCtrlF5, cmResize ) 
        *new TStatusItem( "~F5~ Login",   kbF5,  cmLoginDlg ) +
        *new TStatusItem( "~F6~ Entries", kbF6,  cmEntryDlg ) +
        *new TStatusItem( "~F7~ Policy",  kbF7,  cmPolicyDlg ) +
        *new TStatusItem( "~F8~ Infos",   kbF8,  cmInfosDlg ) +
	
        *new TStatusItem( "~F1~ Help",    kbF1,  cmHelp ) +
        *new TStatusItem( "~F11~ About",  kbF11, cmAboutCmd ) 
        )
    );
}


TMenuBar *TvTUIImplem::initMenuBar(TRect r)
{
    TSubMenu& sub1 =
      *new TSubMenu( (char *)systemMenuIcon, 0, hcSystem ) +
        *new TMenuItem( "About...",  cmAboutCmd, kbF11, hcNoContext, "F11" ); 

    TSubMenu& sub2 =
      *new TSubMenu( "~T~iger", 0, hcFile ) +
        *new TMenuItem( "~L~ogin",   cmLoginDlg, kbF5,  hcLoginDlg,  "F5" ) +
        *new TMenuItem( "~E~ntries", cmEntryDlg, kbF6,  hcEntryDlg,  "F6" ) +
        *new TMenuItem( "~P~olicy",  cmPolicyDlg,kbF7,  hcPolicyDlg, "F7" ) +
        *new TMenuItem( "~I~nfos",   cmInfosDlg, kbF8,  hcInfosDlg,  "F8" ) +
         newLine() +
        *new TMenuItem( "About...",  cmAboutCmd, kbF11, hcNoContext, "F11" ) + 
        *new TMenuItem( "Exit",      cmQuit,     kbF12, hcFExit,     "F12" );

    //TSubMenu& sub3 =
    //  *new TSubMenu( "~F~ile", 0, hcFile ) +
    //  //*new TSubMenu( "~ๆ~มสฬ", 0, hcFile ) + // KOI8 test
    //    *new TMenuItem( "~O~pen...", cmOpenCmd, kbF3, hcFOpen, "F3" ) +
    //    *new TMenuItem( "~C~hange Dir...", cmChDirCmd, kbNoKey, hcFChangeDir ) +
    //     newLine() +
    //    *new TMenuItem( "S~h~ell", cmCallShell, kbNoKey, hcFDosShell ) +
    //    *new TMenuItem( "Exit", cmQuit, kbF12, hcFExit, "F12" );

    TSubMenu& sub4 =
      *new TSubMenu( "~W~indows", 0, hcWindows ) +
        *new TMenuItem( "~R~esize/move", cmResize, kbCtrlF5, hcWSizeMove, "Ctrl-F5" ) +
        *new TMenuItem( "~Z~oom", cmZoom, kbF5, hcWZoom, "F5" ) +
        *new TMenuItem( "~N~ext", cmNext, kbF6, hcWNext, "F6" ) +
        *new TMenuItem( "~C~lose", cmClose, kbAltF3, hcWClose, "Alt-F3" ) +
        *new TMenuItem( "~T~ile", cmTile, kbNoKey, hcWTile ) +
        *new TMenuItem( "C~a~scade", cmCascade, kbNoKey, hcWCascade );

    //TSubMenu& sub4 =
    //  *new TSubMenu( "~O~ptions", 0, hcOptions ) +
    //    *new TMenuItem( "~M~ouse...", cmMouseCmd, kbNoKey, hcOMouse ) +
    //    *new TMenuItem( "~C~olors...", cmColorCmd, kbNoKey, hcOColors ) +
    //    *new TMenuItem( "~S~ave desktop", cmSaveCmd, kbNoKey, hcOSaveDesktop ) +
    //    *new TMenuItem( "~R~etrieve desktop", cmRestoreCmd, kbNoKey, hcORestoreDesktop ) +
    //    *new TMenuItem( "~T~est inputbox", cmTestInputBox, kbNoKey, hcORestoreDesktop );

    r.b.y =  r.a.y + 1;
    return ( new TMenuBar( r, sub1 + sub2 /*+ sub3*/ + sub4 ) );
}


void TvTUIImplem::aboutDlgBox()
{
    TDialog *aboutBox = new TDialog( TRect(0, 0, 50, 16), "About" );

    aboutBox->insert(
      new TStaticText( TRect(5, 2, 45, 10),
        "\003tiger\n"     // The \003 centers the line.
        "\003LinCS editor\n" 
        "\003 \n" 
        "\003GPL License\n" 
        "\003Copyright (c) 2004 Aurelian Melinte\n"  
        "\003 \n" 
        "\003TVision Copyright (c) 1994 Borland Intl\n"
        )
      );

    aboutBox->insert(
        new TButton(TRect(19, 12, 28, 14), " OK", cmOK, bfDefault) );
    aboutBox->options |= ofCentered;

    executeDialog( aboutBox );
    destroy( aboutBox );
}


// Called each time the code page changes. 
void TvTUIImplem::cpCallBack( ushort *map )
{
    TVCodePage::RemapString( systemMenuIcon, osystemMenuIcon, map );
     // If the chain was already used call it
     if ( oldCPCallBack )
        oldCPCallBack( map );
}


//
// isTileable() function ( checks a view on desktop is tileable or not )
//
static bool isTileable( TView *p, void * )
{
   if ( (p->options & ofTileable) != 0 )
       return true;
   else
       return false;
}
//
// idle() function ( updates heap and clock views for this program. )
//
void TvTUIImplem::idle()
{
    TProgram::idle();
    _clock->update();
    if ( deskTop->firstThat(isTileable, 0) != 0 ) {
        enableCommand( cmTile );
        enableCommand( cmCascade );
    } else {
        disableCommand( cmTile );
        disableCommand( cmCascade );
    }
}

void TvTUIImplem::outOfMemory()
{
    messageBox( "Not enough memory available to complete operation.",
      mfError | mfOKButton );
}


void TvTUIImplem::tile()
{
    deskTop->tile( deskTop->getExtent() );
}


void TvTUIImplem::cascade()
{
    deskTop->cascade( deskTop->getExtent() );
}


// dummy in TV
IMainFrame* TvTUIImplem::createMainFrame( void )
{
    assert( _argv && _envp );
    return new TV_MainFrame( this, _argc, _argv, _envp );
}


IEntryDlg*  TvTUIImplem::createEntryDlg( void )
{
    TRect r = getExtent();
    return new TV_EntryDlg( this, r, "Entries" );
}


IInfosDlg*  TvTUIImplem::createInfosDlg( void )
{
    TRect r = TRect(0, 0, 80, 23); //getExtent();
    return new TV_InfosDlg( this, r, "LinCS Infos" );
}


IPolicyDlg* TvTUIImplem::createPolicyDlg( void )
{
    TRect r = TRect(0, 0, 80, 23); //getExtent();
    return new TV_PolicyDlg( this, r, "Policy" );
}


ILoginDlg*  TvTUIImplem::createLoginDlg( void )
{
    TRect r = TRect(10, 3, 71, 15); //getExtent();
    //r.grow( -(r.b.x-r.a.x)/8, -(r.b.y-r.a.y)/8 );
    return new TV_LoginDlg( this, r, "Login" );
}



//
// -------------- Clock Viewer functions
//

TClockView::TClockView( TRect& r ) : TView( r )
{
    strcpy(lastTime, "        ");
    strcpy(curTime, "        ");

	/* SS: now resizing under X works well */
	growMode = gfGrowLoX | gfGrowHiX;
}


void TClockView::draw()
{
    TDrawBuffer buf;
    char c = getColor( 2 );

    buf.moveChar( 0, ' ', c, (short)size.x );
    buf.moveStr( 0, curTime, c );
    writeLine( 0, 0, (short)size.x, 1, buf );
}


void TClockView::update()
{
    time_t t = time(0);
    char *date = ctime(&t);

    date[19] = '\0';
    strcpy( curTime, &date[11] );        /* Extract time. */

    if( strcmp(lastTime, curTime) ) {
        drawView();
        strcpy( lastTime, curTime );
    }
}



