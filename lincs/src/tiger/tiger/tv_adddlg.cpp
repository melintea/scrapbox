//-------------------------------------------------------
//                       add.cpp
//-------------------------------------------------------

#define Uses_TEvent

#if !defined( __ADD_H )
#include "tv_adddlg.hpp"
#endif

TV_AddDlg::TV_AddDlg( bool showValLine ) :
       TDialog(TRect(10, 2, 72, 15), "Add/Rename"),
       TWindowInit(TV_AddDlg::initFrame)

{
 TView *control, *control1;
 helpCtx = hcAddDlg;

 control1 = new TScrollBar(TRect(3, 4, 58, 5));
 insert(control1);

 _name = new TMemo(TRect(3, 3, 58, 4), (TScrollBar*)control1, 0, 0, 1024);
 _name->helpCtx = hcAddDlg;
 insert(_name);

   insert(new TLabel(TRect(3, 2, 9, 3), "Name:", _name));

 control1 = new TScrollBar(TRect(3, 8, 58, 9));
 insert(control1);

 _value = new TMemo(TRect(3, 7, 58, 8), (TScrollBar*)control1, 0, 0, 255);
 _value->helpCtx = hcAddDlg;
 insert(_value);
 
   insert(new TLabel(TRect(3, 6, 10, 7), "Value:", _value));
   
 if ( !showValLine ) {
   _value->hide();
 }

 _okButton = new TButton(TRect(13, 10, 23, 12), "O~K~", cmOK, bfDefault);
 insert(_okButton);

 _cancelButton = new TButton(TRect(35, 10, 49, 12), "~C~ancel", cmCancel, bfDefault);
 insert(_cancelButton);

 selectNext(False);
}

ushort TV_AddDlg::exitCode( void )
{
    return _exit_code;
}

void TV_AddDlg::getData( void* data )
{
    dataAddAttrDlg *d = (dataAddAttrDlg*)data;
    
    if ( !d ) 
        return;
	
	_name->getData( &d->name );
	_value->getData( &d->value );
	d->exit_code = _exit_code;
}

void TV_AddDlg::setData( void* data )
{
    dataAddAttrDlg *d = (dataAddAttrDlg*)data;
    
    if ( !d ) 
        return;
	
	_name->setData( &d->name );
	_value->setData( &d->value );
}

void TV_AddDlg::handleEvent( TEvent& event)
{
    if ( event.what == evKeyDown ) {
        switch( event.keyDown.keyCode ) {
	        case kbEnter: 
                if ( _okButton->state & sfSelected )      _exit_code = cmOK; 
                if ( _cancelButton->state & sfSelected )  _exit_code = cmCancel; 
                //clearEvent( event );
                break;
            default:                    //  Unknown command
		        break; 
	    }
    }
    
    TDialog::handleEvent(event);
}

Boolean TV_AddDlg::valid(ushort command)
{
   Boolean rslt = TDialog::valid(command);
   if (rslt && (command == cmOK))
     {
     }
   return rslt;
}

const char * const TV_AddDlg::name = "TV_AddDlg";

void TV_AddDlg::write( opstream& os )
{
 TDialog::write( os );
 os << _name << _value << _okButton << _cancelButton;
}

void *TV_AddDlg::read( ipstream& is )
{
 TDialog::read( is );
 is >> _name >> _value >> _okButton >> _cancelButton;
 return this;
}

TStreamable *TV_AddDlg::build()
{
    return new TV_AddDlg( streamableInit );
}

// From here to end of file may be removed if TV_AddDlg will not be streamed.

TStreamableClass RV_AddDlg( TV_AddDlg::name,
                        TV_AddDlg::build,
                        __DELTA(TV_AddDlg)
                      );

__link(RButton)
__link(RLabel)
__link(RMemo)
__link(RScrollBar)
__link(RV_AddDlg)


