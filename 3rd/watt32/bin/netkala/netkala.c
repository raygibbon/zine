/*******************************************************************************
 *$Id: ckala.c,v 1.26 1999/06/24 13:40:23 c81315 Exp $ 
 *
 *
 * netkala is a simple net version of the game Kalahari.
 * Two players can play over the internet.
 * 
 * Copyrights (not really existend)
 * Made by: 
 *   Game           Schwaerzler Hermann and Wilhelm Theo (ages ago)
 *   Socket Stuff   Wilhelm Theo (03/99)
 *  
 * Thanks to T. Pelizari for his help concerning the very first handshake.
 ******************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WATT32
#include <tcp.h>
#else
#include <sys/un.h>
#endif
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <curses.h>
#include <ctype.h>

#define TIMEOUT_connect  120    /* timeout to create connection in seconds */
#define TIMEOUT_move     120    /* timeout to read/write move in seconds */
#define DEBUG 0

void check_opts(int, char const **, char *, char *, char *);
char const *get_ip(char const *); 

int  get_connection(int *, char *, char *, char *, char *, char *, int *, int);
void initialize_game(int *, int pos[12][2], char *, char *); 
int  ziehen(int, int, int *, int pos[12][2]);
int  display_field(int *, char *, char *, char *, int, int); 

int  send_turn(int, int, int);
int  receive_turn(int, int);

void blink_number(int, int, int, int);
void sleep_ms(int);

void clear_stdin(void);
void hide_cursor(void);
void int2str(char*, int);
void p_error(char *);
void p_message(char *, char *);

void display_rules(void);

void end_game(int *, char *, char *);
void react2sig(int);
void exit_kala(void);

WINDOW *win_main = NULL;
int kala_sockfd, curses_flag;

/******************************************************************************/
int main(int argc, char const **argv)
{
  int error_flag;
  char l_port[6] = "8880", r_port[6] = "8880", r_ip[16];
  char own_name[20], partner_name[20] = "";
  int feld[12], pos[12][2], whoms_turn, choice, zug_result;
  struct sigaction act; /* Necessary to handle CTRL C sequence adequately */

/******************************************************************************/
#ifdef WATT32   /* initialise Watt-32 tcp/ip library */
  dbug_init();
  sock_init();
#endif

/******************************************************************************/
  /* Process command line options - get remote IP */
  check_opts( argc, argv, r_ip, l_port, r_port ); 

/******************************************************************************/
  printf("Whats your name? ");
  fgets(own_name, 20, stdin);

/******************************************************************************/
  if ( get_connection(&kala_sockfd, r_ip, r_port, l_port, own_name, 
       partner_name, &error_flag, TIMEOUT_connect) == -1 ) {
    fprintf(stderr, "Error: Couldn't connect to partner\n");
    if ( DEBUG )
      fprintf(stderr, "(received error_flag %d from subroutine 'get_connection'\n", 
        error_flag);
    exit(-1);
  }

/******************************************************************************/
  /* Signal handling (Ctrl-C will be handled by subroutine react2sig */
  act.sa_handler = react2sig;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  sigaction(SIGINT, &act, 0);

/******************************************************************************/
  /* Clean exit */
  atexit(exit_kala);

/******************************************************************************/
  whoms_turn = 1;
  initialize_game(feld, pos, partner_name, own_name);
  zug_result = 99;

  if ( error_flag == -1 ) {
    while (zug_result != 0) {
      display_field(feld, partner_name, own_name, "Waiting for partner's turn!",
        0, whoms_turn);
      choice = receive_turn(kala_sockfd, TIMEOUT_move);  
      if ( choice == 99 ) {
        p_error( "Received interrupt or nonsense from partner" );
        exit(-1);
      }
      choice = choice + 6;
      zug_result = ziehen(whoms_turn, choice, feld, pos);
      if ( !(zug_result == 0 || zug_result == 3) ) {
        p_error("Error: Invalid move from partner - Aborting !!!!");
        exit(-1);
      }
    }
  }

  while (1) {
    whoms_turn = whoms_turn * (-1);
    zug_result = 99;
    while ( zug_result != 0 )
    {
      choice = display_field(feld, partner_name, own_name, 
        "Whats your choice [1..5]?", 2, whoms_turn);
      zug_result = ziehen(whoms_turn, choice, feld, pos); 
      if (zug_result == 3 || zug_result == 0) {
        if ( send_turn(choice, kala_sockfd, TIMEOUT_move) != 0 ) {
          p_error( "Error in transmition" );
          exit(-1);
        }
      }
      if (zug_result == 4 ) {
        if ( send_turn(choice, kala_sockfd, TIMEOUT_move) != 0 ) {
          p_error( "Error in transmition" );
          exit(-1);
        }
        end_game(feld, partner_name, own_name);
      }
    } 
    whoms_turn = whoms_turn * (-1);
    zug_result = 99;
    while (zug_result != 0) {
      display_field(feld, partner_name, own_name, "Waiting for partner's turn!",
        0, whoms_turn);
      choice = receive_turn(kala_sockfd, TIMEOUT_move);
      if ( choice == 99 ) {
        p_error( "Received interrupt or nonsense from partner" );
        exit(-1);
      }
      choice = choice + 6;
      zug_result = ziehen(whoms_turn, choice, feld, pos);
      if (zug_result == 4) {
        end_game(feld, partner_name, own_name);
      }
      if ( !(zug_result == 0 || zug_result == 3) ) {
        p_error("Error: Invalid move from partner - Aborting !!!!");
        exit(-1);
      }
    }
  }
}



