
/**
 *  $Revision: 1.12 $
 *  $Date: 2004/03/31 19:25:12 $
 *
 *  \file 
 *  \brief   
 *
 */
/*
# 
# Copyright (c) 2003 Aurelian Melinte. 
# This file is part of LinCS/tiger.  
# 
# LinCS/tiger is distributed under the terms of the GNU Lesser General Public
# License version 2 or any later version.  See the file COPYING.LIB for copying 
# permission or http://www.gnu.org. 
#                                                                            
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  
# IMPLIED, without even the implied warranty of MERCHANTABILITY or FITNESS 
# FOR A PARTICULAR PURPOSE.  ANY USE IS AT YOUR OWN RISK. 
#                                                                            
# Permission to modify the code and to distribute modified code is granted, 
# provided the above notices are retained, and a notice that the code was 
# modified is included with the above copyright notice. 
# 
 */ 

 
#include <cassert>

#include <tiger/lincs.hpp>
using namespace lincs;

#include "cfgs/cfgs_client_api.h"
#include "cfgs/cfgs_tags.h"
#include "cfgs/cfgs_dlist.h"



LinCS *LinCS::_theLinCS = 0;

Log<file_log> *Log<file_log>::_theLog = 0;
ILog *Log<file_log>::_theLogImplem    = 0;


bool LinCS::Init( void )
{
	if ( _theLinCS ) {
    	return true;
	}
	return false;
}


LinCS *LinCS::Instance( ICommandHandler *parent, int argc, 
        char **argv, char **envp )
{
	if ( !_theLinCS )
		_theLinCS = new LinCS( parent, argc, argv, envp );
	return _theLinCS;
}


void LinCS::Handle( ICommand& c ) 
{ 
    switch ( c.type ) {
    case ICommand::evGET_VALUE: 
        GetValue( c );
        break;
    case ICommand::evSET_VALUE:
        SetValue( c );
        break;
    case ICommand::evGET_LAYER: 
        GetLayer( c );
        break;
    case ICommand::evSET_LAYER:
        SetLayer( c );
        break;
    case ICommand::evGET_LAYER_SUBKEYS: 
        GetLayerSub( c );
        break;
    case ICommand::evGET_VALUE_SUBKEYS:
        GetValueSub( c );
        break;
    case ICommand::evGET_INFOS:
        GetInfos( c );
        break;
    case ICommand::evPRINT_MESSAGE: 
        PrintMessage( static_cast<LincsCommand*>(&c)->msg->c_str() );
        break;
    default:
        throw logic_error( "Invalid event type" );
    }
}

void LinCS::PrintMessage( const string &msg )
{ 
    PrintMessage( msg.c_str() );
}

void LinCS::PrintMessage( const char *msg )
{ 
	LincsCommand lc; 
    string       smsg( msg );

	lc.type = LincsCommand::evPRINT_MESSAGE; 
	lc.msg  = &smsg;
	_upstream->Handle( lc ); 
}


void LinCS::GetValue( ICommand& c ) 
{ 
    value *l = dynamic_cast<value*>( static_cast<LincsCommand*>(&c)->e );
    
	cfgs_session *sess = cfgs_connect();
    if ( !sess ) {
        PrintMessage( "Could not connect!" );
        return;
    }
    
    cfgs_entry *val = cfgs_getval( sess, (l->getVal(CFGS_EA_NAME)).c_str(), 
            NULL/*FIXME: layer*/ ); 
    if ( !val ) {
        //PrintMessage( "No such value: " +  l->getVal(CFGS_EA_NAME) );
	    cfgs_disconnect( sess );
        l->clear();
        return;
    }
    
    l->clear();
    for ( cfgs_pair *p=val->attr; p; p=p->next ) {
        char *n, *v; 
    
        n = p->first;
        v = p->second;
        l->setAttrib( SAFE(n), SAFE(v) );
    }
    CFGST_DLIST_FREE( val, cfgs_entry_free );
    
    cfgs_disconnect( sess );
}


void LinCS::SetValue( ICommand& c ) 
{ 
    value *l = dynamic_cast<value*>( static_cast<LincsCommand*>(&c)->e );
    cfgs_entry *e = cfgs_entry_new();
    
    if ( !l || !e ) {
        //FIXME
        return;
    }
    
    for ( entry::attr_map_iter it = l->begin(); it != l->end(); it++ ) { 
        cfgs_entry_add_attr( e, it->first.c_str(), it->second.c_str() );
        //PrintMessage( it->first + ":" + it->second );
    }
                  
	cfgs_session *sess = cfgs_connect();
    if ( !sess ) {
        cfgs_entry_free( e );
        PrintMessage( "Could not connect!" ); //FIXME
        return;
    }
    
    int ret = cfgs_setval( sess, e ); 
    if ( ret != 1 ) {
        PrintMessage( "cfgs_setval failed!" ); //FIXME
    }
    
    cfgs_entry_free( e );
    cfgs_disconnect( sess );
}


void LinCS::GetValueSub( ICommand& c ) 
{ 
    navigator *n = static_cast<LincsCommand*>(&c)->n;
	
    cfgs_session *sess = cfgs_connect();
    if ( !sess ) {
        PrintMessage( "Could not connect!" );
        return;
    }
    
    const char *valuen = n->_value.c_str();
    const char *layern = n->_layer.c_str();
    cfgs_str *subs = cfgs_getsubvals( sess, valuen, NULL/*FIXME layern*/ );
    n->clear();
    for ( cfgs_str *p=subs; p; p=p->next ) {
        n->add_subkey( p->name );
    }
    CFGST_DLIST_FREE( subs, cfgs_str_free );
    
    cfgs_disconnect( sess );
        
    //static_cast<LincsCommand*>(&c)->n->Navigate(); //FIXME: get rid of
}


void LinCS::GetLayer( ICommand& c ) 
{ 
    PrintMessage( "FIXME LinCS::GetLayer " 
        + static_cast<LincsCommand*>(&c)->e->getVal(CFGS_EA_NAME) ); 
}


void LinCS::SetLayer( ICommand& c ) 
{ 
    PrintMessage( "FIXME LinCS::SetLayer "
        + static_cast<LincsCommand*>(&c)->e->getVal(CFGS_EA_NAME) ); 
}


void LinCS::GetLayerSub( ICommand& c ) 
{ 
    navigator *n = static_cast<LincsCommand*>(&c)->n;
	
	cfgs_session *sess = cfgs_connect();
    if ( !sess ) {
        PrintMessage( "Could not connect!" );
        return;
    }
    
    const char *layern = n->_value.c_str();
    cfgs_str *subs = cfgs_getsublayers( sess, layern );
    n->clear();
    for ( cfgs_str *p=subs; p; p=p->next ) {
        n->add_subkey( p->name );
    }
    CFGST_DLIST_FREE( subs, cfgs_str_free );
    
    cfgs_disconnect( sess );
        
    //static_cast<LincsCommand*>(&c)->n->Navigate(); //FIXME: get rid of
}

void LinCS::GetInfos( ICommand& c ) 
{ 
    string *infos = static_cast<LincsCommand*>(&c)->i; 

    cfgs_session *sess = cfgs_connect();
    if ( !sess ) {
        PrintMessage( "Could not connect!" );
        return;
    }
    
    cfgs_str *i = cfgs_getinfos( sess );

    *infos = i && i->name ? i->name : ""; 

    cfgs_disconnect( sess );
    CFGST_DLIST_FREE( i, cfgs_str_free );
}



