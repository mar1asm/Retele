#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define READ 0
#define WRITE 1
#define sizeOfInt 4
#define ERR -1

int nr_comanda, inProcess = true, lungimeComanda, autentificat = 0, fd,
                correctName = 1, lungime, length, lungimeMesaj, socketPair[ 2 ],
                pipe_name[ 2 ];
;
char userName[ 1000 ], comanda[ 1000 ], path[ 1000 ], fisier[ 1000 ],
    option[ 1000 ], copyoption[ 1000 ], raspunsLogare[ 1000 ], mesaj[ 1000 ],
    raspuns[ 1000 ], path[ 1000 ], file[ 1000 ],
    paths[ 10000 ] = "", parola[ 1000 ], raspunsLogin[ 1000 ],
                 raspunsMyStat[ 1000 ];
pid_t pid;
struct stat stats;

void citireLogin ( );
void citireQuit ( );
void citireMyStat ( );
void citireMyFind ( );
void comandaGresita ( );
void functiiInitiale ( struct stat stats, pid_t pid );
void citireComadaDeLaParinte ( );
void trimiteComandaCatreCopil ( );
void citireRaspunsDeLaFiu ( char *p );
void loginFiu ( char *p );
void quitFiu ( char *p );
void myStatFiu ( char *p );
void myFindFiu ( char *p );
void comandaGresitaFiu ( char *p );
void loginParinte ( );
void quitParinte ( );
void myStatParinte ( );
void myFindParinte ( );
void comandaGresitaParinte ( );
void verificareComanda ( );
void procesareParinte ( );
void procesareCopil ( );
bool procesareLogin ( char *username, char *parola, char *infoToDisplay );
void procesareMyFind ( char *fisier, char *infoToDisplay );
void procesareMyStat ( char *fisier, char *infoToDisplay );
void searchFileinPath ( char *path, char *file, char *infoToDisplay );

int main ( ) {

  while ( inProcess ) {

    functiiInitiale ( stats, pid );

    pid = fork ( );

    if ( pid != 0 )
      procesareParinte ( );

    if ( pid == 0 )
      procesareCopil ( );
  }

  return 0;
}

void functiiInitiale ( struct stat stats, pid_t pid ) {
  fgets ( option, 100, stdin );
  strcpy ( copyoption, option );
  stat ( "./FIFO", &stats );
  unlink ( "./FIFO" );
  mkfifo ( "./FIFO", S_IWUSR | S_IRUSR );
  socketpair ( AF_UNIX, SOCK_STREAM, 0, socketPair );
  pipe ( pipe_name );
  verificareComanda ( );
}
void verificareComanda ( ) {
  if ( strstr ( option, "login : " ) )
    citireLogin ( );
  else if ( strstr ( option, "quit" ) )
    citireQuit ( );
  else if ( strstr ( option, "mystat " ) )
    citireMyStat ( );
  else if ( strstr ( option, "myfind" ) )
    citireMyFind ( );
  else
    comandaGresita ( );
}
void procesareParinte ( ) {
  trimiteComandaCatreCopil ( );
  char *comandaTastatura = strtok ( comanda, " " );
  citireRaspunsDeLaFiu ( comandaTastatura );

  if ( strcmp ( comandaTastatura, "login" ) == 0 )
    loginParinte ( );
  if ( strcmp ( comandaTastatura, "mystat" ) == 0 )
    myStatParinte ( );
  if ( strcmp ( comandaTastatura, "myfind" ) == 0 )
    myFindParinte ( );
  if ( strcmp ( comandaTastatura, "quit" ) == 0 )
    quitParinte ( );
  if ( strcmp ( comandaTastatura, "comandaGresita" ) == 0 )
    comandaGresitaParinte ( );
}
void procesareCopil ( ) {

  citireComadaDeLaParinte ( );
  char *comandaTastatura = strtok ( comanda, " " );
  if ( strcmp ( comandaTastatura, "login" ) == 0 )
    loginFiu ( comandaTastatura );
  if ( strcmp ( comandaTastatura, "mystat" ) == 0 )
    myStatFiu ( comandaTastatura );
  if ( strcmp ( comandaTastatura, "myfind" ) == 0 )
    myFindFiu ( comandaTastatura );
  if ( strcmp ( comandaTastatura, "quit" ) == 0 )
    quitFiu ( comandaTastatura );
  if ( strcmp ( comandaTastatura, "comandaGresita" ) == 0 )
    comandaGresitaFiu ( comandaTastatura );

  exit ( 0 );
}