/*******************************************************************************
 * subroutines  
 ******************************************************************************/

/*******************************************************************************
 * check_opts
 *
 * Process command line options 
 *
 * On input:  argc, argv
 * On output: remote ip and port, local port or info|help for the program
 ******************************************************************************/
void check_opts( int argc, char const **argv, char *r_ip, char *l_port, 
                 char *r_port ) 
{
  int i, portnumber;

  if ( argc == 1 || strcmp( argv[1], "-help") == 0 || 
       strcmp( argv[1], "-usage") == 0 ) {
    printf( "\n" );
    printf( "Usage: netkala HOST_IP|HOST_NAME [OPTIONS]\n\n" );
    printf( "Play netkala with another user through the internet\n\n" );
    printf( "Type 'netkala -rules' for description of the game\n\n" );
    printf( "HOST_IP (HOST_NAME):  ip number (machine name) of partner\n" );
    printf( "                      (= remote) machine\n" );
    printf( "OPTIONS:              -l_port LOCAL_PORT  (default is 8880)\n" );
    printf( "                      -r_port REMOTE_PORT (default is 8880)\n" );
    printf( "\nNo-(C) by Wilhelm, Schwaerzler. V1.0 (summer 99)\n\n" );
    exit(0);
  }
  
  if ( argc == 2 && strcmp( argv[1], "-rules") == 0 ) {
    display_rules();
    exit(0);
  }

  if ( strcmp( get_ip( argv[1] ), "NULL" ) == 0 ) {
    fprintf( stderr, "Error in 'get_ip': Couldn't get IP of remote host\n" );
    exit(-1);
  }
  else
    strcpy( r_ip, get_ip( argv[1] ) );

  for ( i = 2; i < argc; i++ ) {
    if ( strcmp( argv[i], "-l_port") == 0 ) {
      portnumber =  atoi( argv[i+1] );
      if (  portnumber > 65535 ||  portnumber < 0 ) {
        fprintf( stderr, "Option ´%s %s´: Port number out of range (0,65535)\n", 
                 argv[i], argv[i+1] );
        exit(0);
      }
      i++;
      sprintf( l_port, "%d", portnumber);
    }  
    if ( strcmp( argv[i], "-r_port") == 0 ) {
      portnumber =  atoi( argv[i+1] );
      if ( atoi( argv[i+1] ) > 65535 || atoi( argv[i+1] ) < 0 ) {
        fprintf( stderr, "Option ´%s %s´: Port number out of range (0,65535)\n", 
                argv[i], argv[i+1] );
        exit(0);
      }
      i++;
      sprintf( r_port, "%d", portnumber);
    }
  }
}


