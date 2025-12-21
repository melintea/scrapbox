/*
 *  Quick replacement for colored static text and other widgets
 */

#if !defined( __TCOLORTX_H )
#define __TCOLORTX_H

#define Uses_TStreamable
#define Uses_TStreamableClass
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TLabel
#define Uses_TInputLine
#define Uses_TListBox
#define Uses_TScrollBar
#define Uses_TKeys
#define Uses_MsgBox
#define Uses_TStringCollection

#include <tv.h>


/*
 *
 */
class TColoredText : public TLabel //TStaticText
{

public:

    TColoredText( const TRect& bounds, const char *aText, unsigned color ) :
        TLabel( bounds, aText, NULL ),
	    _color(color),
	    _text(aText)
        {};
    TColoredText( const TRect& bounds, const char *aText, TView *aLink ) :
        TLabel( bounds, aText, aLink ),
        _text(aText)
	    {};
    TColoredText( const TRect& bounds, const char *aText, TView *aLink, stTVIntl *aIntlText ) : 
        TLabel( bounds, aText, aLink, aIntlText ),
        _text(aText)
	    {};

    const char * getText() 
    { 
        return _text;
    }

    virtual void draw() 
    { 
        //TLabel::draw(); 
	
        TDrawBuffer b;
        uchar scOff;

        if( light ) {
            //_color = getColor(0x0402);
            scOff = 0;
        } else {
            //_color = getColor(0x0301);
            scOff = 4;
        }

        b.moveChar( 0, ' ', _color, size.x );
        if( text != 0 ) {
            b.moveCStr( 1, getText(), _color );
            if( light ) {
	             // Usually this will do nothing because the focus is in the linked object
                setCursor( 1 , 0 );
                resetCursor();
            }
        }
        if( showMarkers )
            b.putChar( 0, specialChars[scOff] );
        writeLine( 0, 0, size.x, 1, b );
	};
    virtual TPalette& getPalette() const { TLabel::getPalette(); };
    virtual void handleEvent( TEvent& event ) {}; //{ TLabel::handleEvent(event); };
    virtual void shutDown() { TLabel::shutDown(); };

    TView *link;

protected:

    Boolean light;
    void init( TView *aLink ) { TLabel::init(aLink); };

#if !defined( NO_STREAM )
private:

    virtual const char *streamableName() const
        { return name; }
	unsigned _color; 
	const char *_text; 

protected:

    TColoredText( StreamableInit si ) :
        TLabel( si ) 
	    {};
    virtual void write( opstream& o) { TLabel::write(o); };
    virtual void *read( ipstream& i) { TLabel::read(i); };

public:

    static const char * const name;
    static TStreamable *build() { TLabel::build(); };
#endif // NO_STREAM
};


  static char *strset( char *str, int ch )
  {
      while( *str )
          *str++ = ch;
  }
  //
  // Borland Tech. Support code
  //
  // 1) Save the current inputline data string.
  // 2) Copy a character into every byte of the inputline
  //    data string.
  // 3) Draw the 'masked' data string.
  // 4) Copy the saved string back into the inputline data area.
  //
  class TInputPassword : public TInputLine
  {
       const char passwordChar;

  public:
       TInputPassword(const TRect& bounds, int aMaxLen):
            TInputLine( bounds, aMaxLen ), passwordChar('*'){}
       void draw()
       {
           char* save = newStr( (char*)getData() );
           strset( (char*)getData(), passwordChar );
           TInputLine::draw();
           setDataFromStr( save );
           delete save;
       }
  };


  class TCustListBox : public TListBox
  {
      public: 
          TCustListBox( const TRect& bounds, ushort aNumCols, TScrollBar *aScrollBar )
              : TListBox(bounds, aNumCols, aScrollBar)
          {
          }

          TCustListBox( const TRect& bounds, ushort aNumCols, TScrollBar *aHScrollBar,
                    TScrollBar *aVScrollBar )
              : TListBox(bounds, aNumCols, aHScrollBar, aVScrollBar)
          {
          }

          virtual void handleEvent( TEvent &ev ) 
          {
              TListBox::handleEvent( ev );
          }

          virtual void focusItem( ccIndex item )
          {
              if ( !list() || item < 0 || item+1 > list()->getCount() ) {
                  return; 
              }

              TListBox::focusItem( item );
              TListBox::drawView();
              message( owner, evBroadcast, cmListItemSelected, this/*list()->at(item)*/ ); 
          }
	  
          //empty the listbox and its associated collection
          void flush( void ) 
          {
              TCollection *lst = list(); 
              while ( lst->getCount() > 0 ) {
                  lst->atFree( 0 );
              }
          }

          virtual ~TCustListBox() {};
      private: 
  };


  /*
   * Non-sorted string collection
   */
  class TNSStringCollection : public TCollection
  {
  public:
      TNSStringCollection( ccIndex aLimit, ccIndex aDelta )
          : TCollection(aLimit, aDelta) {};
      //TNSStringCollection & operator = ( const TNSStringCollection & );

  private:
      virtual int compare( void *key1, void *key2 )
      {
          return strcmp( (char *)key1, (char *)key2 );
      }
      
      virtual void freeItem( void *item )
      {
          delete[] (char *)item;
      }
      
      virtual ccIndex insert( void *item )
      {
          return TCollection::insert( item );
      }
      
      virtual void *readItem( ipstream& ) {}
      virtual void writeItem( void *, opstream& ) {}
  };

  //TNSStringCollection &TNSStringCollection::operator=( const TNSStringCollection & pl )
  //{
  //    int i;
  //    
  //    duplicates = pl.duplicates;
  //    freeAll();
  //    for ( i=0; i<pl.count; i++ ) {
  //        insert( strdup((char *)pl.items[i]) );
  //    }
  //    return *this;
  //}

#endif  // __TCOLORTX_H