// procesare citiree
void citireLogin ( ) {
  strcpy ( ( option + 6 ), ( option + 8 ) );
  strcpy ( ( copyoption + 6 ), ( copyoption + 8 ) );
  char *login = "login ";
  bool ok = true;
  for ( int i = 0; i <= 5; i++ ) {
    if ( login[ i ] != option[ i ] )
      ok = false;
  }
  if ( ok ) {
    if ( autentificat == 1 ) {

      printf ( "Sunteti deja logat. Doriti sa va delogati? (y/n)\n" );
      fgets ( raspunsLogare, 100, stdin );
      if ( strstr ( raspunsLogare, "y" ) ) {
        autentificat = 0;
        printf ( "V-ati delogat. \n" );
      } else {
        printf ( "Sunteti logat in continuare. \n" );
      }
    } else {

      char *pch;
      pch = strtok ( copyoption, " " );
      pch = strtok ( NULL, " " );
      if ( pch != NULL )
        pch = strtok ( NULL, " " );
      if ( pch != NULL ) {
        correctName = 0;
        strcpy ( comanda, "login " );
        pch[ strlen ( pch ) - 1 ] = '\0';
        strcat ( comanda, pch );

      } else {
        correctName = 1;

        pch = strtok ( option, " " );
        pch = strtok ( NULL, " " );

        strcpy ( comanda, "login " );
        pch[ strlen ( pch ) - 1 ] = '\0';
        strcat ( comanda, pch );
        printf ( "Introduceti parola. (este 123)\n" );
        fgets ( parola, 100, stdin );
        pch = strtok ( parola, " " );
        strcat ( comanda, " " );
        strcat ( comanda, parola );
      }
    }
  } else {
    strcpy ( comanda, "comandaGresita" );
  }
}
void citireQuit ( ) {
  if ( strcmp ( option, "quit\n" ) == 0 ) {
    strcpy ( comanda, "quit " );
    inProcess = false;
  } else
    strcpy ( comanda, "comandaGresita" );
}
void citireMyStat ( ) {

  char *pch;
  pch = strtok ( option, " " );
  if ( strcmp ( pch, "mystat" ) == 0 ) {
    if ( pch != NULL )
      pch = strtok ( NULL, " " );
    strcpy ( comanda, "mystat " );
    pch[ strlen ( pch ) - 1 ] = '\0';
    strcat ( comanda, pch );
  } else
    strcpy ( comanda, "comandaGresita" );
  if ( pch != NULL && ! strcmp ( comanda, "comandaGresita" ) ) { // path
    pch = strtok ( NULL, " " );
    pch[ strlen ( pch ) - 1 ] = '\0';
    strcat ( comanda, " " );
    strcat ( comanda, pch );
  }
}
void citireMyFind ( ) {
  char *pch;
  pch = strtok ( option, " " );
  if ( strcmp ( pch, "myfind" ) == 0 ) {
    if ( pch != NULL ) {
      pch = strtok ( NULL, " " );

    } else {
      strcpy ( comanda, "comandaGresita" );
    }
    if ( pch != NULL ) {

      strcpy ( comanda, "myfind " );
      pch[ strlen ( pch ) - 1 ] = '\0';
      strcat ( comanda, pch );
      if ( strcmp ( pch, "" ) == 0 )
        strcpy ( comanda, "comandaGresita" );

    } else {
      strcpy ( comanda, "comandaGresita" );
    }
  } else
    strcpy ( comanda, "comandaGresita" );
}
void comandaGresita ( ) { strcpy ( comanda, "comandaGresita" ); }