/*******************************************************************************
 * get_ip
 *
 * Process command line options 
 * 
 * On input:  String containing either ip address or machine name of a host 
 * On output: String with ip address upon success
 *            "NULL" upon error
 ******************************************************************************/
char const *get_ip( char const *ip_string ) 
{
  struct hostent *r_hostinfo;
  char **r_ip_list;
#if 0
  int i, numeric = 1;

  for ( i = 0; i < strlen( ip_string ); i++ ) {
    if ( !( isdigit( ip_string[i] ) || ip_string[i] == '.' ) ) {
      numeric = 0;
      break;
    }
  }
  if ( numeric == 1 )
    return( ip_string );
#else
  if (inet_addr(ip_string) != INADDR_NONE)
     return (ip_string);
#endif

  r_hostinfo = gethostbyname( ip_string );
  if ( !r_hostinfo ) {
    if ( DEBUG )
      fprintf( stderr, "Error in 'get_opt': Can't get IP address of host %s\n",
        ip_string );
    return( "NULL" );
  }

  if ( r_hostinfo -> h_addrtype != AF_INET ) {
    if ( DEBUG )
      fprintf( stderr, "Error in 'get_opt': Host %s is not an IP host",
        ip_string );
    return( "NULL" );
  }

  r_ip_list = r_hostinfo -> h_addr_list;
  return( inet_ntoa( *(struct in_addr *) *r_ip_list ) );
}

/*******************************************************************************
 * get_connection
 * 
 * Connects to partner socket and exchanges names
 *
 * On input: kalasock_fd, remote ip, remote port, local port, own_name, 
 *           partner_name, result3 (flag), timeout
 * On output: 0 - success, -1 - error
 *            In case of success: *kala_sockfd has the appropriate value for the
 *                                  connection 
 *                                *result3 = 0  if connected as client  
 *                                *result3 = -1 if first got connection from
 *                                             client
 *            In case of error:   *result3 returns the error codes:
 *                                1 - timeout
 *                                2 - couldn't transmit own_name
 *                                3 - couldn't receive partner_name 
 ******************************************************************************/
int get_connection(int *kala_sockfd, char *r_ip, char *r_port, char *l_port, 
                   char *own_name, char *partner_name, int *result3, 
                   int timeout)
{
  int server_sockfd;
  int server_len, kala_len;
  struct sockaddr_in server_address, kala_address;

  int result1, result2, start_time;

  int max_sockfd;
  struct timeval tv;
  fd_set readfds, writefds;

  /****************************************************************************/
  /* Create an unnamed socket for the server and name it */
  server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons( atoi(l_port) );
  server_len = sizeof(server_address);
  bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
    
  /****************************************************************************/
  /* Create a socket for a client */
  *kala_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  kala_address.sin_family = AF_INET; 
  kala_address.sin_addr.s_addr = inet_addr( r_ip );
  kala_address.sin_port = htons( atoi(r_port) );
  kala_len = sizeof(kala_address);

  /****************************************************************************/
  /* Very first Handshake - exchange of names */
  start_time   = time(0);
  result1      = -1;
  result2      = -1;
  *result3     = -1;

  listen(server_sockfd, 5);    /* Create a connection and wait for clients: */

  if (server_sockfd <= *kala_sockfd) 
    max_sockfd = *kala_sockfd;
  else
    max_sockfd = server_sockfd;

  while (time(0) - start_time <= timeout)
  {
    FD_ZERO( &readfds );
    FD_ZERO( &writefds );
 
    FD_SET( *kala_sockfd, &writefds );
    FD_SET( server_sockfd, &readfds );

    tv.tv_sec  = 5;
    tv.tv_usec = 0;

    select( max_sockfd + 1, &readfds, &writefds, (fd_set *)0, &tv );

    if ( FD_ISSET(server_sockfd, &readfds) ) {
      close(*kala_sockfd);
      *kala_sockfd = accept(server_sockfd, (struct sockaddr *)&kala_address, 
        &kala_len);
      result1 = read(*kala_sockfd, partner_name, 20);
      result2 = write(*kala_sockfd, own_name, 20);
      return(0);
    }
    if ( FD_ISSET(*kala_sockfd, &writefds) )
      *result3 = connect(*kala_sockfd, (struct sockaddr *)&kala_address, 
        kala_len);
      if (*result3 != -1) {
        result2 = write(*kala_sockfd, own_name, 20);
        result1 = read(*kala_sockfd, partner_name, 20);
        return(0);
      }
  }

  /* Error: Timeout during handshake */
  if (time(0) - start_time >= timeout)  {
    *result3 = 1;
    close( *kala_sockfd );
    return(-1);
  }

  /* Error transmitting name to partner */
  if (result2 == -1) {
    *result3 = 2;
    close( *kala_sockfd );
    return(-1);
  }

  /* Error: Couldn't read name from partner */
  if (partner_name == "" || result1 <= 0 ) {
    *result3 = 3;
    close( *kala_sockfd );
    return(-1);
  }

  return(-1);
}


