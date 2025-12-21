//-------------------------------------------------------
//                       entrydlg.cpp
//-------------------------------------------------------

#define Uses_TKeys
#define Uses_TKeys_Extended
/*
 * probably a TV bug - have to redefine those
 */
#define kbIns (0x004d)
#define kbDel (0x004e)

#if !defined( __ENTRYDLG_H )
#include "tv_entrydlg.hpp"
#endif

#include "tv_adddlg.hpp"


/*
 *
 */
void TV_EntryDlg::Refresh( void )
{
}


TV_EntryDlg::TV_EntryDlg( ICommandHandler *parent, TRect &dims, const char *title ) : 
       TDialog(dims, title),
       TWindowInit(TV_EntryDlg::initFrame),
       IEntryDlg( parent ) 
{
 TView *control;
 helpCtx = hcEntryDlg;

 control = new TScrollBar(TRect(21, 3, 22, 8));
 insert(control);

 _layers = new TCustListBox(TRect(1, 3, 21, 8), 1, (TScrollBar*)control);
 _layers->helpCtx = hcEntryDlg;
 //_layers->eventMask |= evMouseUp;
 //_layers->options |= ofPreProcess | ofPostProcess;
 _layersStrCol = new TNSStringCollection( 100, 10 );
     _layersStrCol->atInsert( 0, newStr("/") );
     _layersStrCol->atInsert( 1, newStr("..") );
     _layers->newList( _layersStrCol );
 insert(_layers);

   insert(new TLabel(TRect(1, 2, 9, 3), "Layers:", _layers));

 control = new TScrollBar(TRect(47, 3, 48, 8));
 insert(control);

 _layerAttrNames = new TCustListBox(TRect(23, 3, 48, 8), 1, (TScrollBar*)control);
 _layerAttrNames->helpCtx = hcEntryDlg;
 _layerAttrNames->eventMask |= evMouseUp;
 _layerAttrNames->options |= ofPreProcess | ofPostProcess;
 _layerAttrNamesStrCol = new TNSStringCollection( 100, 10 );
     _layerAttrNames->newList( _layerAttrNamesStrCol );
 insert(_layerAttrNames);

   insert(new TLabel(TRect(23, 2, 43, 3), "Layer's Attributes:", _layerAttrNames));

 control = new TScrollBar(TRect(78, 3, 79, 8));
 insert(control);

 _layerAttrValues = new TCustListBox(TRect(48, 3, 78, 8), 1, (TScrollBar*)control);
 _layerAttrValues->helpCtx = hcEntryDlg;
 _layerAttrValues->eventMask |= evMouseUp;
 _layerAttrValues->options |= ofPreProcess | ofPostProcess;
 _layerAttrValuesStrCol = new TNSStringCollection( 100, 10 );
     _layerAttrValues->newList( _layerAttrValuesStrCol );
 insert(_layerAttrValues);

   insert(new TLabel(TRect(48, 2, 56, 3), "Values:", _layerAttrValues));

 _commitLayer = new TButton(TRect(62, 9, 78, 11), "Commit ~L~ayer", cmCommitLayer, bfDefault);
 _commitLayer->helpCtx = hcEntryDlg;
 insert(_commitLayer);

 control = new TScrollBar(TRect(21, 13, 22, 18));
 insert(control);

 _entries = new TCustListBox(TRect(1, 13, 21, 18), 1, (TScrollBar*)control);
 _entries->helpCtx = hcEntryDlg;
 _entries->eventMask |= evMouseUp;
 _entries->options |= ofPreProcess | ofPostProcess;
 _entriesStrCol = new TStringCollection( 100, 10 );
     _entriesStrCol->atInsert( 0, newStr("/") );
     _entriesStrCol->atInsert( 1, newStr("..") );
     _entries->newList( _entriesStrCol );
 insert(_entries);

   insert(new TLabel(TRect(1, 12, 18, 13), "Values in layer:", _entries));

 control = new TScrollBar(TRect(47, 13, 48, 18));
 insert(control);

 _entryAttrNames = new TCustListBox(TRect(23, 13, 48, 18), 1, (TScrollBar*)control);
 _entryAttrNames->helpCtx = hcEntryDlg;
 _entryAttrNames->eventMask |= evMouseUp;
 _entryAttrNames->options |= ofPreProcess | ofPostProcess;
 _entryAttrNamesStrCol = new TNSStringCollection( 100, 10 );
     _entryAttrNames->newList( _entryAttrNamesStrCol );
 insert(_entryAttrNames);

   insert(new TLabel(TRect(23, 12, 43, 13), "Value's Attributes:", _entryAttrNames));

 control = new TScrollBar(TRect(78, 13, 79, 18));
 insert(control);

 _entryAttrValues = new TCustListBox(TRect(48, 13, 78, 18), 1, (TScrollBar*)control);
 _entryAttrValues->helpCtx = hcEntryDlg;
 _entryAttrValues->eventMask |= evMouseUp;
 _entryAttrValues->options |= ofPreProcess | ofPostProcess;
 _entryAttrValuesStrCol = new TNSStringCollection( 100, 10 );
     _entryAttrValues->newList( _entryAttrValuesStrCol );
 insert(_entryAttrValues);

   insert(new TLabel(TRect(48, 12, 55, 13), "Values", _entryAttrValues));

 _commitValue = new TButton(TRect(62, 19, 78, 21), "Commit ~V~alue", cmCommitValue, bfDefault);
 _commitValue->helpCtx = hcEntryDlg;
 insert(_commitValue);

 RefreshLayerName( "/" );

 control = new TStaticText(TRect(77, 21, 79, 22), 0);
 insert(control);

 RefreshValueName( "/" );

 _layerAttrHBar = new TScrollBar(TRect(23, 8, 79, 9));
 insert(_layerAttrHBar);

 _layersHBar = new TScrollBar(TRect(1, 8, 22, 9));
 insert(_layersHBar);

 _entriesHBar = new TScrollBar(TRect(1, 18, 22, 19));
 insert(_entriesHBar);

 _entryAttrHBar = new TScrollBar(TRect(23, 18, 79, 19));
 insert(_entryAttrHBar);

 selectNext(False);
 
 navigateLayers();
 navigateValues();
}

