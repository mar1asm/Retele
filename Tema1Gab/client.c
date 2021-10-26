#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FIFO_NAME "MyTest_FIFO"

int main ( ) {
  char s[ 300 ];
  int num, fd;

  printf ( "Astept cititori...\n" );
  fd = open ( FIFO_NAME, O_WRONLY );
  printf ( "Am un cititor....introduceti ceva..\n" );

  while ( gets ( s ), ! feof ( stdin ) ) {
    if ( ( num = write ( fd, s, strlen ( s ) ) ) == -1 )
      perror ( "Problema la scriere in FIFO!" );
    else
      printf ( "S-au scris in FIFO %d bytes\n", num );
  }
}