/*******************************************************************************
 * ziehen
 * 
 * Calculates new field according to the move requested
 * 
 * On input: whoms_turn, choice, feld, pos.
 * On output: 0 - move was ok; 1 - choice from wrong field; 2 - choice
 *            from empty field; 3 - same player must move again;
 *            4 - end of the game reached.
 ******************************************************************************/
int ziehen(int whoms_turn, int choice, int *feld, int pos[12][2])
{
  int i, dummy, dummy2;

  if ( !( ( whoms_turn ==  1 && choice >= 6 && choice <= 10 ) || 
       ( whoms_turn == -1 && choice >= 0 && choice <= 4 ) ) )
    return(1);

  if ( feld[choice] == 0 ) return(2);

  dummy = choice + feld[choice];
  blink_number( feld[choice], 0, pos[choice][0], pos[choice][1] );
  feld[choice] = 0;
  if ( dummy <= 11 ) {
    for ( i = choice+1; i <= dummy; i++ ) {
      blink_number( feld[i], feld[i] + 1, pos[i][0], pos[i][1] );
      feld[i]++;
    }
    /**************** Take points from opposite field *****************/
    if ( feld[dummy] == 1 && dummy != 5 && dummy != 11 && 
      feld[10-dummy] != 0 ) {
      blink_number( feld[10-dummy], 0, pos[10-dummy][0],
        pos[10-dummy][1] );
      if ( choice > 5 ) {
        blink_number( feld[11], feld[11] + feld[10-dummy], pos[11][0],
          pos[11][1] );
        feld[11] += feld[10-dummy];
      }
      else {
        blink_number( feld[5], feld[5] + feld[10-dummy], pos[5][0],
          pos[5][1] );
        feld[5] += feld[10-dummy];
      }
      feld[10-dummy] = 0;
    }
  }
  else {
    for (i = choice+1; i <= 11; i++) {
      blink_number( feld[i], feld[i] + 1, pos[i][0], pos[i][1] );
      feld[i]++;
    }
    for( i = 0; i <= dummy-12; i++ ) {
      blink_number( feld[i], feld[i] + 1, pos[i][0], pos[i][1] );
      feld[i]++;
    }
    /**************** Take points from opposite field *****************/
    dummy2 = 10-(dummy-12);
    if ( feld[dummy-12] == 1 && dummy-12 != 5 && dummy-12 != 11 && 
      feld[dummy2] != 0 ) {
      blink_number( feld[dummy2], 0, pos[dummy2][0], pos[dummy2][1] );
      if ( choice > 5 ) {
        blink_number( feld[11], feld[11] + feld[dummy2], pos[11][0],
          pos[11][1] );
        feld[11] += feld[dummy2];
      }
      else {
        blink_number( feld[5], feld[5] + feld[dummy2], pos[5][0],
          pos[5][1] );
        feld[5] += feld[dummy2];
      }
      feld[dummy2] = 0;
    }
  }

  /* Felder 0..4 leer -> Spiel Ende */
  if ( feld[0] + feld[1] + feld[2] + feld[3] + feld[4]  == 0 ) {
    for (i = 6; i <= 10; i++) {
      if ( feld[i] != 0 ) {
        blink_number( feld[i], 0, pos[i][0], pos[i][1] );
        blink_number( feld[11], feld[11] + feld[i], pos[11][0],
          pos[11][1] );
        feld[11] += feld[i];
        feld[i] = 0;
      }
    }
  return(4);
  }

  /* Fields 6..10 empty -> end of the game */
  if ( feld[6] + feld[7] + feld[8] + feld[9] + feld[10] == 0 ) {
    for (i = 0; i <= 4; i++) {
      if ( feld[i] != 0 ) {
        blink_number( feld[i], 0, pos[i][0], pos[i][1] );
        blink_number( feld[5], feld[5] + feld[i], pos[5][0],
          pos[5][1] );
        feld[5] += feld[i];
        feld[i] = 0;
      }
    }
  return(4);
  }

  /* Move again, because last shell (??) landed in one of the big side holes */
  if (dummy == 11 || dummy == 5 || dummy == 17 || dummy == 23) return(3);
  else  return(0);
}