void TV_EntryDlg::RefreshLayerName( const char * name )
{
   if ( _crtLayerName ) 
       destroy( _crtLayerName );
   _crtLayerName = new TColoredText(TRect(1, 1, 79, 2), name, 0x1f);
   insert(_crtLayerName);
}

void TV_EntryDlg::RefreshValueName( const char * name )
{
   if ( _crtValueName ) 
       destroy( _crtValueName );
   _crtValueName = new TColoredText(TRect(1, 11, 79, 12), name, 0x1f);
   insert(_crtValueName);
}

void BuildPath( char *buf, const char *path, const char *sub )
{
    if (  *path != '/' || 0 == strcmp(sub, "/") ) {
        strcpy( buf, "/" );
	    return;
    }
    
    if ( 0 == strcmp(sub, "..") ) {
        strncpy( buf, path, FILENAME_MAX );
	    char *pos = strrchr( buf, '/' );
	    if ( pos == buf )
	        buf[ 1 ] = '\0';
		else
		    *pos = '\0'; 
    } else {
        if ( 0 == strcmp(path, "/") )
            snprintf( buf, FILENAME_MAX, "/%s", sub );
	    else
            snprintf( buf, FILENAME_MAX, "%s/%s", path, sub );
    }
}

bool TV_EntryDlg::navigateLayers( void )
{
    char lname[ FILENAME_MAX+1 ] = {0};
    int  n, ii;
    
    BuildPath( lname, _crtLayerName->getText(), 
        (const char *)_layersStrCol->at(_layers->focused) ); 
    n = IEntryDlg::Navigate( lname, NULL );
    if ( n > 0  ) {
        lincs::navigator::subkeys_iter i = _navigator.begin(); 
	
        _layers->flush();
        _layersStrCol->atInsert( 0, newStr("/") );
        _layersStrCol->atInsert( 1, newStr("..") );
        for ( ii=2; i != _navigator.end(); i++, ii++ ) {
            _layersStrCol->atInsert( ii, newStr(i->c_str()) );
	    }
        _layers->setRange( ii );
	    _layers->drawView();
	    
    	_layerAttrNames->flush();
	    _layerAttrValues->flush();
        RefreshLayerName( newStr(lname) );
	    _crtLayerName->drawView();
	} 
    
	GetLayer( lname );
}

