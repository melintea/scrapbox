//-------------------------------------------------------
//
//   logindlg.h: Header file for logindlg.cpp
//
//-------------------------------------------------------

#if !defined( __LOGINDLG_H )
#define __LOGINDLG_H

#define Uses_TStreamable
#define Uses_TStreamableClass
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TInputLine
#define Uses_TComboBox
#define Uses_TStringCollection
#define Uses_TListBox
#define Uses_TSItem

#include <tv.h>

#include "tcombobx.h"
#include "tcolortx.h"

#include "tv_tui.hpp"

struct dataLoginDlg  {
  char host[128];   //TInputLine
  TStringCollection* comboHost;   //TCombo
  char user[128];   //TInputLine
  char password[38];   //TInputLine
  };


class TV_LoginDlg : public ILoginDlg, public TDialog
{

public: //ILoginDlg

    virtual void Show( void ) { show(); } 
    virtual void Hide( void ) { hide(); }
    virtual void Refresh( void );
    TV_LoginDlg( ICommandHandler *parent, TRect &dims, const char *title );
    //FIXME: def constr, copy constr, assign op, destr

public:

    virtual void handleEvent( TEvent& );
    virtual Boolean valid( ushort );

    TButton *_okButton;
    TInputLine *_host;
    TStringCollection *_hosts;
    TComboBox *_comboHost;
    TInputLine *_user;
    TInputPassword *_password;


private:

    void fillHosts( void );
    virtual const char *streamableName() const
        { return name; }

protected:

    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TV_LoginDlg& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TV_LoginDlg*& cl )
    { return is >> (void *&)cl; }
inline opstream& operator << ( opstream& os, TV_LoginDlg& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TV_LoginDlg* cl )
    { return os << (TStreamable *)cl; }


#endif  // __LOGINDLG_H