/*******************************************************************************
 * display_field
 * 
 * Interface between program and player
 * 
 * On input: field, players names, conversation line, conversation flag,
 *           whoms_turn.
 *           conversation flag: 0 - no user input; 1 - input yes/no;
 *                              2 - input of move (0-4,6-10).
 * On output: move (0-4,6-10); 99 for no, 100 for yes.
 ******************************************************************************/
int display_field(int *feld, char *partner_name, char *own_name, 
  char *conversation, int convers_flag, int whoms_turn) 
{
  int c = 199, x, y;
  char dummy[1];

  x = 24;
  y = 8;
  move(y+3, x);
  printw("      /   %2d %2d %2d %2d %2d    \\", feld[10], feld[9], feld[8], 
    feld[7], feld[6]);
  move(y+4, x);
  printw("      |%2d  ------------- %2d |", feld[11], feld[5]);
  move(y+5, x);
  printw("      \\   %2d %2d %2d %2d %2d    /", feld[0], feld[1], feld[2],
    feld[3], feld[4]);

  x -= 10;
  y += 14;
  move(y, 0);
  clrtoeol();

  refresh();

  if (convers_flag == 0) {
    move(y, x);
    attron(A_BLINK);
    printw("%s", conversation);
    attroff(A_BLINK);
    hide_cursor();
    refresh();
  }

  if (convers_flag == 1) {
    while ( strchr("yYnN", c) == NULL ) {
      move(y, x);
      attron(A_BLINK);
      printw("%s", conversation);
      attroff(A_BLINK);
      move(y, x + strlen(conversation) + 2);
      clear_stdin();
      c = getch();
      refresh();
    }
    if ( (c == 'n') || (c == 'N') ) return(99);
    else return(100);
  }

  if (convers_flag == 2) {
    move(y, x);
    attron(A_BLINK);
    printw("%s", conversation);
    attroff(A_BLINK);
    while ( !( c >= 1 && c <= 5 && feld[c-1] != 0 ) ) {
      move(y, x + strlen(conversation) + 2);
      clear_stdin();
      dummy[0] = getch();
      c = dummy[0] - 48;
      refresh();
    }
    return(--c);
  }

  return(0);
}


/*******************************************************************************
 * initialize_game
 * 
 * Initializes the field
 ******************************************************************************/