bool TV_EntryDlg::deleteLayer( void )
{
    ushort ccode = cmCancel;
    const char *crtn = NULL;
    bool ret = false;

    if (  _layersStrCol->getCount() 
       && (crtn = (const char *)_layersStrCol->at(_layers->focused)) != NULL ) {
        if ( 0 == strcmp(crtn, "..") || 0 == strcmp (crtn, "/") )
	        return false;
		
        char buf[ 78 ] = {0};
        snprintf( buf, 77, "Delete Layer %s?", crtn );
        ccode = messageBox( buf, mfWarning | mfOKButton | mfCancelButton );
    }
    if ( ccode == cmCancel )
        return false;

    ret = RemoveLayer( crtn ); 
    if ( ret ) {
        _layersStrCol->atFree( _layers->focused );
        _layers->setRange( _layersStrCol->getCount() );
        _layers->drawView();
    }
    
    return ret;
}

bool TV_EntryDlg::renameLayer( void )
{
    messageBox( "FIXME: renameLayer", mfError | mfOKButton );
}

bool TV_EntryDlg::addLayer( void )
{
    struct dataAddAttrDlg ddata = {0};
    TV_AddDlg *td = new TV_AddDlg( false ); 
    ushort ccode;
    bool ret = false;
    
    if ( TProgram::application->validView(td) )
        ccode = TProgram::deskTop->execView( td );
    if ( ccode == cmOK || td->exitCode() == cmOK ) {
        td->getData( &ddata );
         if ( *ddata.name.buffer ) {
	         ddata.name.buffer[ ddata.name.length ] = '\0';
	         if ( SetLayer(ddata.name.buffer) )
		         GetLayer( ddata.name.buffer );
                 _layersStrCol->atInsert( _layersStrCol->getCount(), newStr(name) );
                 _layers->setRange( _layersStrCol->getCount() );
                 _layers->drawView();
         }
    } 
    
    destroy( td );
    return ret;
}

bool TV_EntryDlg::navigateValues( void )
{
    char vname[ FILENAME_MAX+1 ] = {0};
    int  n, ii;
    
    BuildPath( vname, _crtValueName->getText(), 
        (const char *)_entriesStrCol->at(_entries->focused) ); 
    
    n = IEntryDlg::Navigate( _crtLayerName->getText(), vname );     
    if ( n > 0  ) {
        lincs::navigator::subkeys_iter i = _navigator.begin(); 
	
        _entries->flush();
        _entriesStrCol->atInsert( 0, newStr("/") );
        _entriesStrCol->atInsert( 1, newStr("..") );
        for ( ii=2; i != _navigator.end(); i++, ii++ ) {
            _entriesStrCol->atInsert( ii, newStr(i->c_str()) );
	    }
        _entries->setRange( ii );
	    _entries->drawView();
	    
    	_entryAttrNames->flush();
	    _entryAttrValues->flush();
        RefreshValueName( newStr(vname) );
	    _crtValueName->drawView();
	} 

    GetValue( vname );
}

bool TV_EntryDlg::deleteValue( void )
{
    ushort ccode = cmCancel;
    const char *crtn = NULL;
    bool ret = false;

    if (  _entriesStrCol->getCount() 
       && (crtn = (const char *)_entriesStrCol->at(_entries->focused)) != NULL ) {
        if ( 0 == strcmp(crtn, "..") || 0 == strcmp (crtn, "/") )
	        return false;
		
        char buf[ 78 ] = {0};
        snprintf( buf, 77, "Delete Value %s?", crtn );
        ccode = messageBox( buf, mfWarning | mfOKButton | mfCancelButton );
    }
    if ( ccode == cmCancel )
        return false;

    ret = RemoveValue( crtn ); 
    if ( ret ) {
        _entriesStrCol->atFree( _entries->focused );
        _entries->setRange( _entriesStrCol->getCount() );
        _entries->drawView();
    }
    
    return ret;
}

