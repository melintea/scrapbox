//-------------------------------------------------------
//                       logindlg.cpp
//-------------------------------------------------------

#define Uses_TEvent

#include <tv.h>

#if !defined( __LOGINDLG_H )
#include "tv_logindlg.hpp"
#endif

/*
 *
 */
void TV_LoginDlg::Refresh( void )
{
}


TV_LoginDlg::TV_LoginDlg( ICommandHandler *parent, TRect &dims, const char *title ) : 
    ILoginDlg( parent ),
    TDialog( dims, title ),
    TWindowInit( TDialog::initFrame )
{
 TView *control;
 helpCtx = hcLoginDlg;

 control = new TStaticText(TRect(5, 3, 16, 4), "Host:");
 insert(control);

 control = new TStaticText(TRect(5, 5, 16, 6), "User:");
 insert(control);

 control = new TStaticText(TRect(5, 7, 16, 8), "Password:");
 insert(control);

 _okButton = new TButton(TRect(26, 9, 36, 11), "O~K~", cmOK, bfDefault);
 _okButton->helpCtx = hcLoginDlg;
 insert(_okButton);

 _host = new TInputLine(TRect(18, 3, 56, 4), 128);
 _host->setDataFromStr( newStr("localhost") );
 _host->helpCtx = hcLoginDlg;
 insert(_host);

   _hosts = new TStringCollection(10, 2);
   fillHosts(); 
   _comboHost = new TComboBox(TRect(56, 3, 57, 4), (TInputLine*)_host, _hosts);
   //((TComboBox*)_comboHost)->activateChar('');
   insert(_comboHost);

 _user = new TInputLine(TRect(18, 5, 57, 6), 128);
 _user->helpCtx = hcLoginDlg;
 insert(_user);

 _password = new TInputPassword(TRect(18, 7, 57, 8), 38);
 _password->helpCtx = hcLoginDlg;
 _password->options |= ofPreProcess | ofPostProcess;
 insert(_password);

 selectNext(False);
}

void TV_LoginDlg::fillHosts( void )
{
    _hosts->insert( newStr("localhost") ); //FIXME
}

void TV_LoginDlg::handleEvent( TEvent& event)
{
    TDialog::handleEvent(event);
}

Boolean TV_LoginDlg::valid(ushort command)
{
   Boolean rslt = TDialog::valid(command);
   if (rslt && (command == cmOK))
     {
     }
   return rslt;
}

const char * const TV_LoginDlg::name = "TV_LoginDlg";

void TV_LoginDlg::write( opstream& os )
{
 TDialog::write( os );
 os << _okButton << _host << _comboHost << _user << (TInputLine*)_password;
}

void *TV_LoginDlg::read( ipstream& is )
{
 TDialog::read( is );
 is >> _okButton >> _host >> _comboHost >> _user >> (TInputLine*)_password;
 return this;
}

TStreamable *TV_LoginDlg::build()
{
    //return new TV_LoginDlg( streamableInit );
}

// From here to end of file may be removed if TV_LoginDlg will not be streamed.

TStreamableClass RV_LoginDlg( TV_LoginDlg::name,
                        TV_LoginDlg::build,
                        __DELTA(TV_LoginDlg)
                      );
#if 0
__link(RButton)
__link(RCombo)
__link(RInputLine)
__link(RStaticText)
__link(RV_LoginDlg)
#endif

