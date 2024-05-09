#if 0

#include  <arpa/inet.h>      // MinGW - inet_addr()
#include  <netdb.h>          // MinGW
#include  <stdio.h>
#include  <stdlib.h>
#include  <stdint.h>
#include  <stdbool.h>
#include  <string.h>
#include  <strings.h>        // bzero()
#include  <sys/socket.h>     // MinGW
#include  <unistd.h>         // read(), write(), close()
#include  "logger.h"

#define IPADDR    "192.168.2.1"
#define PORT       23

#define DEFAULT_BUFLEN   256
#define SA struct  sockaddr

#define USERNAME   "root"
#define PASSWORD   "rootpassword"
#define LOGFILE    "charge.log"


void func(int sockfd)
{
    char buff[DEFAULT_BUFLEN];

    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        int n = 0;
        while ( 1 )
        {
            if ( (buff[n++] = getchar()) == '\n' ) {
                break;
            }
        }
        write(sockfd, buff, sizeof(buff));

        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}


// https://stackoverflow.com/questions/58360050/detecting-keyboard-key-press-and-release-on-linux
int kbhit (void)
{
    struct timeval tv;
    fd_set rdfs;

    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    FD_ZERO(&rdfs);
    FD_SET (STDIN_FILENO, &rdfs);

    select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &rdfs);
}


void sendLine( int sockfd, const char *cmd )
{
    char buff[DEFAULT_BUFLEN];

    snprintf( buff, sizeof(buff), "%s\n", cmd );
    write( sockfd, buff, strlen(buff) );
    sleep(1);
}


char *readLine( int sockfd, char *reply, int len )
{
    bzero( reply, len );
    read( sockfd, reply, len );
//  printf( "From Server : <%s>\n", reply );
    return reply;
}


bool stripCRLF( char *txt )
{
    char *endp;

    if ( (endp = strstr(txt,  "\r\n")) ) {
         *endp = 0;
    }
    return endp ? true : false;
}


int login( int sockfd, const char *username, const char *password )
{
    char buff[DEFAULT_BUFLEN];

    sleep(1);
    readLine( sockfd, buff, sizeof(buff) );

    sendLine( sockfd, username );
    readLine( sockfd, buff, sizeof(buff) );

    sendLine( sockfd, password );
    readLine( sockfd, buff, sizeof(buff) );
}


bool logdata( int sockfd, char *buff, int size )
{
    char  tmp1[DEFAULT_BUFLEN], tmp2[DEFAULT_BUFLEN];

    char *timestamp;
    char *chargedata;

    memset( tmp1, 0, sizeof(tmp1) );
    memset( tmp2, 0, sizeof(tmp2) );

    sendLine( sockfd, "date" );
    readLine( sockfd, tmp1,  sizeof(tmp1) );
    timestamp  = strstr( tmp1, "20" );         // YEAR: 20xx

    sendLine( sockfd, "charge" );
    readLine( sockfd, tmp2,  sizeof(tmp2) );
    chargedata = strstr(tmp2, "=");

    // NOTE: TCP is stream protocol and do not send allways full text line packets
    // Lines might be fragmented to different TCP packets/ read function(s).
    // Strategy: discard fragmented incomplete reads / packets.

    if ( timestamp && chargedata )
    {
        bool boo1 = stripCRLF(   timestamp );
        bool boo2 = stripCRLF( ++chargedata );

        if ( boo1 && boo2 ) {
            snprintf( buff, size, "%s - %s\n", chargedata, timestamp );
            return true;
        }
        else {
            snprintf( buff, size, "MISSING(CRLF)\n" );
        }
    }
    else
    {
        if ( !timestamp ) {
            snprintf( buff, size, "MISSING(timestamp):<%s>\n", tmp1 );
        }
        if ( !chargedata ) {
            snprintf( buff, size, "MISSING(chargedata):<%s>\n", tmp2 );
        }
    }
    return false;
}


void logger( int sockfd, const char *filename )
{
    char buff[DEFAULT_BUFLEN];

    FILE *logfile = fopen( filename, "a+" );
    if ( !logfile ) exit;

//  Require ENTER button !!!
    while( !kbhit() )
    {
        bool boo = logdata( sockfd, buff, DEFAULT_BUFLEN );
//      if ( boo )
        {
             printf( "%s", buff );
             fwrite( buff, 1, strlen(buff), logfile );
        }
    }
    fclose( logfile );
}


int lnx_main( int argc, char *argv[] )
{
    int                 sockfd,   connfd;
    struct sockaddr_in  servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else {
        printf("Socket successfully created..\n");
    }
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr( IPADDR );
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else {
        printf("connected to the server..\n");
    }

    // function for chat
    // func(sockfd);

    login(  sockfd, USERNAME, PASSWORD );
    logger( sockfd, LOGFILE );

    // close the socket
    close(sockfd);

    return 0;
}


//====================
#else

// https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
//#pragma comment (lib, "Ws2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")


#define DEBUG           0
#define IPADDR          "192.168.2.1"       // argv[1]
#define DEFAULT_PORT    "23"
#define DEFAULT_BUFLEN  512
#define POLL_PERIOD_s   1                   // [s]


int kbhit(void);


void setNonBlock( SOCKET ConnectSocket )
{
    #define O_NONBLOCK 1

    unsigned long  ul = O_NONBLOCK;

    int  nRet = ioctlsocket(ConnectSocket, FIONBIO, (unsigned long *) &ul);
    if (nRet == SOCKET_ERROR)
    {
        // Failed to put the socket into non-blocking mode
        printf("Error: O_NONBLOCK fail\n");
        exit(1);
    }
}