bool TV_EntryDlg::renameValue( void )
{
    messageBox( "FIXME: renameValue", mfError | mfOKButton );
}

bool TV_EntryDlg::addValue( void )
{
    struct dataAddAttrDlg ddata = {0};
    TV_AddDlg *td = new TV_AddDlg( false ); 
    ushort ccode;
    bool ret = false;
    
    if ( TProgram::application->validView(td) )
        ccode = TProgram::deskTop->execView( td );
    if ( ccode == cmOK || td->exitCode() == cmOK ) {
        td->getData( &ddata );
         if ( *ddata.name.buffer ) {
	         ddata.name.buffer[ ddata.name.length ] = '\0';
	         if ( SetValue(ddata.name.buffer) )
		         GetValue( ddata.name.buffer );
                 _entriesStrCol->atInsert( _entriesStrCol->getCount(), newStr(name) );
                 _entries->setRange( _entriesStrCol->getCount() );
                 _entries->drawView();
         }
    } 
    
    destroy( td );
    return ret;
}

bool TV_EntryDlg::deleteLayerAttrib( void )
{
    ushort ccode = cmCancel;
    const char *crtn = NULL;

    if (  _layerAttrNamesStrCol->getCount() 
       && (crtn = (const char *)_layerAttrNamesStrCol->at(_layerAttrNames->focused)) != NULL ) {
        char buf[ 78 ] = {0};
        snprintf( buf, 77, "Delete Value %s?", crtn );
        ccode = messageBox( buf, mfWarning | mfOKButton | mfCancelButton );
    }
    if ( ccode == cmCancel )
        return false;

    _layerAttrNamesStrCol->atFree( _layerAttrNames->focused );
    _layerAttrNames->setRange( _layerAttrNamesStrCol->getCount() );
    _layerAttrNames->drawView();
	
    _layerAttrValuesStrCol->atFree( _layerAttrValues->focused );
    _layerAttrValues->setRange( _layerAttrValuesStrCol->getCount() );
    _layerAttrValues->drawView();
	
    return true;
}

bool TV_EntryDlg::renameLayerAttrib( void )
{
    struct dataAddAttrDlg ddata = {0};
    TV_AddDlg *td = new TV_AddDlg(); 
    ushort ccode = cmCancel;
    const char *crtn = NULL, *crtv = NULL;
    ccIndex pos;
    
    if ( !TProgram::application->validView(td) ) {
        delete td;
        return false;
	}
    
    if (  _layerAttrNamesStrCol->getCount() 
       && (crtn = (const char *)_layerAttrNamesStrCol->at(_layerAttrNames->focused)) != NULL ) {
        crtv = (const char *)_layerAttrValuesStrCol->at( _layerAttrNames->focused ); 
	
	    ddata.value.length = strlen( crtv );
	    strcpy( ddata.value.buffer, crtv );
	    ddata.value.buffer[ ddata.value.length ] = '\0';
	    
	    ddata.name.length = strlen( crtn );
	    strcpy( ddata.name.buffer, crtn );
	    ddata.name.buffer[ ddata.name.length ] = '\0';
	    
	    td->setData( &ddata );
        ccode = TProgram::deskTop->execView( td );
    }
	
    if ( ccode == cmOK || td->exitCode() == cmOK ) {
        td->getData( &ddata );
         if ( *ddata.name.buffer && *ddata.value.buffer ) {
	 
             char *tmpn = new char[ ddata.name.length+1 ];
             char *tmpv = new char[ ddata.value.length+1 ];
	     
             if ( tmpn && tmpv ) {
                 memset( tmpn, '\0', ddata.name.length+1 );
                 strncpy( tmpn, ddata.name.buffer, ddata.name.length );
                 memset( tmpv, '\0', ddata.value.length+1 );
                 strncpy( tmpv, ddata.value.buffer, ddata.value.length );
		 
		         pos = _layerAttrNames->focused; 
		 
                 _layerAttrNamesStrCol->atReplace( pos, tmpn );
                 _layerAttrNames->setRange( _layerAttrNamesStrCol->getCount() );
                 _layerAttrNames->drawView();
		 
                 _layerAttrValuesStrCol->atReplace( pos, tmpv );
                 _layerAttrValues->setRange( _layerAttrValuesStrCol->getCount() );
                 _layerAttrValues->drawView();
             }
         }
    } 
    
    delete td;
    return true; 
}

