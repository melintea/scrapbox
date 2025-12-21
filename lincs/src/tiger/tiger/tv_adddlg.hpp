//-------------------------------------------------------
//
//   add.h: Header file for add.cpp
//
//-------------------------------------------------------

#if !defined( __ADD_H )
#define __ADD_H

#define Uses_TStreamable
#define Uses_TStreamableClass
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TButton
#define Uses_TLabel
#define Uses_TMemo
#define Uses_TScrollBar

#include <tv.h>

#include "tv_tui.hpp"


struct dataAddAttrDlg  {
  TMemoData name;
  TMemoData value;
  ushort exit_code;
  };


class TV_AddDlg : public TDialog
{

public:

    TV_AddDlg( bool showValLine = true );
    TV_AddDlg( StreamableInit ) :
           TDialog (streamableInit),
           TWindowInit(TV_AddDlg::initFrame) {};
    virtual void handleEvent( TEvent& );
    virtual Boolean valid( ushort );
    ushort exitCode( void );
    virtual void getData( void* data );
    virtual void setData( void* data );
    //FIXME: def constr, copy constr, assign op, destr

    TMemo *_name;
    TMemo *_value;
    TButton *_okButton;
    TButton *_cancelButton;


private:

    virtual const char *streamableName() const
        { return name; }
	
	ushort _exit_code;

protected:

    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TV_AddDlg& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TV_AddDlg*& cl )
    { return is >> (void *&)cl; }
inline opstream& operator << ( opstream& os, TV_AddDlg& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TV_AddDlg* cl )
    { return os << (TStreamable *)cl; }


#endif  // __ADD_H