// procesari in fiu
void loginFiu ( char *p ) {
  // iau primul parametru de dupa comanda (username-ul)
  fd = open ( "./FIFO", O_WRONLY );
  p = strtok ( NULL, " " );
  char *parola;
  if ( strcmp ( p, "123\n" ) )
    parola = strtok ( NULL, " " );
  else
    ( strcpy ( parola, p ) );
  bool gasit = procesareLogin ( p, parola, raspunsLogin );
  if ( correctName == 0 ) {
    strcpy ( raspuns, "NumeGresit" );
    length = strlen ( raspuns );
    write ( fd, &length, sizeOfInt );
    write ( fd, raspuns, length );
    close ( fd );
  }
  if ( strcmp ( raspunsLogin, "totOK" ) == 0 ) {
    strcpy ( raspuns, "NumeGasit" );

    length = strlen ( raspuns );
    write ( fd, &length, sizeOfInt );
    write ( fd, raspuns, length );
    close ( fd );
  }
  if ( strcmp ( raspunsLogin, "userGresit" ) == 0 ) {
    strcpy ( raspuns, "NumeNegasit" );

    length = strlen ( raspuns );
    write ( fd, &length, sizeOfInt );
    write ( fd, raspuns, length );
    close ( fd );
  }
  if ( strcmp ( raspunsLogin, "parolaGresita" ) == 0 ) {
    strcpy ( raspuns, "ParolaGresita" );
    length = strlen ( raspuns );
    write ( fd, &length, sizeOfInt );
    write ( fd, raspuns, length );
    close ( fd );
  }
  if ( strcmp ( raspunsLogin, "lipsaNume" ) == 0 ) {
    strcpy ( raspuns, "lipsaNume" );
    length = strlen ( raspuns );
    write ( fd, &length, sizeOfInt );
    write ( fd, raspuns, length );
    close ( fd );
  }
}
void myFindFiu ( char *p ) {
  p = strtok ( NULL, " " );
  strcpy ( file, p );

  procesareMyFind ( file, paths );
  length = strlen ( paths );
  if ( length == 0 ) {
    close ( socketPair[ 0 ] );
    strcpy ( raspuns, "FisierNegasit" );
    length = strlen ( raspuns );
    write ( socketPair[ 1 ], &length, sizeOfInt );
    write ( socketPair[ 1 ], raspuns, length );
    close ( socketPair[ 1 ] );
  } else {
    close ( socketPair[ 0 ] );
    write ( socketPair[ 1 ], &length, sizeOfInt );
    write ( socketPair[ 1 ], paths, length );
    close ( socketPair[ 1 ] );
  }
}
void comandaGresitaFiu ( char *p ) {
  close ( socketPair[ READ ] );
  strcpy ( raspuns, "ComandaGresita" );
  length = strlen ( raspuns );
  write ( socketPair[ WRITE ], &length, sizeOfInt );
  write ( socketPair[ WRITE ], raspuns, length );
  close ( socketPair[ WRITE ] );
}
void quitFiu ( char *p ) {
  close ( socketPair[ READ ] );
  strcpy ( raspuns, "Iesire" );
  length = strlen ( raspuns );
  write ( socketPair[ WRITE ], &length, sizeOfInt );
  write ( socketPair[ WRITE ], raspuns, length );
  close ( socketPair[ WRITE ] );
}
void myStatFiu ( char *p ) {
  close ( socketPair[ READ ] );
  p = strtok ( NULL, " " );
  procesareMyStat ( p, raspuns );
  length = strlen ( raspuns );
  write ( socketPair[ WRITE ], &length, sizeOfInt );
  write ( socketPair[ WRITE ], raspuns, length );
  close ( socketPair[ WRITE ] );
}
// procesari in parinte
void loginParinte ( ) {
  if ( strcmp ( mesaj, "NumeGasit" ) == 0 ) {
    printf ( "Ati intrat in cont.\n" );
    autentificat = 1;
  }
  if ( strcmp ( mesaj, "NumeNegasit" ) == 0 )
    printf ( "User neidentificat. Va rugam sa va creati un cont. "
             "Accesati fisierul nume.txt.\n" );

  if ( strcmp ( mesaj, "NumeGresit" ) == 0 )
    printf ( "Introduceti un singur nume.\n" );
  if ( strcmp ( mesaj, "lipsaNume" ) == 0 )
    printf ( "Nu ati introdus niciun username.\n" );
  if ( strcmp ( mesaj, "ParolaGresita" ) == 0 )
    printf ( "Parola gresita.\n" );
}
void myFindParinte ( ) {
  if ( strcmp ( mesaj, "FisierNegasit" ) == 0 )
    printf ( "Nu exista in sistem un fisier cu acest nume.\n" );
  else
    printf ( "%s", mesaj );
}
void myStatParinte ( ) {
  if ( strlen ( mesaj ) == 0 )
    printf ( "Fisierul nu exista.\n" );
  else {
    printf ( "Tipul fisierului: " );
    printf ( "%s", mesaj );
  }
}
void quitParinte ( ) {
  if ( strcmp ( mesaj, "Iesire" ) == 0 ) {
    printf ( "Terminare proces.\n" );
  }
}
void comandaGresitaParinte ( ) {
  if ( strcmp ( mesaj, "ComandaGresita" ) == 0 ) {
    printf ( "Comanda gresita.\n" );
  }
}

