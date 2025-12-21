/*
    news vigilant.cpp  $Id: sendnews.cpp,v 1.1 2004/03/05 18:51:33 amelinte Exp $
*/
/*
	Three different functionlities built in:
	  -if called by name amail, will post mail
	  -if called by name anews, will post news
	  -if called by msnews will post to a bunch of groups

    M$: create an empty win32 console application called proxy, add this file,
    link with ws2_32.lib and compile. Use invoker.exe to set it up an NT service
    or use KEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run on
    95/98

    UNIX (Solaris tested):
    g++ -o vigilant  -DSOLTHREAD -lthread -lsocket -lnsl vigilant.cpp
        with solaris threads
    g++ -o vigilant  -lpthread -lsocket -lnsl vigilant.cpp
        with posix threads, all Unices?
    and ./vigilant &
	
	LINUX
	g++ -o sendnews sendnews.cpp

    Define _DEBUG for more verbose log and _USE_GRX to use a GNU rx regular
    expressions and link with lrx.a
    g++ -o vigilant -D_USE_GRX -D_DEBUG -I./rx-1.5/rx -lpthread -lsocket -lnsl -L./rx-1.5/rx -lrx vigilant.cpp
    Using GNU rx 1.5
	
*/

//stl debug identificators truncated to 255
#pragma warning( disable : 4786 ) 

#define _REENTRANT

///#define _DEBUG
///#define WIN32
//#define _USE_GRX

#include <sys/types.h>

#ifndef WIN32 //UNIX
#  include <sys/errno.h>
#  include <sys/socket.h>
#  include <sys/time.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <sys/uio.h>
#  include <unistd.h>
# ifdef SOLTHREAD
#  include <thread.h> 
# else
#  include <pthread.h>
# endif
#else /* win */
#  include <windows.h>
#  include <direct.h>
#  include <io.h>
#  define thread_t DWORD
#  define sockaddr SOCKADDR
#endif

/*
 *   Windoze
 */
#ifndef WIN32 //UNIX
#  define FAR
#  define TRUE   (1)
#  define FALSE  (0)
#  define LPVOID void*
#endif

/*
 *   theading fixes
 */
#ifndef WIN32 //UNIX
#   ifndef SOLTHREAD
      //pthreads
#     define thread_t       pthread_t
#     define THR_SELF()     pthread_self()
#     define THR_DETACHED   (0x0)
#     define EXIT_THREAD(x) pthread_exit((void *)x);
#   else //solthreads
#     define THR_SELF()     thr_self()
#     define EXIT_THREAD(x) thr_exit((void *)x);
#   endif
#else /* win */
#  define THR_SELF()      GetCurrentThread()
#  define THR_DETACHED    (0x0)
#  define EXIT_THREAD(x)  ExitThread(x);
#endif

/*
 *   socket fixes
 */
#ifndef WIN32 //UNIX
#  define SOCKET int
#  define SOCKET_ERROR (-1)
#  define UINT   unsigned int
#  define WSAENOTCONN ENOTCONN
//#  define FD_SET fd_set
   typedef void* (*LPTHREAD_START_ROUTINE)(void*);
#  define LPSOCKADDR struct sockaddr*
#  define   CLOSESOCKET(x)  close(x)
#else
#  define   CLOSESOCKET(x)  closesocket(x)
#endif


#ifdef _USE_GRX
  extern "C" {
#  include "rxposix.h"
  }
#endif

#include <string>
#include <vector>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <ctime>

#include <iostream>

using namespace std;


int   readLine( SOCKET s, char* buf, int buflen );
unsigned long nameToAddr( const char* name );
int   fullsend( SOCKET mysocfd, const char FAR* pBufTextOut, int lTextLen, int flags=0 );
int   fullrecv( SOCKET mysocfd, char FAR* pBufTextIn, int lTextLen, int flags );
char* strlwr( char* pIn );
SOCKET open_conn( const char* host, unsigned short port ); //connect
int   log( char* fmt, ... );
int   error( char* fmt, ... ); //log error
char* loadFile( const char*, size_t& );

#ifdef _DEBUG
#  define DBG(x)  x
#else
#  define DBG(x)
#endif