bool TV_EntryDlg::addLayerAttrib( void )
{
    struct dataAddAttrDlg ddata = {0};
    TV_AddDlg *td = new TV_AddDlg(); 
    ushort ccode = cmCancel;
    const char *crtn = NULL;
    
    if ( !TProgram::application->validView(td) ) {
        delete td;
        return false;
	}
    
    ccode = TProgram::deskTop->execView( td );
    if ( ccode == cmOK || td->exitCode() == cmOK ) {
        td->getData( &ddata );
         if ( *ddata.name.buffer && *ddata.value.buffer ) {
	 
             char *tmpn = new char[ ddata.name.length+1 ];
             char *tmpv = new char[ ddata.value.length+1 ];
	     
             if ( tmpn && tmpv ) {
                 memset( tmpn, '\0', ddata.name.length+1 );
                 strncpy( tmpn, ddata.name.buffer, ddata.name.length );
                 memset( tmpv, '\0', ddata.value.length+1 );
                 strncpy( tmpv, ddata.value.buffer, ddata.value.length );
		 
                 _layerAttrNamesStrCol->atInsert( _layerAttrNamesStrCol->getCount(), tmpn );
                 _layerAttrNames->setRange( _layerAttrNamesStrCol->getCount() );
                 _layerAttrNames->drawView();
		 
                 _layerAttrValuesStrCol->atInsert( _layerAttrValuesStrCol->getCount(), tmpv );
                 _layerAttrValues->setRange( _layerAttrValuesStrCol->getCount() );
                 _layerAttrValues->drawView();
             }
         }
    } 
    
    delete td;
    return true; 
}

bool TV_EntryDlg::deleteValueAttrib( void )
{
    ushort ccode = cmCancel;
    const char *crtn = NULL;

    if (  _entryAttrNamesStrCol->getCount() 
       && (crtn = (const char *)_entryAttrNamesStrCol->at(_entryAttrNames->focused)) != NULL ) {
        char buf[ 78 ] = {0};
        snprintf( buf, 77, "Delete Value %s?", crtn );
        ccode = messageBox( buf, mfWarning | mfOKButton | mfCancelButton );
    }
    if ( ccode == cmCancel )
        return false;

    _entryAttrNamesStrCol->atFree( _entryAttrNames->focused );
    _entryAttrNames->setRange( _entryAttrNamesStrCol->getCount() );
    _entryAttrNames->drawView();
	
    _entryAttrValuesStrCol->atFree( _entryAttrValues->focused );
    _entryAttrValues->setRange( _entryAttrValuesStrCol->getCount() );
    _entryAttrValues->drawView();
	
    return true;
}

bool TV_EntryDlg::renameValueAttrib( void )
{
    struct dataAddAttrDlg ddata = {0};
    TV_AddDlg *td = new TV_AddDlg(); 
    ushort ccode = cmCancel;
    const char *crtn = NULL, *crtv = NULL;
    ccIndex pos;
    
    if ( !TProgram::application->validView(td) ) {
        delete td;
        return false;
	}
    
    if (  _entryAttrNamesStrCol->getCount() 
       && (crtn = (const char *)_entryAttrNamesStrCol->at(_entryAttrNames->focused)) != NULL ) {
        crtv = (const char *)_entryAttrValuesStrCol->at( _entryAttrNames->focused ); 
	
	    ddata.value.length = strlen( crtv );
	    strcpy( ddata.value.buffer, crtv );
	    ddata.value.buffer[ ddata.value.length ] = '\0';
	    
	    ddata.name.length = strlen( crtn );
	    strcpy( ddata.name.buffer, crtn );
	    ddata.name.buffer[ ddata.name.length ] = '\0';
	    
	    td->setData( &ddata );
        ccode = TProgram::deskTop->execView( td );
    }
	
    if ( ccode == cmOK || td->exitCode() == cmOK ) {
        td->getData( &ddata );
         if ( *ddata.name.buffer && *ddata.value.buffer ) {
	 
             char *tmpn = new char[ ddata.name.length+1 ];
             char *tmpv = new char[ ddata.value.length+1 ];
	     
             if ( tmpn && tmpv ) {
                 memset( tmpn, '\0', ddata.name.length+1 );
                 strncpy( tmpn, ddata.name.buffer, ddata.name.length );
                 memset( tmpv, '\0', ddata.value.length+1 );
                 strncpy( tmpv, ddata.value.buffer, ddata.value.length );
		 
		         pos = _entryAttrNames->focused; 
		 
                 _entryAttrNamesStrCol->atReplace( pos, tmpn );
                 _entryAttrNames->setRange( _entryAttrNamesStrCol->getCount() );
                 _entryAttrNames->drawView();
		 
                 _entryAttrValuesStrCol->atReplace( pos, tmpv );
                 _entryAttrValues->setRange( _entryAttrValuesStrCol->getCount() );
                 _entryAttrValues->drawView();
             }
         }
    } 
    
    delete td;
    return true; 
}

