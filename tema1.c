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

#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"
#define NORMAL "\x1B[0m"

// base functions
int login ( char *username, char *password );
int myStat ( char *filename, char *path, char *fileInfo );
int myFind ( char *filename, char *path, int depth, char *filesInfo );
void quit ( pid_t parrentPid );
void info ( char *command, char *answer );

// aux functions
void getPermissions ( mode_t permissions, char *perm );
void getFilesInfo ( char *fnct, char *dirPath, char *filename, char *fileInfo );
void getDate ( time_t time, char *date );
void getFileType ( int type, char *fileType );
void printFilesInfo ( char *command, char *answer );

int main ( ) {

  int fd, isRunning = 1, loggedIn = 0;
  char *command;
  pid_t pid, ppid = getpid ( );
  int length;
  while ( isRunning ) {
    size_t l = 0;
    printf ( "\nWrite a command: \ninfo *command_name\nlogin: username "
             "password (username: user, password: pass)\nmystat filename "
             "*path\nmyfind filename *path "
             "*-depth\nquit\n\n" );
    length = getline ( &command, &l, stdin );
    if ( length != 1 )
      command[ --length ] = 0;

    int sockp[ 2 ];
    socketpair ( AF_UNIX, SOCK_STREAM, 0, sockp );

    int pipe1[ 2 ];
    pipe ( pipe1 );

    struct stat stats;
    if ( stat ( "./myFifo", &stats ) >= 0 ) {
      if ( unlink ( "./myFifo" ) < 0 ) {
        perror ( "unlink failed" );
        return -1;
      }
    }
    if ( mkfifo ( "./myFifo", 0666 ) == -1 ) {
      printf ( RED "Eroare la FIFO\n" NORMAL );
      exit ( 0 );
    }

    pid = fork ( );

    if ( pid == 0 ) {
      setpgid ( 0, 0 );
      int length;
      char command[ 100 ];
      close ( pipe1[ 1 ] );

      read ( pipe1[ 0 ], &length, 4 );
      read ( pipe1[ 0 ], command, length );
      command[ length ] = 0;

      char *p;
      p = strtok ( command, " " );
      if ( ! strcmp ( p, "info" ) ) {
        char comm[ 20 ], answer[ 500 ];
        strcpy ( comm, "all" );
        p = strtok ( NULL, " " );
        if ( p ) {
          strcpy ( comm, p );
        }
        info ( comm, answer );
        close ( sockp[ 0 ] );
        length = strlen ( answer );
        write ( sockp[ 1 ], &length, 4 );
        write ( sockp[ 1 ], answer, length );
      }
      if ( ! strcmp ( p, "quit" ) ) { // quit
        quit ( ppid );
      } else if ( ! strcmp ( p, "login:" ) ) { // login
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
        if ( login ( username, password ) )
          strcpy ( answer, "login_success" );
        else
          strcpy ( answer, "login_fail" );
        int answerLength = strlen ( answer );
        fd = open ( "./myFifo", O_WRONLY );
        write ( fd, &answerLength, 4 );
        write ( fd, answer, answerLength );
        close ( fd );
      } else if ( ! strcmp ( p, "mystat" ) ||
                  ! strcmp ( p, "myfind" ) ) { // mystat or myfind
        char answer[ 500 ], filename[ 100 ], path[ 100 ], depth[ 3 ],
            command[ 20 ];
        close ( sockp[ 0 ] );
        if ( ! loggedIn ) {
          strcpy ( answer, "You need to log in first" );
          length = strlen ( answer );
          write ( sockp[ 1 ], &length, 4 );
          write ( sockp[ 1 ], answer, length );
        } else {
          strcpy ( command, p );
          p = strtok ( NULL, " " );

          if ( p ) {
            strcpy ( filename, p );
            p = strtok ( NULL, " " );
          }
          while ( p ) {
            int pathOrDepth = 1;
            if ( p[ 0 ] == '-' ) {
              for ( int i = 1; i < strlen ( p ); i++ ) {
                if ( ! ( p[ i ] >= '0' && p[ i ] <= '9' ) )
                  pathOrDepth = 0;
              }
            }
            if ( pathOrDepth )
              strcpy ( depth, p );
            else
              strcpy ( path, p );
            p = strtok ( NULL, " " );
          }

          answer[ 0 ] = 0;
          int ret = 0;
          if ( ! strcmp ( command, "mystat" ) )
            ret = myStat ( filename, path, answer );
          else {
            int d = -1;
            if ( strlen ( depth ) ) {
              strcpy ( depth, depth + 1 );
              d = atoi ( depth );
            }
            ret = myFind ( filename, path, d, answer );
          }

          if ( ret == 1 )
            strcpy ( answer, "Invalid path" );

          length = strlen ( answer );
          write ( sockp[ 1 ], &length, 4 );
          if ( length ) {
            write ( sockp[ 1 ], answer, length );
          }
          close ( sockp[ 1 ] );
        }
      } else {
        char answer[ 20 ];
        close ( sockp[ 0 ] );
        strcpy ( answer, "Unknown command" );
        length = strlen ( answer );
        write ( sockp[ 1 ], &length, 4 );
        write ( sockp[ 1 ], answer, length );
      }
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

      if ( ! strcmp ( answer, "login_success" ) ) {
        printf ( GREEN "%s\n", "You have successfully logged in" NORMAL );
        loggedIn = 1;
      } else {
        loggedIn = 0;
        printf ( RED "%s\n", "Login failed" NORMAL );
      }
      close ( fd2 );
    } else if ( ! strcmp ( p, "mystat" ) ) {
      char answer[ 100 ];
      close ( sockp[ 1 ] );
      read ( sockp[ 0 ], &length, 4 );
      if ( length == 0 )
        printf ( RED "No file with this name found. Try searching for smth "
                     "else\n" NORMAL );
      else {
        read ( sockp[ 0 ], answer, length );
        answer[ length ] = 0;
        if ( ! strcmp ( answer, "Invalid path" ) ||
             ! strcmp ( answer, "You need to log in first" ) )
          printf ( RED "%s\n" NORMAL, answer );
        else {
          printFilesInfo ( p, answer );
        }
      }
      close ( sockp[ 0 ] );
    } else if ( ! strcmp ( p, "myfind" ) ) {
      char answer[ 500 ];
      close ( sockp[ 1 ] );
      read ( sockp[ 0 ], &length, 4 );
      if ( length == 0 )
        printf ( RED "No file with this name found. Try searching for smth "
                     "else\n" NORMAL );
      else {
        read ( sockp[ 0 ], answer, length );
        answer[ length ] = 0;
        if ( ! strcmp ( answer, "Invalid path" ) ||
             ! strcmp ( answer, "You need to log in first" ) )
          printf ( RED "%s\n" NORMAL, answer );
        else {
          printFilesInfo ( p, answer );
        }
      }
      close ( sockp[ 0 ] );
    } else {
      char answer[ 20 ];
      close ( sockp[ 1 ] );
      read ( sockp[ 0 ], &length, 4 );
      read ( sockp[ 0 ], answer, length );
      answer[ length ] = 0;
      if ( ! strcmp ( p, "info" ) )
        printf ( "%s\n", answer );
      else
        printf ( RED "%s\n" NORMAL, answer );
      close ( sockp[ 0 ] );
    }
  }
}

