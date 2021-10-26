#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void client ( int pipe[ 2 ], int sockp[ 2 ] );
void server ( int pipe[ 2 ], int sockp[ 2 ] );

int startsWith ( const char *a, const char *b );

int main ( ) {
  pid_t pid;
  int sockp[ 2 ];
  socketpair ( AF_UNIX, SOCK_STREAM, 0, sockp );

  int pipe1[ 2 ];
  if ( pipe ( pipe1 ) ) {
    printf ( "Eroare la pipe\n" );
    exit ( 0 );
  }

  switch ( pid = fork ( ) ) {
  case -1:
    perror ( "fork" );
    exit ( 1 );
    return 0;
  case 0:
    client ( pipe1, sockp );
    break;
  default:
    server ( pipe1, sockp );
  }
}





int startsWith ( const char *a, const char *b ) {
  if ( strncmp ( a, b, strlen ( b ) ) == 0 )
    return 1;
  return 0;
}
