#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
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
int myStat ( char *filename, char *path, char *fileInfo );
int myFind ( char *filename, char *path, char *filesInfo );

// aux functions
void getPermissions ( mode_t permissions, char *perm );
void getFilesInfo ( char *fnct, char *dirPath, char *filename, char *fileInfo );

void getDate ( time_t time, char *date );
void getFileType ( int type, char *fileType );

int main ( ) {

  int fd, isRunning = 1;
  char *command;
  pid_t pid;
  int length;
  while ( isRunning ) {
    size_t l = 0;
    printf ( "Write a command: \n" );
    length = getline ( &command, &l, stdin );
    if ( length != 1 )
      command[ --length ] = 0;

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
    if ( mkfifo ( "./myFifo", 0666 ) == -1 ) {
      printf ( "Eroare la FIFO\n" );
      exit ( 0 );
    }

    pid = fork ( );

    if ( pid == 0 ) {
      int length;
      char command[ 100 ];
      close ( pipe1[ 1 ] );

      read ( pipe1[ 0 ], &length, 4 );
      read ( pipe1[ 0 ], command, length );
      command[ length ] = 0;
      if ( ! strcmp ( command, "quit" ) ) {
        // ceva....
        printf ( "exiting quit" );
        exit ( 0 );
      }
      char *p;
      p = strtok ( command, " " );

      if ( ! strcmp ( p, "login:" ) ) {
        char answer[ 500 ], username[ 20 ], password[ 10 ];
        p = strtok ( NULL, " " );
        if ( p ) {
          strcpy ( username, p );
          p = strtok ( NULL, " " );
        }
        if ( p ) {
          strcpy ( password, p );
        }
        close ( sockp[ 0 ] );
        login ( username ) ? strcpy ( answer, "login_success" )
                           : strcpy ( answer, "login_fail" );
        int answerLength = strlen ( answer );
        fd = open ( "./myFifo", O_WRONLY );
        write ( fd, &answerLength, 4 );
        write ( fd, answer, answerLength );
        close ( fd );
        exit ( 0 );
      }
      if ( ! strcmp ( p, "mystat" ) || ! strcmp ( p, "myfind" ) ) {
        char answer[ 500 ], filename[ 100 ], path[ 100 ], flags[ 3 ],
            command[ 20 ];
        close ( sockp[ 0 ] );
        strcpy ( command, p );
        p = strtok ( NULL, " " );

        if ( p ) {
          strcpy ( filename, p );
          p = strtok ( NULL, " " );
        }
        if ( p ) {
          strcpy ( path, p );
          p = strtok ( NULL, " " );
        }
        if ( p ) {
          strcpy ( flags, p );
        }

        answer[ 0 ] = 0;
        if ( ! strcmp ( command, "mystat" ) )
          myStat ( filename, path, answer );
        else
          myFind ( filename, path, answer );

        length = strlen ( answer );
        write ( sockp[ 1 ], &length, 4 );
        if ( length ) {
          write ( sockp[ 1 ], answer, length );
        }
        close ( sockp[ 1 ] );

        exit ( 0 );
      }
      printf ( "unknown command\n" );
      exit ( 0 );
    }

    close ( pipe1[ 0 ] );
    write ( pipe1[ 1 ], &length, 4 );
    write ( pipe1[ 1 ], command, length );
    close ( pipe1[ 1 ] );

    char *p = strtok ( command, " " );
    if ( ! strcmp ( p, "login:" ) ) {
      char answer[ 100 ];
      int fd2;
      fd2 = open ( "./myFifo", O_RDONLY );
      read ( fd2, &length, 4 );
      read ( fd2, answer, length );
      answer[ length ] = 0;

      ! strcmp ( answer, "login_success" )
          ? printf ( "%s\n", "You have successfully logged in" )
          : printf ( "%s\n", "Login failed" );
    }

    if ( ! strcmp ( p, "mystat" ) ) {
      char answer[ 100 ];
      close ( sockp[ 1 ] );
      read ( sockp[ 0 ], &length, 4 );
      if ( length == 0 )
        printf (
            "No file with this name found. Try searching for smth else\n" );
      else {
        read ( sockp[ 0 ], answer, length );
        answer[ length ] = 0;
        printf ( "%s\n", answer );
      }

      close ( sockp[ 0 ] );
    }

    if ( ! strcmp ( p, "myfind" ) ) {
      char answer[ 500 ];
      close ( sockp[ 1 ] );
      read ( sockp[ 0 ], &length, 4 );
      if ( length == 0 )
        printf (
            "No file with this name found. Try searching for smth else\n" );
      else {
        read ( sockp[ 0 ], answer, length );
        answer[ length ] = 0;
        printf ( "%s\n", answer );
      }

      close ( sockp[ 0 ] );
    }
  }
}