void initialize_game(int *feld, int pos[12][2], char *partner_name, 
  char *own_name) 
{
  const char title[] = "*  KALAHARI by Wilhelm-Schwaerzler  *";
  int i, x, y;

  for (i = 0; i <= 4; i++) {
    feld[i]   = 3; 
    feld[i+6] = 3; 
  }
  feld[5]   = 0;
  feld[11]  = 0;

  /* Initialize screen using curses */
  win_main = initscr();
  if ( win_main == NULL ) 
    exit(-1);
  curses_flag = 1;
  
  x = 22; 
  y = 2;
  move(y, x);
  attron(A_REVERSE);
  printw("%s", title);
  attroff(A_REVERSE);

  x = 24; 
  y = 8;
  move(y, x);
  attron(A_BOLD);
  printw("           %s", partner_name );
  attroff(A_BOLD);

  move(y+2, x);
  printw("       /-------------------\\");
  move(y+3, x);
  printw("      /   %2d %2d %2d %2d %2d    \\", 3, 3, 3, 3, 3);
  pos[10][0] = 34;
  pos[9][0]  = 37;
  pos[8][0]  = 40;
  pos[7][0]  = 43;
  pos[6][0]  = 46;
  for( i = 6; i <= 10; pos[i++][1] = y+3); 
  move(y+4, x);
  printw("      |%2d  ------------- %2d |", 0, 0);
  pos[5][0]  = 49;
  pos[5][1]  = y + 4;
  pos[11][0] = 31;
  pos[11][1] = y + 4;
  move(y+5, x);
  printw("      \\   %2d %2d %2d %2d %2d    /", 3, 3, 3, 3, 3);
  pos[0][0]  = 34;
  pos[1][0]  = 37;
  pos[2][0]  = 40;
  pos[3][0]  = 43;
  pos[4][0]  = 46;
  for ( i = 0; i <= 4; pos[i++][1] = y+5 );
  move(y+6, x);
  printw("       \\-------------------/");
  attron(A_REVERSE);
  for (i = 1; i <= 5; i++) {
    move(y+8, x+8+3*i);
    printw("%d", i);
  }
  attroff(A_REVERSE);

  move(y+10, x);
  attron(A_BOLD);
  printw("           %s", own_name );
  attroff(A_BOLD);

  cbreak();
  noecho();

  refresh();
}


/*******************************************************************************
 * end_game
 * 
 * Shows end result and ends the game
 ******************************************************************************/
void end_game(int *feld, char *partner_name, char *own_name)
{
  int x, y, res1, res2;
  char winner[20];

  display_field(feld, partner_name, own_name, "", 0, 1);
  refresh();
  sleep(4);

  erase();

  if ( feld[5] > feld[11] ) {
    strcpy(winner, own_name);
    res1 = feld[5];
    res2 = feld[11];
  }
  if ( feld[5] < feld[11] ) {
    strcpy(winner, partner_name);
    res1 = feld[11];
    res2 = feld[5];
  }
  if ( feld[5] == feld[11] ) {
    strcpy(winner, "Nobody");
    res1 = feld[11];
    res2 = feld[5];
  }
     
  x = 32; 
  y = 2;
  move(y, x);

  attron(A_REVERSE);
  printw("END OF THE GAME");
  attroff(A_REVERSE);

  move(y+5, x);
  printw("The winner is:");

  move(y+7, x);
  attron(A_BLINK);
  printw("%s", winner);
  attroff(A_BLINK);

  move(y+9, x);
  printw("Score  %d:%d", res1,  res2 );

  refresh();
  sleep(7);

  exit(0);
}


/*******************************************************************************
 * send_turn
 * 
 * sends choice via socket to partner
 *
 * On input: socket fd to write to, timeout
 ******************************************************************************/
int send_turn(int choice, int sockfd, int timeout)
{
  char choice_string[3];

  struct timeval tv;
  fd_set writefds;
  tv.tv_sec  = timeout;
  tv.tv_usec = 0;

  choice_string[0] = choice;
  choice_string[1] = '\0';

  FD_ZERO( &writefds );
  FD_SET( sockfd, &writefds );

  select( sockfd + 1, (fd_set *)0, &writefds, (fd_set *)0, &tv );

  if ( FD_ISSET(sockfd, &writefds) ) {
    if ( write(sockfd, &choice_string, 3) == -1 ) {
      p_message("'send_turn': Error transmitting choice to partner", ""); 
      return(-1);
    }
  else return(0);
  }
  p_message("'send_turn': Timout during transmition", "");
  return(-1);
}
  