void quit ( pid_t parrentPid ) {
  printf ( "%d\n%d\n", parrentPid, getpid ( ) );
  kill ( -parrentPid, SIGTERM );
  sleep ( 2 );
  kill ( -parrentPid, SIGKILL );
}

int login ( char *username, char *password ) {
  FILE *users, *pass;
  size_t l1 = 0, l2 = 0;
  int lengthU, lengthP;
  users = fopen ( "users.txt", "r" );
  pass = fopen ( "passwords.txt", "r" );
  char *usernames, *passwords;

  lengthU = getline ( &usernames, &l1, users );
  lengthP = getline ( &passwords, &l2, pass );

  while ( lengthU != -1 ) {
    usernames[ lengthU - 1 ] = 0;
    passwords[ lengthP - 1 ] = 0;
    if ( ! strcmp ( username, usernames ) &&
         ! strcmp ( password, passwords ) ) {
      return 1;
    }
    lengthU = getline ( &usernames, &l1, users );
    lengthP = getline ( &passwords, &l2, pass );
  }
  return 0;
}

int myStat ( char *filename, char *path, char *fileInfo ) {

  int r_left = 0, r_right = 0;
  if ( filename[ 0 ] == '*' && filename[ 1 ] == '.' ) {
    r_left = strlen ( filename ) - 1;
  }

  if ( filename[ strlen ( filename ) - 2 ] == '.' &&
       filename[ strlen ( filename ) - 1 ] == '*' ) {
    r_right = 1;
  }

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
      if ( ! strcmp ( dir->d_name, filename ) ||
           ( r_right &&
             ! strncmp ( dir->d_name, filename, strlen ( filename ) - 1 ) ) ||
           ( r_left &&
             ( ! strcmp ( dir->d_name + strlen ( dir->d_name ) - r_left,
                          filename + 1 ) ) ) )
        getFilesInfo ( "stat", path, dir->d_name, fileInfo );
    }
  }
  closedir ( d );
}

