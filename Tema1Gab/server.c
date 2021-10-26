/* recDataFifo.c
  Exemplu de comunicare intre procese aflate pe aceeasi masina folosind
  FIFO-uri. Programul curent citeste date dintr-un FIFO.

  Autor: Lenuta Alboaie <adria@infoiasi.ro> (c)2009
*/
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

  mknod ( FIFO_NAME, 0666, 0 );

  printf ( "Astept sa scrie cineva...\n" );
  fd = open ( FIFO_NAME, O_RDONLY );
  printf ( "A venit cineva:\n" );

  do {
    if ( ( num = read ( fd, s, 300 ) ) == -1 )
      perror ( "Eroare la citirea din FIFO!" );
    else {
      s[ num ] = '\0';
      printf ( "S-au citit din FIFO %d bytes: \"%s\"\n", num, s );
    }
  } while ( num > 0 );
}