/*******************************************************************************
 * receive_turn
 * 
 * receive choice from partner (via socket)
 *
 * On input: socket fd to receive from, timeout
 * On output:  choice: within (0..4) if ok
 *                     99 if partner interrupted program or error occured
 ******************************************************************************/
int receive_turn(int sockfd, int timeout)
{
  char choice[3], dummy[3];

  struct timeval tv;
  fd_set readfds;

  tv.tv_sec  = timeout;
  tv.tv_usec = 0;

  FD_ZERO( &readfds );
  FD_SET( sockfd, &readfds );

  select( sockfd + 1, &readfds, (fd_set *)0, (fd_set *)0, &tv );

  if ( FD_ISSET(sockfd, &readfds) ) {
    if ( read(sockfd, choice, 3) == -1 ) {
      p_message("'receive turn': Error reading choice from partner", ""); 
      return(99);
    }
    if ( (int) choice[0] == 99 ) {
      p_message("'receive turn': Received Interrupt from partner", "");
      return(99);
    }
    else {
      int2str( dummy, (int) choice[0] ); 
      p_message( "receive_turn - received choice", dummy );
      return( (int) choice[0] );
    }
  }
  p_message("'receive turn': Timout occured", "");
  return(99);
}

/******************************************************************************
 * blink_number
 *
 * makes the number at the given position flash
 *****************************************************************************/
void blink_number(int old, int new, int xpos, int ypos)
{
  char old_str[20], new_str[20];
  char old_c[20], new_c[20];
  int i;

  int2str( old_str, old );
  int2str( new_str, new );

  if ( strlen( old_str ) == 1 )
    sprintf(old_c, " %s", old_str);
  else
    sprintf(old_c, "%s", old_str);

  if ( strlen( new_str ) == 1 )
    sprintf(new_c, " %s", new_str);
  else
    sprintf(new_c, "%s", new_str);

  move( ypos, xpos );
  printw( "  " );
  refresh();

  move( ypos, xpos );
  attron( A_REVERSE );
  printw( "%s", old_c );
  hide_cursor();
  refresh();

  for ( i = 1; i <= 2; i++ ) {
    attroff( A_REVERSE );
    move( ypos, xpos );
    printw( "%s", old_c );
    hide_cursor();
    refresh();
    sleep_ms( 150000 );

    move( ypos, xpos );
    attron( A_REVERSE );
    printw( "%s", old_c );
    hide_cursor();
    refresh();
    sleep_ms( 150000 );
  }

  attroff( A_REVERSE );
  move( ypos, xpos );
  printw( "%s", new_c );
  hide_cursor();
  refresh();
}


/******************************************************************************
 * sleep_ms
 *
 * sleeps for time_us in micro seconds (1 000 000 usec = 1 second)
 *****************************************************************************/
void sleep_ms(int time_us)
{
  struct timeval tv;
  tv.tv_sec  = 0;
  tv.tv_usec = time_us;

  select( 0, NULL, NULL, NULL, &tv );
}


/*******************************************************************************
 * clear_stdin
 * 
 * loescht eventuelle eingaben vom stdin (verwendet curses)
 ******************************************************************************/
void clear_stdin(void)
{
  int c;

  nodelay(win_main, TRUE);
  do {
    c = getch();
  } while (c != ERR);  /* das gelesene zeichen ist ERR wenn nix da ist */
  nodelay(win_main, FALSE);

  return;
}


/*******************************************************************************
 * int2str
 * 
 * Convert integer to char *[3] 
 ******************************************************************************/
void int2str( char *string, int zahl)
{
  int dummy;
 
  if ( zahl > 99 || zahl < 0 ) {
    printf("int2str - Error: Input integer not in [0,99] !!  Aborting !!" );
    sleep(3);
    exit(-1);
  }
  if ( zahl >= 10 ) {
    dummy     = zahl / 10;
    string[0] =  dummy + 48;
    string[1] = zahl - 10 * dummy + 48;
    string[2] = '\0';
  }
  else {
    string[0] = zahl + 48;
    string[1] = '\0';
  }
}


/******************************************************************************
 * hide_cursor
 * 
 * hides the cursor (na ja ?)
 *****************************************************************************/
