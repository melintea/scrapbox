//-------------------------------------------------------
//
//   policdlg.h: Header file for policdlg.cpp
//
//-------------------------------------------------------

#if !defined( __POLICDLG_H )
#define __POLICDLG_H

#define Uses_TStreamable
#define Uses_TStreamableClass
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TStaticText

#include <tv.h>

#include "tv_tui.hpp"



class TV_PolicyDlg : public IPolicyDlg, public TDialog
{

public: //IPolicyDlg

    virtual void Show( void ) { show(); } 
    virtual void Hide( void ) { hide(); }
    virtual void Refresh( void );
    TV_PolicyDlg( ICommandHandler *parent, TRect &dims, const char *title );
    //FIXME: def constr, copy constr, assign op, destr

public:

    virtual void handleEvent( TEvent& );
    virtual Boolean valid( ushort );



private:

    virtual const char *streamableName() const
        { return name; }

protected:

    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TV_PolicyDlg& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TV_PolicyDlg*& cl )
    { return is >> (void *&)cl; }
inline opstream& operator << ( opstream& os, TV_PolicyDlg& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TV_PolicyDlg* cl )
    { return os << (TStreamable *)cl; }


#endif  // __POLICDLG_H

