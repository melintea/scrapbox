//-------------------------------------------------------
//                       infosdlg.cpp
//-------------------------------------------------------

#define Uses_TEvent

#include <tv.h>

#if !defined( __INFOSDLG_H )
#include "tv_infosdlg.hpp"
#endif

/*
 *
 */
void TV_InfosDlg::Refresh( void )
{
}


TV_InfosDlg::TV_InfosDlg( ICommandHandler *parent, TRect &dims, const char *title ) : 
    IInfosDlg( parent ),
    TDialog( dims, title ),
    TWindowInit( TDialog::initFrame )
{
 TView *control;
 helpCtx = hcInfosDlg;

 GetInfos();
 control = new TStaticText(TRect(2, 2, 76, 21), _infos.c_str());
 insert(control);

 selectNext(False);
}

void TV_InfosDlg::handleEvent( TEvent& event)
{
    TDialog::handleEvent(event);
}

Boolean TV_InfosDlg::valid(ushort command)
{
   Boolean rslt = TDialog::valid(command);
   if (rslt && (command == cmOK))
     {
     }
   return rslt;
}

const char * const TV_InfosDlg::name = "TV_InfosDlg";

void TV_InfosDlg::write( opstream& os )
{
 TDialog::write( os );
}

void *TV_InfosDlg::read( ipstream& is )
{
 TDialog::read( is );
 return this;
}

TStreamable *TV_InfosDlg::build()
{
    //return new TV_InfosDlg( streamableInit );
}

// From here to end of file may be removed if TV_InfosDlg will not be streamed.

TStreamableClass RV_InfosDlg( TV_InfosDlg::name,
                        TV_InfosDlg::build,
                        __DELTA(TV_InfosDlg)
                      );

__link(RStaticText)
__link(RV_InfosDlg)