void hide_cursor(void) {
  move( 2, 21 );
  refresh();
}


/******************************************************************************
 * p_error
 * 
 * displays error messages 
 *****************************************************************************/
void p_error(char *error)
{
  move(21, 10);
  clrtoeol();
  refresh();
  move(21, 10);
  printw("%s", error);
  refresh();
  sleep(3);
}


/******************************************************************************
 * p_message
 * 
 * displays messages 
 *****************************************************************************/
void p_message(char *message, char *arg)
{
  if ( DEBUG ) {
    move(21, 10);
    clrtoeol();
    refresh();
    move(21, 10);
    printw("%s: ##%s##", message, arg);
    refresh();
    sleep(3);
  } 
}


/*******************************************************************************
 * react2sig
 * 
 * reaction to a CTR-C signal
 ******************************************************************************/
void react2sig(int sig) 
{
  p_error("catched SIGINT - aborting");
  send_turn(99, kala_sockfd, 10);
  exit(-1);
}


/*******************************************************************************
 * exit_kala
 * 
 * Clean exit of kala        
 ******************************************************************************/
void exit_kala(void)
{
  if ( curses_flag == 1 ) {
    p_error( "Exiting netkala !" );
    endwin();
  }
  else
    fprintf( stderr, "Exiting netkala!!\n");
  close( kala_sockfd ); 
}  

/*******************************************************************************
 * display_rules
 *
 * Info for the user - displays the rules of the game 
 ******************************************************************************/
void display_rules(void)
{
  char rules_text[]="
       Description:
       ------------

       Netkala  is a  game for  two players.  To my knowledge it 
       comes  from  Afrika,  where it was  played  in  sand  with
       marbles  or Kaori shells as pawns (pieces). It is a simple
       and short game.  One rather needs to be clever than  lucky
       to  win.  It has a similar character as 'draughts' (Ameri-
       can 'checker', German 'Dame') or the German kind of  'mor-
       ris'  (called  'Muehle').  

       How to play netkala:
       --------------------

       Each  player  sits  in front of a row of five holes (indi-
       cated by the five small 'o' signs next to the line ´Play-
       er 1/2´ on  the  schematic sketch  bellow), and  has a big 
       whole to his right (indicated by capital 'O'):

                   Player 1
                 o  o  o  o  o
              O                 O
                 o  o  o  o  o
                   Player 2

       At the beginning of the game all the small ('o') holes are
       filled  with three pieces each.  The aim of the game is to
       collect as many pieces in his big hole.
       An arbitrary player starts (usually  the  one,  that  dies
       earlier).   He can choose one of his small holes and move.
       He does this by picking all the  pieces  of  the  hole  he
       choose,  and  distributing  them  over  all  the following
       holes.

       To understand what I mean by following holes you  have  to
       think  of  all the holes forming an accidently flattened
       circle.  Moreover we define  the  direction  of  the  game
       opposite  to  the  direction of conventional clocks (swiss
       quality clocks).  By distributing it is  meant,  that  the
       player  leaves  exactly one piece in each of the following
       holes. No matter whether its his or his  opponents  whole.
       And no matter whether its a small or a big whole.

       Usually  the players play one after each other and nothing
       embarrassing will happen. To avoid, that  netkala  becomes
       too boring, there are three extra rules:


       Rule 1: If  the  last piece in a move goes into one of the
               big  side  holes (capital  'O'),  the  same player 
               plays again.

       Rule 2: If  the  last  piece  in a  move goes into a small 
               hole,  that is  empty,  the  player  can  take all 
               pieces from the small hole opposite to it, and put 
               them  into  his  big  hole (no matter,  whether the 
               pieces  come  from  one of  his, or his  opponents 
               small holes).

       Rule 3: At the  time  one  of  the players hasn't a single
               piece in one of his small  holes  left, the  other
               player can collect all the pieces that are left in
               his small holes and put them into  his  big  hole.
               The  number of  pieces in  the  big holes are com-
               pared.  The one who was able to collect more wins.  
               The game is finished.\n";
  printf(rules_text);
}