//====================================================================
char *gGroups[] = {
	"soc.culture.romanian",
	"soc.culture.usa",
	"soc.culture.canada",
	"soc.culture.france",
	"soc.culture.french",
	"soc.culture.austria",
	"soc.culture.belgium",
	"soc.culture.british",
	"soc.culture.bulgaria",
	"soc.culture.ecsd",
	"soc.culture.europe",
	"soc.culture.european",
	"soc.culture.german",
	"soc.culture.greek",
	"soc.culture.intercultural",
	"soc.culture.israel",
	"soc.culture.italian",
	"soc.culture.magyar",
	"soc.culture.misc",
	"soc.culture.multicultural",
	"soc.culture.netherlands",
	"soc.culture.nordic",
	"soc.culture.occitan",
	"soc.culture.polish",
	"soc.culture.portuguese",
	"soc.culture.spain",
	"soc.culture.swiss",
	"soc.culture.us",
	//"soc.culture.",
	NULL,
};

//====================================================================
#define LOGFILE      "vigilant.log"
#define LINESIZE     513 //512 cf to rfc977
#define EOA          "\r\n.\r\n"  //end of article signaled

bool   keeplog    = true;

//====================================================================
typedef char* va_list_;
#define va_start_(arg_ptr, first) arg_ptr = (va_list_)&first + sizeof(first)
#define va_arg_(arg_ptr, type)    ((type*)(arg_ptr += sizeof(type)))[-1]
#define va_end_(arg_ptr)          /* empty */

