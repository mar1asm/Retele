#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int startsWith ( const char *a, const char *b ) {
  if ( strncmp ( a, b, strlen ( b ) ) == 0 )
    return 1;
  return 0;
}

int main ( ) {
  int exit = 0;
  // while ( ! exit ) {
  printf ( "Selectati comanda:\n1."
           "login [nume]\n2.get-logged-users\n3.get-proc-info "
           "[pid]\n4.logout\n5.quit\n" );
  char *comanda = NULL;
  size_t len = 0;
  char *raspuns;
  int fd;
  do {
    fd = open ( "./FIFO", O_WRONLY ); // check for errors

    int size = getline ( &comanda, &len, stdin );

    if ( ( write ( fd, &size, sizeof ( int ) ) < 0 ) ) {
      printf ( "[Client]Eroare la scriere in FIFO\n" );
      return -1;
    }

    if ( ( write ( fd, comanda, size ) < 0 ) ) {
      printf ( "[Client]Eroare la scriere in FIFO\n" );
      return -1;
    }

    close ( fd );

    fd = open ( "./FIFO", O_RDONLY ); // check for errors

    if ( ( read ( fd, &size, sizeof ( int ) ) ) < 0 ) {
      printf ( "[Client]Eroare la citire din FIFO\n" );
      return -1;
    }
    raspuns = ( char * ) malloc ( ( size + 1 ) );
    if ( ( read ( fd, raspuns, size ) ) < 0 ) {
      printf ( "[Client]Eroare la citire din FIFO\n" );
      free ( raspuns );
      return -1;
    }
    close ( fd );
    printf ( "[Client] Raspuns server: %s\n", raspuns );
    if ( startsWith ( comanda, "login" ) ) {
    }
    if ( startsWith ( comanda, "get-logged-users" ) ) {
    }
    if ( startsWith ( comanda, "login" ) ) {
    }
    if ( startsWith ( comanda, "logout" ) ) {
    }
    if ( startsWith ( comanda, "logout" ) ) {
    }
    if ( startsWith ( comanda, "quit" ) ) {
    }
    free ( raspuns );
  } while ( strcmp ( raspuns, "quit" ) );
}