int login ( char *username ) {
  FILE *file;
  size_t l = 0;
  int length;
  file = fopen ( "users.txt", "r" );
  char *usernames;
  length = getline ( &usernames, &l, file );

  while ( length != -1 ) {
    usernames[ length - 1 ] = 0;
    if ( ! strcmp ( username, usernames ) ) {
      return 1;
    }
    length = getline ( &usernames, &l, file );
  }
  return 0;
}

int myStat ( char *filename, char *path, char *fileInfo ) {
  if ( ! strlen ( path ) ) {
    path[ 0 ] = '.';
    path[ 1 ] = 0;
  }

  DIR *d = opendir ( path );

  if ( d == NULL )
    return 1;

  struct dirent *dir;
  while ( ( dir = readdir ( d ) ) != NULL ) {
    if ( dir->d_type != DT_DIR && ! strcmp ( dir->d_name, filename ) ) {
      getFilesInfo ( "stat", path, dir->d_name, fileInfo );
    }
  }
  closedir ( d );
}

int myFind ( char *filename, char *path, char *filesInfo ) {
  if ( ! strlen ( path ) ) {
    path[ 0 ] = '.';
    path[ 1 ] = 0;
  }
  DIR *d = opendir ( path );
  if ( d == NULL )
    return 1;
  struct dirent *dir;
  while ( ( dir = readdir ( d ) ) != NULL ) {
    if ( dir->d_type != DT_DIR ) {
      if ( ! strcmp ( dir->d_name, filename ) ) {
        char fileInfo[ 200 ];
        fileInfo[ 0 ] = 0;
        getFilesInfo ( "find", path, filename, fileInfo );
        strcat ( filesInfo, fileInfo );
      }
    } else if ( dir->d_type == DT_DIR && strcmp ( dir->d_name, "." ) != 0 &&
                strcmp ( dir->d_name, ".." ) != 0 ) { // sa nu ma intorc
      char new_path[ 255 ];
      strcpy ( new_path, path );
      strcat ( new_path, "/" );
      strcat ( new_path, dir->d_name );
      myFind ( filename, new_path, filesInfo );
    }
  }
  closedir ( d );
}

void getFilesInfo ( char *fnct, char *dirPath, char *filename,
                    char *fileInfo ) {

  char filePath[ 100 ]; // path for stat
  strcpy ( filePath, dirPath );
  strcat ( filePath, "/" );
  strcat ( filePath, filename );

  struct stat st;
  stat ( filePath, &st ); // path
  char absolute_path[ PATH_MAX ];
  realpath ( dirPath, absolute_path );
  strcat ( fileInfo, absolute_path );
  strcat ( fileInfo, " " );

  char date[ 200 ];
  getDate ( st.st_ctime, date );
  strcat ( fileInfo, date );
  mode_t permissions = st.st_mode;
  strcat ( fileInfo, " " );

  char perm[ 10 ];
  getPermissions ( permissions, perm );
  strcat ( fileInfo, perm );
  strcat ( fileInfo, "\n" );

  if ( ! strcmp ( fnct, "find" ) )
    return;
}

void getDate ( time_t time, char *date ) {
  struct tm *tm;
  char convertedTime[ 200 ];
  tm = localtime ( &time );
  strftime ( convertedTime, sizeof ( convertedTime ), "%d.%m.%Y-%H:%M:%S", tm );
  strcpy ( date, convertedTime );
}

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

void getFileType ( int type, char *fileType ) {
  if ( type == DT_REG ) {
    strcpy ( fileType, "regular" );
    return;
  }
  if ( type == DT_DIR ) {
    strcpy ( fileType, "directory" ); // just in case..
    return;
  }
  if ( type == DT_FIFO ) {
    strcpy ( fileType, "fifo" );
    return;
  }
  if ( type == DT_SOCK ) {
    strcpy ( fileType, "socket" );
    return;
  }
  if ( type == DT_CHR ) {
    strcpy ( fileType, "char" );
    return;
  }
  if ( type == DT_BLK ) {
    strcpy ( fileType, "block" );
    return;
  }
  if ( type == DT_LNK ) {
    strcpy ( fileType, "link" );
    return;
  }
  if ( type == DT_UNKNOWN ) {
    strcpy ( fileType, "unknown" );
    return;
  }
}