int sendBuffer( SOCKET ConnectSocket, char *sendbuf )
{
    int iResult;

    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return -1;
    }
//  printf("Bytes Sent: %d\n", iResult);
    Sleep( 500 );
    return iResult;
}


int recvBuffer( SOCKET ConnectSocket, char *recvbuf, int recvbuflen )
{
    int iResult;

    memset( recvbuf, 0, recvbuflen );
    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 ) {
            #if DEBUG
            printf("Bytes received: %d\n%s\n", iResult, recvbuf);
            #endif
        }
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else {
            int  errorCode =  WSAGetLastError();
            if ( errorCode == WSAEWOULDBLOCK ) break;
            printf("recv failed with error: %d\n", errorCode);
        }
    } while( iResult > 0 );

    return iResult;
}


void login( SOCKET ConnectSocket )
{
    char recvbuf[DEFAULT_BUFLEN];

    // Receive "Hello" message and prompt "user"
    recvBuffer( ConnectSocket, recvbuf, sizeof(recvbuf) );
    // Send username
    sendBuffer( ConnectSocket, "root\n" );
    // Receive "password" request
    recvBuffer( ConnectSocket, recvbuf, sizeof(recvbuf) );
    // Send password
    sendBuffer( ConnectSocket, "rootpassword\n" );
    // Receive prompt
    recvBuffer( ConnectSocket, recvbuf, sizeof(recvbuf) );
}


int stripCRLF( char *txt )
{
    char *endp;

    if ( (endp = strstr(txt, "\r\n")) ) {
         *endp = 0;
    }
    return endp ? 1 : 0;
}


int logdata( SOCKET ConnectSocket, char *buff, int size )
{
    #define true  1
    #define false 0

    char  tmp1[DEFAULT_BUFLEN], tmp2[DEFAULT_BUFLEN];

    memset( tmp1, 0, sizeof(tmp1) );
    memset( tmp2, 0, sizeof(tmp2) );

//  sendBuffer( ConnectSocket, "date\n" );
    sendBuffer( ConnectSocket, "uptime\n" );
    recvBuffer( ConnectSocket, tmp1,  sizeof(tmp1) );

    sendBuffer( ConnectSocket, "charge\n" );
    recvBuffer( ConnectSocket, tmp2,  sizeof(tmp2) );

//  char *timestamp  = strstr( tmp1, "20" );
    char *timestamp  = strstr( tmp1, "Up" ) + 3;
    char *chargedata = strstr( tmp2, "="  );

    // NOTE: TCP is stream protocol and do not send allways full text line packets
    // Lines might be fragmented to different TCP packets/ read function(s).
    // Strategy: discard fragmented incomplete reads / packets.

    if ( timestamp && chargedata )
    {
        int boo1 = stripCRLF(   timestamp );
        int boo2 = stripCRLF( ++chargedata );

        if ( boo1 && boo2 ) {
            snprintf( buff, size, "%s - %s\n", chargedata, timestamp );
            return true;
        }
        else {
            snprintf( buff, size, "MISSING(CRLF)\n" );
        }
    }
    else
    {
        if ( !timestamp ) {
            snprintf( buff, size, "MISSING(timestamp):<%s>\n", tmp1 );
        }
        if ( !chargedata ) {
            snprintf( buff, size, "MISSING(chargedata):<%s>\n", tmp2 );
        }
    }
    return false;
}


void logger( SOCKET ConnectSocket, const char *filename )
{
    char buff[DEFAULT_BUFLEN];
    int  counter = 0;

    FILE *logfile = fopen( filename, "a+" );
    if ( !logfile ) exit(1);

//  Require ENTER button !!!
    while( !kbhit() )
    {
        // sendBuffer() have 500 ms delay, logdata() send commands: date & charge

        if ( (++counter % POLL_PERIOD_s) == 0 )
        {
            int  boo = logdata( ConnectSocket, buff, DEFAULT_BUFLEN );
            if ( boo )
            {
                 printf( "%s", buff );
                 fwrite( buff, 1, strlen(buff), logfile );
            }
        }
        else
        {
            Sleep( 1000 );
        }
    }
    fclose( logfile );
}


int __cdecl main(int argc, char **argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    char ipaddr[128] = IPADDR;
    char recvbuf[DEFAULT_BUFLEN];
    int  iResult;
    int  recvbuflen = DEFAULT_BUFLEN;

    #if 1
    if (argc > 1) {
        strncpy( ipaddr, argv[1], sizeof(ipaddr) );
    }
    #else
    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }
    #endif

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(ipaddr, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo (%s) failed with error: %d\n", ipaddr, iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    Sleep(1000);

    setNonBlock( ConnectSocket );

    //-----------------------------------------------------------------------

    login(  ConnectSocket );
    #if 1
    logger( ConnectSocket, "logger.log" );
    #else

    // Send "date" command
    sendBuffer( ConnectSocket, "date\n" );
    // Receive "date" command response
    recvBuffer( ConnectSocket, recvbuf, sizeof(recvbuf) );
    // Send "charge" command
    sendBuffer( ConnectSocket, "charge\n" );
    // Receive "date" command response
    recvBuffer( ConnectSocket, recvbuf, sizeof(recvbuf) );
    while ( !kbhit() );
    #endif

    //-----------------------------------------------------------------------

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n%s\n", iResult, recvbuf);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else {
            int  errorCode =  WSAGetLastError();
            if ( errorCode == WSAEWOULDBLOCK ) break;
            printf("recv failed with error: %d\n", errorCode);
        }

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}

#endif