/*-----------------------------------------------------------------*/
/* create a new thread */
#ifndef WIN32 //UNIX
int create_thread( void *stack_base, size_t stack_size,
                   void *(*start_func)(void *), 
                   void *arg, long flags,
                   thread_t *new_thread_ID
                 )
{
    int rez = 0;
#else //win
HANDLE create_thread( LPSECURITY_ATTRIBUTES stack_base, DWORD stack_size,
                      LPTHREAD_START_ROUTINE start_func,
                      LPVOID arg, DWORD  flags,
                      LPDWORD new_thread_ID
                    )
{
    HANDLE rez = 0;
#endif

#ifndef WIN32 //UNIX
#  ifdef SOLTHREAD
    rez = thr_create(stack_base, stack_size, start_func, arg, flags, new_thread_ID);
#  else
    //pthreads
    rez = pthread_create( new_thread_ID, (const pthread_attr_t *)stack_base, 
	                      start_func, arg );
#  endif
#else
    rez = CreateThread (NULL, stack_size, start_func, arg, flags, new_thread_ID);
#endif

    return rez;
}
/*-----------------------------------------------------------------*/
#ifdef WIN32
int strncasecmp(const char *s1, const char *s2, int n)
{
    assert( s1 != NULL );
    assert( s2 != NULL );

    return strnicmp( s1, s2, n );
}
#endif

/*-----------------------------------------------------------------*/
char* strlwr( char* pIn )
{
    assert( pIn != NULL );

    char* p = pIn;

    for ( p = pIn; p < pIn + strlen( pIn ); p++  )
    {
        *p = tolower( *p );
    }

    return pIn;
}

/*-----------------------------------------------------------------*/
int readLine( SOCKET s, char* buf, int buflen )
{
    int   nCharsRead = 0;
    char* pBufStart = buf;

    assert( buf != NULL );

    do
    {
        nCharsRead = fullrecv( s, buf, 1, 0 ); //flags = 0??
    }
    while ( nCharsRead > 0 && nCharsRead < buflen && *buf++ != '\n' );
    *buf = '\0';

    log("%d<--%s", s, pBufStart); //there is a \n at the end
    return ( nCharsRead > 0 );
}
/*-----------------------------------------------------------------*/
unsigned long nameToAddr( const char* name )
{
    assert( name != NULL );

    char hostName[ LINESIZE ];
    struct hostent* hostStruct = NULL;
    struct in_addr* hostNode   = NULL;

    //begin with a digit, assume IP string
    if ( isdigit(name[0]) )
        return( inet_addr(name) );
    else
        strcpy( hostName, name );

    hostStruct = gethostbyname( hostName );
    if ( NULL == hostStruct )
        return( 0 ); //not found

    //extract IP
    hostNode = (struct in_addr*) hostStruct->h_addr;
    DBG( log( "\tIP: %s\n", inet_ntoa(*hostNode) ); );
    return( hostNode->s_addr );
}
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
int fullsend( SOCKET mysocfd, 
              const char FAR* pBufTextOut, 
			  int lTextLen, 
              int flags )
{
    assert( pBufTextOut != NULL );

    int sendsofar = 0, lastsend = 0;

    while ( sendsofar < lTextLen )
    {
        lastsend  = send( mysocfd,
                          pBufTextOut + sendsofar*sizeof(char),
                          lTextLen - sendsofar,
                          flags );
        if ( lastsend == SOCKET_ERROR )
        {
#ifndef WIN32 //UNIX
            perror( "send" );
#else
            error("Send err: %d", WSAGetLastError());
#endif
            return lastsend;
        }
        else
            sendsofar += lastsend;
    }
    //DBG( log( "%d-->%s", mysocfd, pBufTextOut ); );

    return sendsofar;
}
/*-----------------------------------------------------------------*/
int fullrecv( SOCKET mysocfd, char FAR* pBufTextIn, int lTextLen, int flags )
{
    assert( pBufTextIn != NULL );

    int recvsofar = 0, lastrecv = 0;

    while ( recvsofar < lTextLen )
    {
        lastrecv  = recv( mysocfd,
                          pBufTextIn + recvsofar*sizeof(char),
                          lTextLen - recvsofar,
                          flags );
        if ( lastrecv == 0 ) //socket closed
            return recvsofar;
        else if ( lastrecv == SOCKET_ERROR )
            return lastrecv;
        else
            recvsofar += lastrecv;
    }

    return recvsofar;
}
/*-----------------------------------------------------------------*/
SOCKET open_conn( const char* hostName, unsigned short usPort )
{
    assert( hostName != NULL );

    SOCKET rsrv = 0;
    struct sockaddr_in rsrvINETAddress;
    LPSOCKADDR rsrvSockAddrPtr = NULL;

    rsrvSockAddrPtr = (LPSOCKADDR)&rsrvINETAddress;
    memset((char *) &rsrvINETAddress, 0, sizeof(rsrvINETAddress));
    rsrvINETAddress.sin_family = AF_INET;
    rsrvINETAddress.sin_addr.s_addr = nameToAddr( hostName );
    rsrvINETAddress.sin_port = htons( usPort );
    rsrv = socket( AF_INET, SOCK_STREAM, 0 );

    if ( -1 == connect(rsrv, (LPSOCKADDR)rsrvSockAddrPtr, sizeof(rsrvINETAddress)) )
    {
        error("open_conn: Cannot connect %s %d\n", hostName, usPort);
        DBG( perror( "Connect to server:" ); );
        return 0;
    }
    DBG( log("Connect to %s %d\n", hostName, usPort); );

    return rsrv;
}
/*-----------------------------------------------------------------*/
int log( char* fmt, ... )
{
    int     rez = EOF;
    FILE*   pfl = NULL;
    va_list args;

    if ( !keeplog ) //the diff from error
        return EOF;

    va_start( args, fmt );
    if ( (pfl=fopen(LOGFILE, "a+b")) == NULL )
    {
        perror("Open log file ");
        return EOF;
    }
    rez = vfprintf( pfl, fmt, args );
    va_end( args );
    fclose( pfl );

    return rez;
}
/*-----------------------------------------------------------------*/
int error( char* fmt, ... )
{
    int     rez = EOF;
    FILE*   pfl = NULL;
    va_list args;

    va_start( args, fmt );
    if ( (pfl=fopen(LOGFILE, "a+")) == NULL )
    {
        perror("Open log file ");
        return EOF;
    }
    rez = vfprintf( pfl, fmt, args );
    va_end( args );
    fclose( pfl );

    return rez;
}
//====================================================================
char* strToLower( char* pLine )
{
    assert( pLine != NULL );

    for ( int i=0; i<strlen(pLine); i++ )
    {
        pLine[i] = tolower( pLine[i] );
    }
    return pLine;
}
//====================================================================
char* trimString( char* pLine )
{
    assert( pLine != NULL );

    while( pLine[0] == ' ' ||
           pLine[0] == '\t'
         )
        pLine++;

    while( pLine[strlen(pLine)-1] == ' '  ||
           pLine[strlen(pLine)-1] == '\t' ||
           pLine[strlen(pLine)-1] == '\r' ||
           pLine[strlen(pLine)-1] == '\n'
         )
        pLine[ strlen(pLine) -1 ] = '\0';

    return pLine;
}
//====================================================================
int readLine( FILE* pf, char* buf, int iBSize )
{
    assert( pf != NULL );
    assert( buf != NULL );

    int  iRead = 0;
    char ch = ' ';

    memset( buf, '\0', iBSize );
    while( !feof(pf) &&  ch != '\n' && iRead < iBSize )
    {
        ch = fgetc( pf );
        buf[ iRead++ ] = ch;
    }

    //DBG( log("\n  Read line: %s", buf); )
    return iRead;
}
//====================================================================
// special for nntp because of LINESIZE
int writeLine( SOCKET s, char* fmt, ... )
{
    char     line[LINESIZE];
    va_list  args;

    memset( line, '\0', LINESIZE );
    va_start( args, fmt );
    vsprintf( line, fmt, args );
    va_end( args );
    log("%d-->%s", s, line);

    return( fullsend( s, line, strlen(line) ) );
}
//====================================================================
int extractCode( char* line )
{
    assert( line != NULL );

    char code[4];
    strncpy( code, line, 3 );

    DBG( log("\tcode: %d\n", atoi(code)); );
    return atoi( code );
}
//====================================================================
// wait 100 ms on s and ret
int wait100ms( SOCKET rsrv )
{
    struct  timeval  tv100 = {0, 100}; //sleep 100 us
    fd_set  fds;
    int     ready = 0;

    FD_ZERO( &fds );
    FD_SET( rsrv, &fds );
    ready = select(rsrv+1, &fds, NULL, NULL, &tv100); //sleep

    return ready;
}
//====================================================================
#ifdef WIN32
void sleep( unsigned secs )
{
    Sleep( secs*1000 );
}
#endif
//====================================================================
void help_sendnews( void )
{
    cout << endl ;
	cout << "Usage: anews <file>" << endl ;
	cout << "Usage: multisend news 'msnews <file>'" << endl ;
    cout << "You can set env vars NEWSHOST, NEWSGROUP, NEWSFROM SUBJECT" << endl ;
	cout << "Usage: amail <file>" << endl ;
    cout << "You can set env vars MAILHOST, MAILFROM MAILTO SUBJECT" << endl ;
	cout << endl ;
}

///? fix this crappy global later
string sendnews_param;
char* sendnews_get_param( const char* param )
{
    char*  p = getenv( param );
	char   buf[ LINESIZE+1 ];
	
	if ( !p )
	{
	    cout << "Enter value for " << param << " : " ;
		cin.getline( buf, LINESIZE );
		sendnews_param = buf;
	}
	else 
	    sendnews_param = p;
	
    return const_cast<char*>(sendnews_param.c_str());
}

int sendnews_get_random( void )
{
	time_t    now;
	int       ret;

	time( &now );
	now -= 2156;
	now  = now % 10000;
	
	srandom( (unsigned int)now );
	ret = (int)random();
	//ret = ret % 100000;

    return (ret+538);
}
//====================================================================
/* surpise headers !!

From root@info.polymtl.ca  Mon Sep  3 14:35:54 2001
Received: from localhost (amelinte@shockley.info.polymtl.ca [132.207.12.151])
	by info.polymtl.ca (8.11.3/8.11.3) with SMTP id f83IZsP02955
	for amelinte@info.polymtl.ca; Mon, 3 Sep 2001 14:35:54 -0400 (EDT)
From: Super-User <root@info.polymtl.ca>
Date: Monday, 03-Sep-01 12:51:36 EST
Subject: sdfg4
Message-ID: <707067858@zulu.UUCP>
To: undisclosed-recipients:;
Status: R
*/
//
// first argument is the filename that contains the text to be sent
//
int do_sendmail ( int argc, char** argv, char** env )
{
    SOCKET s = -1;
    char   line[ LINESIZE ]; // line read from server
	char   date[ LINESIZE ];
	char   msgid[ LINESIZE ]; 
    int    acode = 0;
	time_t    now;
	struct tm tnow;
	int       mid = sendnews_get_random();
	FILE*  fmsg = NULL;

	string msrv    = sendnews_get_param( "MAILHOST" );  
	string from    = sendnews_get_param( "MAILFROM" );  
	string subject = sendnews_get_param( "SUBJECT" );  
	string to      = sendnews_get_param( "MAILTO" );  

	time( &now );
	now -= mid%7200;
	strftime( date, LINESIZE-1, "%A, %d-%b-%g %T EST", localtime( &now ) );
	sprintf( msgid, "%d@zulu.UUCP", mid );

	if ( argc != 2 )
	    { help_sendnews(); return 1; }


    //open conn
    if ( !((s = open_conn( msrv.c_str(), 25 )) > 0 ) )
        perror("Conn to mail srv: "), exit(1);
    //do protocol
    readLine( s, line, LINESIZE );
    if ( (acode = extractCode( line )) != 220 )
        goto cleanup_;
    writeLine( s, "HELO localhost\r\n" ); ///repalce with hostname
    readLine( s, line, LINESIZE );
    if ( (acode = extractCode( line )) != 250 )
        goto cleanup_;
    writeLine( s, "MAIL FROM: %s\r\n", from.c_str() ); ///repalce with m adr
    readLine( s, line, LINESIZE );
    if ( (acode = extractCode( line )) != 250 )
        goto cleanup_;
    writeLine( s, "RCPT TO: %s\r\n", to.c_str() );
    readLine( s, line, LINESIZE );
    if ( (acode = extractCode( line )) != 250 )
        goto cleanup_;
    writeLine( s, "DATA\r\n" );
    readLine( s, line, LINESIZE );
    if ( (acode = extractCode( line )) != 354 )
        goto cleanup_;

    //send msg
    DBG( printf("*** Sending message...\n"); );

        //header
        writeLine( s, "From: %s\r\n", from.c_str() ); 
        writeLine( s, "Date: %s\r\n", date ); 
        writeLine( s, "Subject: %s\r\n", subject.c_str() ); 
        writeLine( s, "Message-ID: <%s>\r\n", msgid ); 
        writeLine( s, "\r\n" ); 
		
		//body
    	fmsg = fopen( argv[1], "rt" );
	    if ( fmsg )
		{
		    #define BUFL 1024
		    char    buf[ BUFL+1 ];
			size_t  nread=0, nwrite=0;
		    while ( !feof(fmsg) )
			{
			    nread = fread( (void*)buf, 1, BUFL, fmsg );
    			buf[ nread ] = '\0';
				//cout << nread << ":" << buf << endl;
				nwrite = fullsend( s, buf, nread, 0 );
				assert( nread == nwrite );
			}
			fclose( fmsg );
		}
    	else
	        perror( "" );
			
		//EOT
		writeLine( s, EOA );

    //close conn
    readLine( s, line, LINESIZE );
    if ( (acode = extractCode( line )) != 250 )
        goto cleanup_;
    writeLine( s, "QUIT\r\n" );
    readLine( s, line, LINESIZE );
    if ( (acode = extractCode( line )) != 221 )
        goto cleanup_;

cleanup_:
    ;
    if ( s > 0 )
        CLOSESOCKET( s );
    return(0);
}
//====================================================================
/*
Relay-Version: version B 2.10 2/13/83; site cbosgd.UUCP
Posting-Version: version B 2.10 2/13/83; site eagle.UUCP
From: jerry@eagle.uucp (Jerry Schwarz)
Date: Friday, 19-Nov-82 16:14:55 EST
Newsgroups: net.general
Subject: Usenet Etiquette -- Please Read
Message-ID: <642@eagle.UUCP>
Path: cbosgd!mhuxj!mhuxt!eagle!jerry

Followup-To: net.news
Expires: Saturday, 1-Jan-83 00:00:00 EST
Date-Received: Friday, 19-Nov-82 16:59:30 EST
Organization: Bell Labs, Murray Hill
*/	 
// this macro craps g++
//#define END_SEND_SESSION                   \
//    {                                      \
//        writeLine( nssock, "QUIT\r\n" );   \
//        CLOSESOCKET( nssock );             \
//	}
void END_SEND_SESSION( SOCKET nssock, int err ) 
{ 
    writeLine( nssock, "QUIT\r\n" ); 
    CLOSESOCKET( nssock ); 
	exit( err );
}
//
// first argument is the filename that contains the text to be sent
//
int do_sendnews ( int argc, char** argv, char** env, string& group )
{
    SOCKET nssock = -1;
    char   line[ LINESIZE ]; // line read from server
	char   date[ LINESIZE ];
	char   msgid[ LINESIZE ]; 
    int    acode = 0;
	time_t    now;
	struct tm tnow;
	int       mid = sendnews_get_random();
	FILE*  fmsg = NULL;

	string nsrv    = sendnews_get_param( "NEWSHOST" );  //"news.polymtl.ca"
	string from    = sendnews_get_param( "NEWSFROM" );  //"jerry@zulu.uucp"
	string subject = sendnews_get_param( "SUBJECT" );  
	
	sleep( 1 );
	time( &now );
	now -= mid%7200;
	strftime( date, LINESIZE-1, "%A, %d-%b-%g %T EST", localtime( &now ) );
	sprintf( msgid, "%d@zulu.UUCP", mid );
	
	if ( argc != 2 )
	    { help_sendnews(); return 1; }

    if ( (nssock = open_conn( nsrv.c_str(), 119)) > 0 )
    {
        readLine( nssock, line, LINESIZE );
        acode = extractCode( line );
        if ( acode != 200 ) //posting allowed
            END_SEND_SESSION( nssock, 1 );

        writeLine( nssock, "GROUP %s\r\n", group.c_str() ); 
        readLine( nssock, line, LINESIZE );
        acode = extractCode( line );
        if ( acode != 211 ) //group exists
            END_SEND_SESSION( nssock, 1 );
		
        writeLine( nssock, "POST\r\n" ); 
        readLine( nssock, line, LINESIZE );
        acode = extractCode( line );
        if ( acode != 340 ) //posting permitted
            END_SEND_SESSION( nssock, 1 );

        //header
        writeLine( nssock, "Relay-Version: version B 2.10 2/13/83; site cbosgd.UUCP\r\n" ); 
        writeLine( nssock, "Posting-Version: version B 2.10 2/13/83; site zulu.UUCP\r\n" ); 
        writeLine( nssock, "From: %s\r\n", from.c_str() ); 
        writeLine( nssock, "Date: %s\r\n", date ); 
        writeLine( nssock, "Newsgroups: %s\r\n", group.c_str() ); 
        writeLine( nssock, "Subject: %s\r\n", subject.c_str() ); 
        writeLine( nssock, "Message-ID: <%s>\r\n", msgid ); 
        writeLine( nssock, "Path: alpha!beta!gamma!zada!null\r\n" ); 
        writeLine( nssock, "\r\n" ); 
		
		//body
    	fmsg = fopen( argv[1], "rt" );
	    if ( fmsg )
		{
		    #define BUFL 1024
		    char    buf[ BUFL+1 ];
			size_t  nread=0, nwrite=0;
		    while ( !feof(fmsg) )
			{
			    nread = fread( (void*)buf, 1, BUFL, fmsg );
    			buf[ nread ] = '\0';
				//cout << nread << ":" << buf << endl;
				nwrite = fullsend( nssock, buf, nread, 0 );
				assert( nread == nwrite );
			}
			fclose( fmsg );
		}
    	else
	        perror( "" );
			
		//EOT
		writeLine( nssock, EOA );
        readLine( nssock, line, LINESIZE );
        acode = extractCode( line );
        if ( acode != 240 ) //article posted ok
            DBG( log("*** Error posting: %s \n", line ); );
		
        // QUIT would be appreciated...
		END_SEND_SESSION( nssock, 0 );
    } // if conn

    return 1;
}

int do_sendmultinews ( int argc, char** argv, char** env )
{
	char **grp = gGroups;
	
	//sleep( 30 );
	sleep( 9*60+34 );
	while ( *grp ) {
	    string sgrp = *grp;
		DBG( log("Posting to '%s' \n", sgrp.c_str() ); );
		sleep( 1 );
		do_sendnews( argc, argv, env, sgrp );
		grp++;
	}
	DBG( log("\n*** Done ***\n"); );

    return 1;
}

//====================================================================
int main ( int argc, char** argv, char** env )
{
    ///? bug: will match if one of the folders of the full path contains 
	//   amail
    if ( strstr( argv[0], "amail" ) ) {
        return do_sendmail( argc, argv, env );
    } else if ( strstr( argv[0], "msnews" ) ) {
        return do_sendmultinews( argc, argv, env );
	} else {
		string group  = sendnews_get_param( "NEWSGROUP" ); //"concordia.forsale"
        return do_sendnews( argc, argv, env, group );
	}
}
//====================================================================