// comunicare procese
void trimiteComandaCatreCopil ( ) {
  // parinte-> fiu prin pipe

  close ( pipe_name[ READ ] );
  lungimeComanda = strlen ( comanda );
  write ( pipe_name[ WRITE ], &lungimeComanda, sizeOfInt );
  write ( pipe_name[ WRITE ], comanda, lungimeComanda );
  close ( pipe_name[ WRITE ] );
}
void citireComadaDeLaParinte ( ) {

  close ( pipe_name[ WRITE ] );
  read ( pipe_name[ READ ], &lungime, sizeOfInt );
  read ( pipe_name[ READ ], comanda, lungime );
  close ( pipe_name[ READ ] );
  comanda[ lungime ] = '\0';
}
void citireRaspunsDeLaFiu ( char *p ) {
  memset ( mesaj, 0, sizeof ( mesaj ) );
  if ( strcmp ( p, "login" ) == 0 ) {
    fd = open ( "FIFO", O_RDONLY );
    read ( fd, &lungimeMesaj, sizeOfInt );
    read ( fd, mesaj, lungimeMesaj );
    close ( fd );
  } else {
    close ( socketPair[ WRITE ] );
    read ( socketPair[ READ ], &lungimeMesaj, sizeOfInt );
    read ( socketPair[ READ ], mesaj, lungimeMesaj );
    close ( socketPair[ READ ] );
  }
}

