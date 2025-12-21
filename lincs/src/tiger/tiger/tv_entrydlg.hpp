//-------------------------------------------------------
//
//   entrydlg.h: Header file for entrydlg.cpp
//
//-------------------------------------------------------

#if !defined( __ENTRYDLG_H )
#define __ENTRYDLG_H

#define Uses_TStreamable
#define Uses_TStreamableClass
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TColoredText
#define Uses_TLabel
#define Uses_TListBox
#define Uses_TSortedListBox
#define Uses_TScrollBar
#define Uses_TStringCollection

#include <tv.h>

#include "tcolortx.h"

#include "tv_tui.hpp"

//#if !defined(__TListBoxRec)
//#define __TListBoxRec
//struct TListBoxRec
//{
//    TCollection* items;
//    short selection;
//};
//#endif

struct dataEntryDlg  {
  TListBoxRec layers;   //TListBox
  TListBoxRec layerAttrNames;   //TListBox
  TListBoxRec layerAttrValues;   //TListBox
  TListBoxRec entries;   //TListBox
  TListBoxRec entryAttrNames;   //TListBox
  TListBoxRec entryAttrValues;   //TListBox
  };


class TV_EntryDlg : public IEntryDlg, public TDialog
{

public: //IEntryDlg

    virtual void Show( void ) { show(); } 
    virtual void Hide( void ) { hide(); }
    virtual void Refresh( void );
    TV_EntryDlg( ICommandHandler *parent, TRect &dims, const char *title );
    //FIXME: def constr, copy constr, assign op, destr

public:

    virtual void handleEvent( TEvent& );
    virtual Boolean valid( ushort );

private:

    TCustListBox *_layers;
    TNSStringCollection *_layersStrCol;
    TCustListBox *_layerAttrNames;
    TNSStringCollection *_layerAttrNamesStrCol;
    TCustListBox *_layerAttrValues;
    TNSStringCollection *_layerAttrValuesStrCol;
    TButton *_commitLayer;
    TCustListBox *_entries;
    TStringCollection *_entriesStrCol;
    TCustListBox *_entryAttrNames;
    TNSStringCollection *_entryAttrNamesStrCol;
    TCustListBox *_entryAttrValues;
    TNSStringCollection *_entryAttrValuesStrCol;
    TButton *_commitValue;
    TColoredText *_crtLayerName;
    TColoredText *_crtValueName;
    TScrollBar *_layerAttrHBar;
    TScrollBar *_layersHBar;
    TScrollBar *_entriesHBar;
    TScrollBar *_entryAttrHBar;


    virtual const char *streamableName() const
        { return name; }
	
	bool SetLayer( const char *name ) ;
	bool GetLayer( const char *name ) ;
	bool RemoveLayer( const char *name ) ;
	bool SetValue( const char *name ) ;
	bool GetValue( const char *name ) ; 
	bool RemoveValue( const char *name ) ;
	
	void RefreshLayerName( const char * name );
	void RefreshValueName( const char * name );
	
	bool navigateLayers( void );
	bool deleteLayer( void );
	bool renameLayer( void );
	bool addLayer( void );
	bool navigateValues( void );
	bool deleteValue( void );
	bool renameValue( void );
	bool addValue( void );
	bool deleteLayerAttrib( void );
	bool renameLayerAttrib( void );
	bool addLayerAttrib( void );
	bool deleteValueAttrib( void );
	bool renameValueAttrib( void );
	bool addValueAttrib( void );

protected:

    //virtual void write( opstream& );
    //virtual void *read( ipstream& );

public:

    static const char * const name;
    //static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TV_EntryDlg& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TV_EntryDlg*& cl )
    { return is >> (void *&)cl; }
inline opstream& operator << ( opstream& os, TV_EntryDlg& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TV_EntryDlg* cl )
    { return os << (TStreamable *)cl; }


#endif  // __ENTRYDLG_H

