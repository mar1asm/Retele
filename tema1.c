#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int login ( char *username );
int myStat ( char *filename );
int myFind ( char *filename, char *path, char *filesInfo );

int main ( ) {

  int fd, isRunning = 1;
  char command[ 100 ];
  pid_t pid;
  while ( isRunning ) {
    printf ( "Write a command: \n" );
    fgets ( command, 100, stdin );

    int sockp[ 2 ];
    socketpair ( AF_UNIX, SOCK_STREAM, 0, sockp );

    int pipe1[ 2 ]; // mod
    pipe ( pipe1 );

    struct stat stats;
    if ( stat ( "./fifo", &stats ) >= 0 ) { // MyFifo exista
      if ( unlink ( "./fifo" ) < 0 )        // il sterg
      {
        perror ( "unlink failed" );
        return -1;
      }
    }
    if ( mkfifo ( "./fifo", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH ) == -1 )
      printf ( "Eroare la FIFO\n" );

    pid = fork ( );

    if ( pid == 0 ) {
      int length;
      char command[ 100 ];
      char answer[ 500 ];
      close ( pipe1[ 1 ] );
      read ( pipe1[ 0 ], &length, 4 );
      read ( pipe1[ 0 ], command, length );
      if ( ! strcmp ( command, "quit" ) ) {
        // ceva....
        printf ( "exiting quit" );
        exit ( 0 );
      }
      char *p;
      p = strtok ( command, " " );

      if ( ! strcmp ( p, "login:" ) ) {
        p = strtok ( NULL, " " );
        close ( sockp[ 0 ] );
        login ( p ) ? strcpy ( answer, "login_success" )
                    : strcpy ( answer, "login_fail" );
        int answerLength = strlen ( answer );
        fd = open ( "./fifo", O_WRONLY );
        write ( fd, &answerLength, 4 );
        write ( fd, answer, answerLength );
        close ( fd );
        exit ( 0 );
      }
      if ( ! strcmp ( p, "mystat" ) ) {
        p = strtok ( NULL, " " );
        printf ( "exiting myfind" );
        exit ( 0 );
      }
      if ( ! strcmp ( p, "myfind" ) ) {
        p = strtok ( NULL, " " );
        p[ strlen ( p ) - 1 ] = '\0';
        strcpy ( answer, "\0" );
        myFind ( p, "/home/maria/retele", answer );
        printf ( "ok" );
        printf ( "%s", answer );
        exit ( 0 );
      }
      printf ( "unknown command" );
      printf ( "exiting unknown command" );
      exit ( 0 );
    }
    int length = strlen ( command );
    close ( pipe1[ 0 ] );
    write ( pipe1[ 1 ], &length, 4 );
    write ( pipe1[ 1 ], command, strlen ( command ) );
    close ( pipe1[ 1 ] );

    char *answer = NULL;

    char *p = strtok ( command, " " );
    if ( ! strcmp ( p, "login:" ) ) {
      fd = open ( "./fifo", O_RDONLY );
      read ( fd, &length, 4 );
      read ( fd, answer, length );
      ! strcmp ( answer, "login_success" )
          ? printf ( "%s\n", "You have successfully logged in" )
          : printf ( "%s\n", "Login failed" );
    };
  }
}

int login ( char *username ) {
  FILE *file;
  file = fopen ( "users.txt", "r" );
  char *usernames;
  username[ strlen ( username ) - 1 ] = '\0';
  while ( fscanf ( file, "%s\n", usernames ) != EOF ) {
    if ( ! strcmp ( username, usernames ) ) {
      return 1;
    }
  }
  return 0;
}

int myStat ( char *filename ) { printf ( "hello from myFind" ); }

int myFind ( char *filename, char *path, char *filesInfo ) {

  DIR *d = opendir ( path );
  if ( d == NULL )
    return 1;
  struct dirent *dir;
  while ( ( dir = readdir ( d ) ) != NULL ) {
    if ( dir->d_type != DT_DIR ) {
      if ( ! strcmp ( dir->d_name, filename ) ) {
        char filePath[ 100 ];
        strcpy ( filePath, path );
        strcat ( filePath, "/" );
        strcat ( filePath, filename );
        struct stat st;
        stat ( filePath, &st );

        struct tm *tm;
        char convertedTime[ 200 ];
        tm = localtime ( &st.st_ctime );
        strftime ( convertedTime, sizeof ( convertedTime ), "%d.%m.%Y-%H:%M:%S",
                   tm );
        strcat ( filesInfo, path );
        strcat ( filesInfo, " " );
        strcat ( filesInfo, convertedTime );

        // tm=localtime(&st.st_ctim)

        strcat ( filesInfo, ( char * ) st.st_size );
      }
    } else if ( dir->d_type == DT_DIR && strcmp ( dir->d_name, "." ) != 0 &&
                strcmp ( dir->d_name, ".." ) != 0 ) {
      char new_path[ 255 ];
      strcat ( new_path, path );
      strcat ( new_path, "/" );
      strcat ( new_path, dir->d_name );
      myFind ( filename, new_path, filesInfo );
    }
  }
  printf ( "nu e ok" );
  closedir ( d );
}