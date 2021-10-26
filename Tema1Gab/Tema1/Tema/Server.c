#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct User {
  char name[ 30 ];
  int loggedIn;
};

int startsWith ( const char *a, const char *b ) {
  if ( strncmp ( a, b, strlen ( b ) ) == 0 )
    return 1;
  return 0;
}

int login ( char comanda[], struct User *users, int nOfUsers ) {
  int userExists = 0;
  char username[ 100 ];

  char *pch;
  pch = strtok ( comanda, " " );
  if ( pch == NULL )
    return -1;
  pch = strtok ( NULL, " " );
  if ( pch == NULL )
    return -1;
  strcpy ( username, pch );

  for ( int i = 0; i <= nOfUsers; i++ ) {
    if ( ! strcmp ( users[ i ].name, username ) ) {
      users[ i ].loggedIn = 1;
      return 1;
    }
  }
  return 0;
}

int isLoggedIn ( struct User *users, int nOfUsers ) {
  for ( int i = 0; i < nOfUsers; i++ ) {
    if ( users[ i ].loggedIn )
      return 1;
  }
  return 0;
}

int logout ( struct User *users, int nOfUsers ) {
  for ( int i = 0; i < nOfUsers; i++ ) {
    if ( users[ i ].loggedIn == 1 ) {
      users[ i ].loggedIn = 0;
      return 1;
    }
  }
  return 0;
}

int getLoggedUsers ( ) {}

char *getProcInfo ( char comanda[] ) {
  char pid[ 100 ];
  int processID;

  char *pch;
  pch = strtok ( comanda, " " );
  if ( pch == NULL )
    return NULL;
  pch = strtok ( NULL, " " );
  if ( pch == NULL )
    return NULL;
  strcpy ( pid, pch );
  if ( ( processID = strtol ( pid, NULL, 10 ) ) == 0 )
    return NULL;
}