bool TV_EntryDlg::addValueAttrib( void )
{
    struct dataAddAttrDlg ddata = {0};
    TV_AddDlg *td = new TV_AddDlg(); 
    ushort ccode = cmCancel;
    const char *crtn = NULL;
    
    if ( !TProgram::application->validView(td) ) {
        delete td;
        return false;
	}
    
    ccode = TProgram::deskTop->execView( td );
    if ( ccode == cmOK || td->exitCode() == cmOK ) {
        td->getData( &ddata );
         if ( *ddata.name.buffer && *ddata.value.buffer ) {
	 
             char *tmpn = new char[ ddata.name.length+1 ];
             char *tmpv = new char[ ddata.value.length+1 ];
	     
             if ( tmpn && tmpv ) {
                 memset( tmpn, '\0', ddata.name.length+1 );
                 strncpy( tmpn, ddata.name.buffer, ddata.name.length );
                 memset( tmpv, '\0', ddata.value.length+1 );
                 strncpy( tmpv, ddata.value.buffer, ddata.value.length );
		 
                 _entryAttrNamesStrCol->atInsert( _entryAttrNamesStrCol->getCount(), tmpn );
                 _entryAttrNames->setRange( _entryAttrNamesStrCol->getCount() );
                 _entryAttrNames->drawView();
		 
                 _entryAttrValuesStrCol->atInsert( _entryAttrValuesStrCol->getCount(), tmpv );
                 _entryAttrValues->setRange( _entryAttrValuesStrCol->getCount() );
                 _entryAttrValues->drawView();
             }
         }
    } 
    
    delete td;
    return true; 
}


bool TV_EntryDlg::SetLayer( const char *name ) 
{
    ccIndex nattr = _layerAttrNamesStrCol->getCount();
    ccIndex i;
    
    _layer.clear();
	for ( i=0; i < nattr; i++ ) {
	    _layer.setAttrib( 
	        (const char*)_layerAttrNamesStrCol->at(i), 
		    (const char*)_layerAttrValuesStrCol->at(i) );
	}
	
    // FIXME: if name is relative    
    
    return IEntryDlg::SetLayer( name ); 
}

bool TV_EntryDlg::GetLayer( const char *name ) 
{
    // FIXME: if name is relative    
    
    if ( !IEntryDlg::GetLayer(name) )
        return false;
    
    // update other controls
	_layerAttrNames->flush();
	_layerAttrValues->flush();
	// fill lists with attributes 
	lincs::entry::attr_map_iter attr = _layer.begin();
	for ( int i=0; attr != _layer.end(); attr++, i++ ) {
	    _layerAttrNamesStrCol->atInsert( i, newStr(attr->first.c_str()) );
    	_layerAttrValuesStrCol->atInsert( i, newStr(attr->second.c_str()) );
	}
    _layerAttrNames->setRange( _layerAttrNamesStrCol->getCount() );
    _layerAttrNames->drawView();
    _layerAttrValues->setRange( _layerAttrValuesStrCol->getCount() );
    _layerAttrValues->drawView();
    
    //_layersStrCol->atInsert( _layersStrCol->getCount(), newStr(name) );
    //_layers->setRange( _layersStrCol->getCount() );
    //_layers->drawView();
		     
    //FIXME: refresh layer name
		     
    return true;
}

