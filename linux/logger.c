
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

#define IPADDR    "192.168.1.152"
#define PORT       23

#define LINESIZE   256
#define SA struct  sockaddr

#define USERNAME   "root",
#define PASSWORD   "rootpassword"
#define LOGFILE    "charge.log"


void func(int sockfd)
{
    char buff[LINESIZE];

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
    char buff[LINESIZE];

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
    char buff[LINESIZE];

    sleep(1);
    readLine( sockfd, buff, sizeof(buff) );

    sendLine( sockfd, username );
    readLine( sockfd, buff, sizeof(buff) );

    sendLine( sockfd, password );
    readLine( sockfd, buff, sizeof(buff) );
}


bool logdata( int sockfd, char *buff, int size )
{
    char  tmp1[LINESIZE], tmp2[LINESIZE];

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
    char buff[LINESIZE];

    FILE *logfile = fopen( filename, "a+" );
    if ( !logfile ) exit;

//  Require ENTER button !!!
    while( !kbhit() )
    {
        bool boo = logdata( sockfd, buff, LINESIZE );
//      if ( boo )
        {
             printf( "%s", buff );
             fwrite( buff, 1, strlen(buff), logfile );
        }
    }
    fclose( logfile );

}


int main( int argc, char *argv[] )
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