int myFind ( char *filename, char *path, int depth, char *filesInfo ) {
  int r_left = 0, r_right = 0;
  if ( ! strlen ( path ) ) {
    path[ 0 ] = '.';
    path[ 1 ] = 0;
  }
  DIR *d = opendir ( path );
  if ( d == NULL ) {
    return 1;
  }
  if ( filename[ 0 ] == '*' && filename[ 1 ] == '.' ) {
    r_left = strlen ( filename ) - 1;
  }

  if ( filename[ strlen ( filename ) - 2 ] == '.' &&
       filename[ strlen ( filename ) - 1 ] == '*' ) {
    r_right = 1;
  }
  struct dirent *dir;
  while ( ( dir = readdir ( d ) ) != NULL ) {
    if ( dir->d_type != DT_DIR ) {
      if ( dir->d_type != DT_DIR ) {
        if ( ! strcmp ( dir->d_name, filename ) ||
             ( r_right &&
               ! strncmp ( dir->d_name, filename, strlen ( filename ) - 1 ) ) ||
             ( r_left &&
               ( ! strcmp ( dir->d_name + strlen ( dir->d_name ) - r_left,
                            filename + 1 ) ) ) ) {
          char fileInfo[ 200 ];
          fileInfo[ 0 ] = 0;
          getFilesInfo ( "find", path, dir->d_name, fileInfo );
          strcat ( filesInfo, fileInfo );
          getFilesInfo ( "stat", path, dir->d_name, fileInfo );
        }
      }
    } else if ( dir->d_type == DT_DIR && strcmp ( dir->d_name, "." ) != 0 &&
                strcmp ( dir->d_name, ".." ) != 0 ) { // sa nu ma intorc
      char new_path[ 255 ];
      strcpy ( new_path, path );
      strcat ( new_path, "/" );
      strcat ( new_path, dir->d_name );
      if ( depth )
        myFind ( filename, new_path, depth - 1, filesInfo );
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
  realpath ( filePath, absolute_path );
  strcat ( fileInfo, absolute_path );
  strcat ( fileInfo, " " );

  mode_t permissions = st.st_mode;
  char perm[ 10 ];
  getPermissions ( permissions, perm );
  strcat ( fileInfo, perm );

  strcat ( fileInfo, " " );

  char date[ 200 ];
  getDate ( st.st_ctime, date );
  strcat ( fileInfo, date );

  if ( ! strcmp ( fnct, "find" ) ) {
    strcat ( fileInfo, "\n" );
    return;
  }
  strcat ( fileInfo, " " );
  getDate ( st.st_atime, date );
  strcat ( fileInfo, date );
  getDate ( st.st_mtime, date );
  strcat ( fileInfo, date );

  char othrInfo[ 100 ];
  getFileType ( st.st_mode, othrInfo );
  strcat ( fileInfo, othrInfo );
  strcat ( fileInfo, " " );

  sprintf ( othrInfo, "%ld", st.st_dev );
  strcat ( fileInfo, othrInfo );

  strcat ( fileInfo, " " );

  sprintf ( othrInfo, "%ld", st.st_nlink );
  strcat ( fileInfo, othrInfo );
  strcat ( fileInfo, " " );

  sprintf ( othrInfo, "%d", st.st_uid );
  strcat ( fileInfo, othrInfo );
  strcat ( fileInfo, " " );

  sprintf ( othrInfo, "%d", st.st_gid );
  strcat ( fileInfo, othrInfo );
  strcat ( fileInfo, " " );

  strcat ( fileInfo, "\n" );
}

void printFilesInfo ( char *command, char *answer ) {

  char *ch;
  int filesFound = 0;
  ch = strchr ( answer, '\n' );
  while ( ch ) {
    filesFound++;
    ch = strchr ( ch + 1, '\n' );
  }
  printf ( BLUE "%d file(s) found\n" NORMAL, filesFound );
  char *info;
  info = strtok ( answer, " " );

  for ( int i = 0; i < filesFound; i++ ) {
    printf ( "\nFile no. %d\n", i + 1 );

    printf ( "Absolute path: " GREEN "%s" NORMAL "\n", info );
    info = strtok ( NULL, " " );
    printf ( "Rights: " GREEN "%s" NORMAL "\n", info );
    info = strtok ( NULL, " " );
    printf ( "Last change: " GREEN "%s" NORMAL "\n", info );
    if ( ! strcmp ( command, "mystat" ) ) {
      info = strtok ( NULL, " " );
      printf ( "Last access: " GREEN "%s" NORMAL "\n", info );
      info = strtok ( NULL, " " );
      printf ( "Last modification: " GREEN "%s" NORMAL "\n", info );
      info = strtok ( NULL, " " );
      printf ( "File type: " GREEN "%s" NORMAL "\n", info );
      info = strtok ( NULL, " " );
      printf ( "Device ID: " GREEN "%s" NORMAL "\n", info );
      info = strtok ( NULL, " " );
      printf ( "No. of hard links: " GREEN "%s" NORMAL "\n", info );
      info = strtok ( NULL, " " );
      printf ( "UID: " GREEN "%s" NORMAL "\n", info );
      info = strtok ( NULL, " " );
      printf ( "GID: " GREEN "%s" NORMAL "\n", info );
    }

    info = strtok ( NULL, " \n" );
  }
}

void getDate ( time_t time, char *date ) {
  struct tm *tm;
  char convertedTime[ 200 ];
  tm = localtime ( &time );
  strftime ( convertedTime, sizeof ( convertedTime ), "%d.%m.%Y-%H:%M:%S", tm );
  strcpy ( date, convertedTime );
  strcat ( date, " " );
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

  switch ( type & S_IFMT ) // type of file
  {

  case S_IFBLK:
    strcpy ( fileType, "block" );
    break;
  case S_IFCHR:
    strcpy ( fileType, "char" );
    break;
  case S_IFDIR:
    strcpy ( fileType, "directory" );
    break;
  case S_IFIFO:
    strcpy ( fileType, "fifo" );
    break;
  case S_IFLNK:
    strcpy ( fileType, "link" );
    break;
  case S_IFREG:
    strcpy ( fileType, "regular" );
    break;
  case S_IFSOCK:
    strcpy ( fileType, "socket" );
    break;
  default:
    strcpy ( fileType, "unknown" );
    break;
  }
}

void info ( char *command, char *answer ) {
  if ( ! strcmp ( command, "all" ) ) {
    strcpy ( answer,
             "\nPentru a afla informatii despre o comanda, "
             "foloseste comanda info, urmata de numele comenzii. \nDe exemplu "
             "info login:\n" );
  }

  if ( ! strcmp ( command, "login:" ) || ! strcmp ( command, "login" ) ) {
    strcpy ( answer, "\nFoloseste comanda \"login:\" urmata de username si "
                     "parola, separate prin spatii. "
                     "\nDe exemplu  \"login: maria 123\"\n" );
  }
  if ( ! strcmp ( command, "mystat" ) ) {
    strcpy (
        answer,
        "\nFoloseste comanda \"mystat\" urmata de numele fisierului despre "
        "care doresti sa afli informatii si path-ul directorului in care este "
        "fisierul, separate prin spatii. "
        "ex \"mystat: filename.ext maria/retele/tema1\". \nIn cazul in "
        "care path-ul nu a fost specificat, se vor afisa informatii "
        "despre fisierele aflate in directorul curent. \nSe poate folosi de "
        "asemenea si '*' pentru a cauta toate fisierele cu o anumita extensie, "
        "sau toata fisierele cu acelasi nume, indiferent de extensie. \nDe "
        "exemplu "
        "mystat filename.*\n" );
  }

  if ( ! strcmp ( command, "myfind" ) ) {
    strcpy (
        answer,
        "\nFoloseste comanda \"myfind\" urmata de numele fisierului despre "
        "care doresti sa afli informatii, path-ul directorului din care sa "
        "se inceapa cautarea si adancimea maxima la care sa se caute "
        "(precedata de simbolul \'-\'), separate "
        "prin spatii. ex \"myfind: filename.ext maria/retele/tema1 -3\". \nIn "
        "cazul in care path-ul nu a fost specificat, se vor cauta fisierele "
        "pornind din directorul curent. In cazul in care adancimea la care sa "
        "se caute nu e specificata, cautarea se va face in toate "
        "subdirectoarele \nSe poate folosi de "
        "asemenea si '*' pentru a cauta toate fisierele cu o anumita extensie, "
        "sau toata fisierele cu acelasi nume, indiferent de extensie. \nDe "
        "exemplu mystat filename.*\n" );
  }

  if ( ! strcmp ( command, "quit" ) ) {
    strcpy ( answer, "\nFoloseste comanda \"quit\" pentru a incheia." );
  }
  if ( ! strcmp ( command, "info" ) ) {
    strcpy (
        answer,
        "\nFoloseste comanda \"info\" pentru a afla mai multe informatii..." );
  }
}