bool TV_EntryDlg::RemoveLayer( const char *name ) 
{
    return IEntryDlg::RemoveLayer( name );
}

bool TV_EntryDlg::SetValue( const char *name ) 
{
    ccIndex nattr = _entryAttrNamesStrCol->getCount();
    ccIndex i;
    bool    ret;
    
    _value.clear();
	for ( i=0; i < nattr; i++ ) {
	    _value.setAttrib( 
	        (const char*)_entryAttrNamesStrCol->at(i), 
		    (const char*)_entryAttrValuesStrCol->at(i) );
	}
	
    // FIXME: if name is relative    
    
    ret = IEntryDlg::SetValue( name ); 
    return ret;
}

bool TV_EntryDlg::GetValue( const char *name ) 
{
    // FIXME: if name is relative  
    
    if ( !IEntryDlg::GetValue(name) )
        return false;
    
    // update other controls
	_entryAttrNames->flush();
	_entryAttrValues->flush();
	// fill lists with attributes 
	lincs::entry::attr_map_iter attr = _value.begin();
	for ( int i=0; attr != _value.end(); attr++, i++ ) {
	    _entryAttrNamesStrCol->atInsert( i, newStr(attr->first.c_str()) );
    	_entryAttrValuesStrCol->atInsert( i, newStr(attr->second.c_str()) );
	}
    _entryAttrNames->setRange( _entryAttrNamesStrCol->getCount() );
    _entryAttrNames->drawView();
    _entryAttrValues->setRange( _entryAttrValuesStrCol->getCount() );
    _entryAttrValues->drawView();
    
    //_entriesStrCol->atInsert( _entriesStrCol->getCount(), newStr(name) );
    //_entries->setRange( _entriesStrCol->getCount() );
    //_entries->drawView();
    
    //FIXME: update value name static
		     
    return true;
}

bool TV_EntryDlg::RemoveValue( const char *name ) 
{
    return IEntryDlg::RemoveValue( name );
}