// procesari functionalitati
bool procesareLogin ( char *username, char *parola, char *infoToDisplay ) {

  strcpy ( infoToDisplay, "*\0" );
  bool user;
  char usersNames[ 1000 ];

  if ( strcmp ( parola, "123\n" ) != 0 ) {
    strcpy ( infoToDisplay, "parolaGresita" );

  } else {
    FILE *fisier = fopen ( "nume.txt", "r" );
    while ( fscanf ( fisier, "%s\n", usersNames ) != EOF )
      if ( strcmp ( usersNames, username ) == 0 )
        strcpy ( infoToDisplay, "totOK" );
    if ( strcmp ( infoToDisplay, "*" ) == 0 )
      strcpy ( infoToDisplay, "userGresit" );
    if ( strcmp ( username, "123\n" ) == 0 ) {
      strcpy ( infoToDisplay, "lipsaNume" );
    }
  }

  // printf ( "In login function %s \n", infoToDisplay );
  return true;
}
void procesareMyStat ( char *file, char *fileInfo ) {
  struct stat st;
  mode_t drepturi = st.st_mode & S_IFMT;
  if ( stat ( file, &st ) == ERR )
    return;
  if ( S_ISDIR ( st.st_mode ) )
    strcpy ( fileInfo, "Fisierul e director.\n" );
  if ( S_ISFIFO ( st.st_mode ) )
    strcpy ( fileInfo, "Fisierul e FIFO sau PIPE.\n" );
  if ( S_ISSOCK ( st.st_mode ) )
    strcpy ( fileInfo, "Fisierul e socket.\n" );
  if ( S_ISREG ( st.st_mode ) )
    strcpy ( fileInfo, "Fisierul este unul de tip 'regular file'.\n" );
  if ( S_ISLNK ( st.st_mode ) )
    strcpy ( fileInfo, "Fisierul e un link.\n" );
  if ( S_ISBLK ( st.st_mode ) )
    strcpy ( fileInfo, "Fisierul este unul de tip 'block device'.\n" );
  if ( S_ISCHR ( st.st_mode ) )
    strcpy ( fileInfo, "Fisierul este unul de tip 'character device'.\n" );

  sprintf ( fileInfo + strlen ( fileInfo ), "Dimensiunea fisierului: %ld\n",
            st.st_size );

  strcat ( fileInfo, "Data ultimei accesari: " );
  strcat ( fileInfo, ctime ( &st.st_atime ) );

  strcat ( fileInfo, "Data ultimei modificari: " );
  strcat ( fileInfo, ctime ( &st.st_mtime ) );

  if ( drepturi ) {
    if ( S_IRUSR )
      strcat ( fileInfo, "Fisierul are drepturi de citire.\n" );
    else
      strcat ( fileInfo, "Fisierul  nu are drepturi de citire.\n" );

    if ( S_IWUSR )
      strcat ( fileInfo, "Fisierul are drepturi de scriere.\n" );
    else
      strcat ( fileInfo, "Fisierul nu are drepturi de scriere.\n" );

    if ( S_IXUSR )
      strcat ( fileInfo, "Fisierul are drepturi de executie.\n" );
    else
      strcat ( fileInfo, "Fisierul nu are drepturi de executie.\n" );
  }
}
void procesareMyFind ( char *file, char *infoToDisplay ) {

  searchFileinPath ( "/home", file, infoToDisplay );
}
void searchFileinPath ( char *path, char *file, char *infoToDisplay ) {
  struct stat st;
  stat ( path, &st );
  DIR *dir;
  struct dirent *dirEnt;
  if ( S_ISDIR ( st.st_mode ) ) {
    if ( ( dir = opendir ( path ) ) != NULL ) {
      while ( ( dirEnt = readdir ( dir ) ) != NULL ) {
        if ( strcmp ( dirEnt->d_name, "." ) != 0 )
          if ( strcmp ( dirEnt->d_name, ".." ) != 0 ) {
            bool sameName = ! strcmp ( dirEnt->d_name, file );
            char new_path[ 1000 ];
            char fileInfo[ 1000 ];
            sprintf ( new_path, "%s/%s", path, dirEnt->d_name );
            procesareMyStat ( new_path, fileInfo );
            if ( sameName )
              sprintf ( infoToDisplay + strlen ( infoToDisplay ), "\n%s\n%s\n",
                        new_path, fileInfo );
            searchFileinPath ( new_path, file, infoToDisplay );
          }
      }
    } else
      return;

    closedir ( dir );
  }
}