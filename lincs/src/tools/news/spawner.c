/*
 *  create a process from the command line
 *
 */


#include <windows.h>
#include <stdio.h>


#define PROC_CMD_SIZE  (1024*10)
char   g_ProcCmd[ PROC_CMD_SIZE ];



BOOL RunCommand( char *cmd )
{
    STARTUPINFO         sinfo;
    PROCESS_INFORMATION pinfo;
    BOOL                ret;

    sinfo.cb = sizeof(STARTUPINFO);
    sinfo.lpReserved = NULL;
    sinfo.lpReserved2 = NULL;
    sinfo.cbReserved2 = 0;
    sinfo.lpDesktop = NULL;
    sinfo.lpTitle = NULL;
    sinfo.dwFlags = 0;
    sinfo.dwX = 0;
    sinfo.dwY = 0;
    sinfo.dwFillAttribute = 0;
    sinfo.wShowWindow = SW_SHOW;

    // non-zero = success
    ret = CreateProcess( NULL,  cmd,   NULL, 
                         NULL,  TRUE,  DETACHED_PROCESS, 
                         NULL,  NULL,  &sinfo, 
                         &pinfo );

    if ( ret == FALSE ) {
        LPVOID lpMsgBuf;

        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
        );
        printf( "\n%s\n", lpMsgBuf );
        LocalFree( lpMsgBuf );
    }

    return ret;
    
}


int
main( int argc, char* argv[] )
{
    char **crtarg;
    BOOL ret;

    memset( g_ProcCmd, '\0', PROC_CMD_SIZE );
    for ( crtarg=argv+1; *crtarg; crtarg++ ) {
        strcat( g_ProcCmd, *crtarg );
        strcat( g_ProcCmd, " " );
    }
    
    //printf( "\n*%s* \n", g_ProcCmd );
    ret = RunCommand( g_ProcCmd );

    return !ret;
}
