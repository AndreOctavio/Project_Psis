#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <pthread.h>

#include<netinet/in.h>
#include<arpa/inet.h>

#include<ctype.h>

#define WINDOW_SIZE 20
#define SOCK_PORT 5000
#define N_THREADS 12

typedef enum msg_type_t {connection, ball_information, ball_movement, field_status, health_0, disconnect, lobby_full, prize_spawn, continue_game} msg_type_t;
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;
typedef enum game_state_t {in_game, countdown, game_over} game_state_t;

/*Structure to store player's information*/
typedef struct player_info_t 
{   
    char ch;
    int pos_x, pos_y;
    int hp;

} player_info_t;

typedef struct message_t
{   
    msg_type_t msg_type;
    player_info_t player[10];
    player_info_t bots[10];
    player_info_t prizes[10];
    int player_num;
    int direction;
} message_t;

typedef struct thread_args_t
{
    WINDOW * my_win;
    WINDOW * message_win;
    struct sockaddr_in server_addr;
    int socket_fd;
    char character;
    message_t msg;
    player_info_t * player;
    game_state_t * game_state;
    pthread_mutex_t * lock;

} thread_args_t;

typedef struct server_args_t
{
    WINDOW * my_win;
    WINDOW * message_win;

    int con_socket [10]; // Array to store the players addresses

    /* player_data - stores all the info regarding the current players;
    *  bot_data - stores all the info regarding the bots in the game; 
    *  prize_data - stores all the info regarding the prizes in the game; */
    player_info_t player_data[10]; 
    player_info_t bot_data[10]; 
    player_info_t prize_data[10];

    int n_bots;
    int n_players;
    int n_prizes;

    int tmp_self;

    pthread_mutex_t lock;
} server_args_t;

void show_all_health(WINDOW * message_win, player_info_t player_data[10]);