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

// base functions
int login ( char *username );
int myStat ( char *filename );
int myFind ( char *filename, char *path, char *filesInfo );

// aux functions
void getPermissions ( mode_t permissions, char *perm );

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
    if ( stat ( "./myFifo", &stats ) >= 0 ) { // MyFifo exista
      if ( unlink ( "./myFifo" ) < 0 )        // il sterg
      {
        perror ( "unlink failed" );
        return -1;
      }
    }
    if ( mkfifo ( "./myFifo", 0666 ) == -1 )
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
        fd = open ( "./myFifo", O_WRONLY );
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
        close ( sockp[ 0 ] );
        p = strtok ( NULL, " " );
        p[ strlen ( p ) - 2 ] = '\0';
        strcpy ( answer, "" );
        myFind ( p, "/home/maria/retele", answer );
        length = strlen ( answer );
        answer[ length - 1 ] = '\0';
        write ( sockp[ 1 ], &length, 4 );

        if ( length ) {
          write ( sockp[ 1 ], answer, length );
        }
        close ( sockp[ 1 ] );
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

    char *p = strtok ( command, " " );
    if ( ! strcmp ( p, "login:" ) ) {
      char answer[ 100 ];
      int fd2;
      fd2 = open ( "./myFifo", O_RDONLY );
      read ( fd2, &length, 4 );
      read ( fd2, answer, length );

      ! strcmp ( answer, "login_success" )
          ? printf ( "%s\n", "You have successfully logged in" )
          : printf ( "%s\n", "Login failed" );
    };

    if ( ! strcmp ( p, "myfind" ) ) {
      char answer[ 100 ];
      close ( sockp[ 1 ] );
      read ( sockp[ 0 ], &length, 4 );
      if ( length == 0 )
        printf (
            "No file with this name found. Try searching for smth else\n" );
      else {
        read ( sockp[ 0 ], answer, length );
        printf ( "%s\n", answer );
      }

      close ( sockp[ 0 ] );
    }
  }
}

int login ( char *username ) {
  FILE *file;
  file = fopen ( "users.txt", "r" );
  char usernames[ 20 ];
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
        strcat ( filesInfo, path );
        strcat ( filesInfo, " " );

        struct tm *tm;
        char convertedTime[ 200 ];

        tm = localtime ( &st.st_ctime );
        strftime ( convertedTime, sizeof ( convertedTime ), "%d.%m.%Y-%H:%M:%S",
                   tm );
        strcat ( filesInfo, convertedTime );
        mode_t permissions = st.st_mode;
        strcat ( filesInfo, " " );
        char perm[ 10 ];
        getPermissions ( permissions, perm );
        strcat ( filesInfo, perm );
        strcat ( filesInfo, "\n" );
      }
    } else if ( dir->d_type == DT_DIR && strcmp ( dir->d_name, "." ) != 0 &&
                strcmp ( dir->d_name, ".." ) != 0 ) { // sa nu ma intorc
      char new_path[ 255 ];
      strcpy ( new_path, path ); // lol
      strcat ( new_path, "/" );
      strcat ( new_path, dir->d_name );
      myFind ( filename, new_path, filesInfo );
    }
  }
  closedir ( d );
}

// aux functions

void getPermissions ( mode_t permissions, char *perm ) {
  perm[ 0 ] = ( permissions & S_IRUSR ) ? 'r' : '-';
  perm[ 1 ] = ( permissions & S_IWUSR ) ? 'w' : '-';
  perm[ 2 ] = ( permissions & S_IXUSR ) ? 'x' : '-';
  perm[ 3 ] = ( permissions & S_IRGRP ) ? 'r' : '-';
  perm[ 4 ] = ( permissions & S_IWGRP ) ? 'w' : '-';
  perm[ 5 ] = ( permissions & S_IXGRP ) ? 'x' : '-';
  perm[ 6 ] = ( permissions & S_IROTH ) ? 'r' : '-';
  perm[ 7 ] = ( permissions & S_IWOTH ) ? 'w' : '-';
  perm[ 8 ] = ( permissions & S_IXOTH ) ? 'x' : '-';
  perm[ 9 ] = '\0';
}