int main ( ) {
  int size;
  char *comanda, *raspuns;
  struct User users[ 100 ];
  int nOfUsers = 0;
  char *thisUser;

  struct stat stats;
  if ( stat ( "./FIFO", &stats ) >= 0 ) {
    if ( unlink ( "./FIFO" ) < 0 ) {
      printf ( "Eroare la FIFO\n" );
      exit ( 0 );
    }
  }
  if ( mkfifo ( "./FIFO", 0666 ) == -1 ) {
    printf ( "Eroare la FIFO\n" );
    exit ( 0 );
  }

  int pipe1[ 2 ];
  if ( pipe ( pipe1 ) ) {
    printf ( "Eroare la pipe\n" );
    exit ( 0 );
  }

  int sockp[ 2 ];
  socketpair ( AF_UNIX, SOCK_STREAM, 0, sockp );

  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t r;

  // numar numarul de utilizatori din fisier
  fp = fopen ( "./users", "r" );
  if ( fp == NULL )
    exit ( EXIT_FAILURE );

  while ( ( r = getline ( &line, &len, fp ) ) != -1 ) {
    nOfUsers++;
  }

  // users = ( struct User * ) malloc ( nOfUsers * sizeof ( struct User ) );

  // citesc utilizatorii

  fseek ( fp, 0, SEEK_SET );
  int currentUser = 0;
  while ( ( r = getline ( &line, &len, fp ) ) != -1 ) {
    strcpy ( users[ currentUser ].name, line );
    users[ currentUser++ ].loggedIn = 0;
  }

  fclose ( fp );
  if ( line )
    free ( line );

  int fd;

  while ( 1 ) {

    int pid;
    switch ( pid = fork ( ) ) {
    case -1:
      printf ( "[Server] Eroare la FORK\n" );
      exit ( 1 );
    case 0: { // proces copil

      char *comanda;
      int size;

      if ( ( read ( pipe1[ 0 ], &size, sizeof ( int ) ) ) < 0 ) {
        printf ( "[Server -copil]Eroare la citire din PIPE\n" );

        return -1;
      }

      comanda = ( char * ) malloc ( ( size + 1 ) );

      if ( ( read ( pipe1[ 0 ], comanda, size ) ) < 0 ) {
        printf ( "[Server -copil]Eroare la citire din PIPE\n" );
        free ( comanda );

        return -1;
      }

      comanda[ size ] = '\0';

      char answer[ 255 ];
      if ( startsWith ( comanda, "login" ) ) {
        int success = login ( comanda, users, nOfUsers );
        switch ( success ) {
        case -1:
          strcpy ( answer, "error" );
          break;
        case 0:
          strcpy ( answer, "denied" );
          break;
        default:
          strcpy ( answer, "success" );
          break;
        }
      } else if ( startsWith ( comanda, "get-logged-users" ) ) {
        getLoggedUsers ( );
      } else if ( startsWith ( comanda, "get-proc-info" ) ) {
        char *success = getProcInfo ( comanda );
        if ( success == NULL )
          strcpy ( answer, "wrong input" );
        else
          strcpy ( answer, success );
      } else if ( startsWith ( comanda, "logout" ) ) {
        int success = logout ( users, nOfUsers );
        if ( success == 1 )
          strcpy ( answer, "success" );
        else
          strcpy ( answer, "failed" );
      } else if ( startsWith ( comanda, "quit" ) ) {
        logout ( users, nOfUsers );
        strcpy ( answer, "quit" );
      } else {
        strcpy ( answer, "Wrong comm" );
      }

      size = strlen ( answer );
      if ( ( write ( sockp[ 1 ], &size, sizeof ( int ) ) < 0 ) ) {
        printf ( "[Server - copil]Eroare la scriere in sockp\n" );
        exit ( 0 );
      }

      if ( ( write ( sockp[ 1 ], answer, size ) < 0 ) ) {
        printf ( "[Server - copil]Eroare la scriere in sockp\n" );
        exit ( 0 );
      }
      close ( fd );
      exit ( 0 );

      break;
    }
    default: // parinte -> trimite comanda catre copil prin pipe si asteapta
             // raspuns prin FIFO
    {
      printf ( "[Server - parinte] astept mesaj de la client" );

      fd = open ( "./FIFO", O_RDONLY ); // check for errors

      if ( ( read ( fd, &size, sizeof ( int ) ) ) < 0 ) {
        printf ( "[Server]Eroare la citire din FIFO\n" );

        return -1;
      }

      comanda = ( char * ) malloc ( ( size + 1 ) );

      if ( ( read ( fd, comanda, size ) ) < 0 ) {
        printf ( "[Server]Eroare la citire din FIFO\n" );
        free ( comanda );

        return -1;
      }

      comanda[ size ] = '\0';

      if ( ( write ( pipe1[ 1 ], &size, sizeof ( int ) ) < 0 ) ) {
        printf ( "[Server - parinte]Eroare la scriere in pipe\n" );
        exit ( 0 );
      }

      if ( ( write ( pipe1[ 1 ], comanda, size ) < 0 ) ) {
        printf ( "[Server - parinte]Eroare la scriere in pipe\n" );
        exit ( 0 );
      }
      close ( pipe1[ 1 ] );

      if ( ( read ( sockp[ 0 ], &size, sizeof ( int ) ) ) < 0 ) {
        printf ( "[Server - parinte]Eroare la citire din socket\n" );
        free ( comanda );
        return -1;
      }
      raspuns = ( char * ) malloc ( ( size + 1 ) );
      if ( ( read ( sockp[ 0 ], raspuns, size ) ) < 0 ) {
        printf ( "[Server - parinte]Eroare la citire din socket\n" );
        free ( comanda );
        free ( raspuns );
        return -1;
      }

      close ( sockp[ 0 ] );

      raspuns[ size ] = '\0';
      printf ( "Raspuns login: %s\n", raspuns );
      int size = strlen ( raspuns );

      fd = open ( "./FIFO", O_WRONLY ); // check for errors

      if ( ( write ( fd, &size, sizeof ( int ) ) < 0 ) ) {
        printf ( "[Server - parinte]Eroare la scriere in FIFO\n" );
        exit ( 0 );
      }

      if ( ( write ( fd, raspuns, size ) < 0 ) ) {
        printf ( "[Server - parinte]Eroare la scriere in FIFO\n" );
        exit ( 0 );
      }

      close ( fd );
    }
    }
    free ( raspuns );
    free ( comanda );
  }
}
