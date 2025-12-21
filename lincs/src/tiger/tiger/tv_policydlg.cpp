//-------------------------------------------------------
//                       policdlg.cpp
//-------------------------------------------------------

#define Uses_TEvent

#include <tv.h>

#if !defined( __POLICDLG_H )
#include "tv_policydlg.hpp"
#endif

/*
 *
 */
void TV_PolicyDlg::Refresh( void )
{
}


TV_PolicyDlg::TV_PolicyDlg( ICommandHandler *parent, TRect &dims, const char *title ) : 
    IPolicyDlg( parent ),
    TDialog( dims, title ),
    TWindowInit( TDialog::initFrame )
{
 TView *control;
 helpCtx = hcPolicyDlg;

 control = new TStaticText(TRect(38, 10, 49, 11), "FIXME");
 insert(control);

 selectNext(False);
}

void TV_PolicyDlg::handleEvent( TEvent& event)
{
    TDialog::handleEvent(event);
}

Boolean TV_PolicyDlg::valid(ushort command)
{
   Boolean rslt = TDialog::valid(command);
   if (rslt && (command == cmOK))
     {
     }
   return rslt;
}

const char * const TV_PolicyDlg::name = "TV_PolicyDlg";

void TV_PolicyDlg::write( opstream& os )
{
 TDialog::write( os );
}

void *TV_PolicyDlg::read( ipstream& is )
{
 TDialog::read( is );
 return this;
}

TStreamable *TV_PolicyDlg::build()
{
    //return new TV_PolicyDlg( streamableInit );
}

// From here to end of file may be removed if TV_PolicyDlg will not be streamed.

TStreamableClass RV_PolicyDlg( TV_PolicyDlg::name,
                        TV_PolicyDlg::build,
                        __DELTA(TV_PolicyDlg)
                      );

__link(RStaticText)
__link(RV_PolicyDlg)