void TV_EntryDlg::handleEvent( TEvent& event)
{
    //TApplication::handleEvent( event );

    if ( event.what == evCommand ) {
        switch( event.message.command ) {
            case cmCommitLayer: 
                SetLayer( _crtLayerName->getText() );
                clearEvent( event );
                break;
            case cmCommitValue: 
                SetValue(  _crtValueName->getText() );
                clearEvent( event );
                break;
            case cmListItemSelected: 
                messageBox( "FIXME: cmListItemSelected", mfError | mfOKButton );
                clearEvent( event );
                break;
            default:                    //  Unknown command
                break; 
        }
    } else if ( event.what == evKeyDown ) {
        switch( event.keyDown.keyCode ) {
            case kbIns: case kbA:
                if ( _entries->state & sfSelected ) addValue(); 
                if ( _layers->state & sfSelected )  addLayer(); 
                if (  _layerAttrNames->state & sfSelected 
                   || _layerAttrValues->state & sfSelected ) addLayerAttrib(); 
                if (  _entryAttrNames->state & sfSelected 
                   || _entryAttrValues->state & sfSelected ) addValueAttrib(); 
                clearEvent( event );
                break;
            case kbDel: case kbD: 
                if ( _entries->state & sfSelected ) deleteValue(); 
                if ( _layers->state & sfSelected )  deleteLayer(); 
                if (  _layerAttrNames->state & sfSelected 
                   || _layerAttrValues->state & sfSelected ) deleteLayerAttrib(); 
                if (  _entryAttrNames->state & sfSelected 
                   || _entryAttrValues->state & sfSelected ) deleteValueAttrib(); 
                clearEvent( event );
                break;
            case kbN: 
                if ( _entries->state & sfSelected ) renameValue(); 
                if ( _layers->state & sfSelected )  renameLayer(); 
                if (  _layerAttrNames->state & sfSelected 
                   || _layerAttrValues->state & sfSelected ) renameLayerAttrib(); 
                if (  _entryAttrNames->state & sfSelected 
                   || _entryAttrValues->state & sfSelected ) renameValueAttrib(); 
                clearEvent( event );
                break;
            case kbEnter: 
                if ( _entries->state & sfSelected ) navigateValues(); 
                if ( _layers->state & sfSelected )  navigateLayers(); 
                //if (  _layerAttrNames->state & sfSelected 
                //   || _layerAttrValues->state & sfSelected ) renameLayerAttrib(); 
                //if (  _entryAttrNames->state & sfSelected 
                //   || _entryAttrValues->state & sfSelected ) renameValueAttrib(); 
                if ( _commitLayer->state & sfSelected )  SetLayer( _crtLayerName->getText() ); 
                if ( _commitValue->state & sfSelected )  SetValue( _crtValueName->getText() ); 
                clearEvent( event );
                break;
            /* Putting this functionality into the list class does not seem to work */
            case kbDown:
                if ( _layers->state & sfSelected ) _layers->focusItem( _layers->focused+1 ); 
                if (  _layerAttrNames->state & sfSelected 
		           || _layerAttrValues->state & sfSelected ) { 
		            _layerAttrNames->focusItem( _layerAttrNames->focused+1 ); 
		            _layerAttrValues->focusItem( _layerAttrValues->focused+1 ); 
			    }
                if ( _entries->state & sfSelected ) _entries->focusItem( _entries->focused+1 ); 
                if (  _entryAttrNames->state & sfSelected 
		           || _entryAttrValues->state & sfSelected ) { 
		            _entryAttrNames->focusItem( _entryAttrNames->focused+1 ); 
		            _entryAttrValues->focusItem( _entryAttrValues->focused+1 ); 
			    }
                break;
            case kbUp:
                if ( _layers->state & sfSelected ) _layers->focusItem( _layers->focused-1 ); 
                if (  _layerAttrNames->state & sfSelected 
		           || _layerAttrValues->state & sfSelected ) { 
		            _layerAttrNames->focusItem( _layerAttrNames->focused-1 ); 
		            _layerAttrValues->focusItem( _layerAttrValues->focused-1 ); 
			    }
                if ( _entries->state & sfSelected ) _entries->focusItem( _entries->focused-1 ); 
                if (  _entryAttrNames->state & sfSelected 
		           || _entryAttrValues->state & sfSelected ) { 
		            _entryAttrNames->focusItem( _entryAttrNames->focused-1 ); 
		            _entryAttrValues->focusItem( _entryAttrValues->focused-1 ); 
			    }
                break;
            default:                    //  Unknown command
                break; 
        }
    }
    
    // modal/less
    //if ( event.what != evKeyboard || event.keyDown.keyCode != kbEsc )
        TDialog::handleEvent( event );  
}

Boolean TV_EntryDlg::valid(ushort command)
{
   Boolean rslt = TDialog::valid(command);
   if (rslt && (command == cmOK))
     {
     }
   return rslt;
}

const char * const TV_EntryDlg::name = "TV_EntryDlg";
#if 0
void TV_EntryDlg::write( opstream& os )
{
 TDialog::write( os );
 os << _layers << _layerAttrNames << _layerAttrValues << _commitLayer << _entries << _entryAttrNames << _entryAttrValues << _commitValue << _crtLayerName << _crtValueName << _layerAttrHBar << _layersHBar << _entriesHBar << _entryAttrHBar;
}

void *TV_EntryDlg::read( ipstream& is )
{
 TDialog::read( is );
 is >> _layers >> _layerAttrNames >> _layerAttrValues >> _commitLayer >> _entries >> _entryAttrNames >> _entryAttrValues >> _commitValue >> _crtLayerName >> _crtValueName >> _layerAttrHBar >> _layersHBar >> _entriesHBar >> _entryAttrHBar;
 return this;
}

TStreamable *TV_EntryDlg::build()
{
    return new TV_EntryDlg( streamableInit );
}

// From here to end of file may be removed if TV_EntryDlg will not be streamed.

TStreamableClass RV_EntryDlg( TV_EntryDlg::name,
                        TV_EntryDlg::build,
                        __DELTA(TV_EntryDlg)
                      );

__link(RButton)
__link(RColoredText)
__link(RLabel)
__link(RListBox)
__link(RScrollBar)
__link(RStaticText)
__link(RV_EntryDlg)
#endif

