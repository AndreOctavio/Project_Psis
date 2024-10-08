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
#define MAX_PLAYERS (WINDOW_SIZE - 2) * (WINDOW_SIZE - 2)

//#define N_THREADS 12

typedef enum msg_type_t {connection, ball_information, ball_movement, field_status, health_0, reconnect, lobby_full} msg_type_t;
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;
typedef enum game_state_t {in_game, countdown, game_over} game_state_t;

/* Structure to store player's information*/
typedef struct player_info_t 
{   
    char ch;
    int pos_x, pos_y;
    int hp;

} player_info_t;

/* main message structure */
typedef struct message_t
{   
    msg_type_t msg_type;
    player_info_t player[MAX_PLAYERS];
    player_info_t bots[10];
    player_info_t prizes[10];
    int player_num;
    int direction;
} message_t;

/* struct to store changes in the field to send as field_status */
typedef struct msg_field_update
{
    msg_type_t msg_type;
    player_info_t old_status;
    player_info_t new_status;
    int arr_position;

} msg_field_update;

/* client arguments to pass to threads */
typedef struct thread_args_t
{
    WINDOW * my_win;
    WINDOW * message_win;
    int socket_fd;
    int player_id;
    player_info_t player_data[MAX_PLAYERS];
    player_info_t * player;
    game_state_t * game_state;
    pthread_mutex_t * lock;

} thread_args_t;

typedef struct countdown_thread_t
{   
    WINDOW * my_win;

    int self;
    int n_bots;

    player_info_t * players;
    int * all_sockets;

    int * n_players;
    int * free_space;

    pthread_t server_thread;

    pthread_mutex_t * lock_player;
    pthread_mutex_t * lock_free;
    pthread_mutex_t * lock_window;


} countdown_thread_t;

typedef struct server_args_t
{
    WINDOW * my_win;
    WINDOW * message_win;

    int con_socket [MAX_PLAYERS]; // Array to store the players addresses
    int free_space [WINDOW_SIZE - 2][WINDOW_SIZE - 2]; // Array that contains the positions that are free in the board

    /* player_data - stores all the info regarding the current players;
    *  bot_data - stores all the info regarding the bots in the game; 
    *  prize_data - stores all the info regarding the prizes in the game; */
    player_info_t player_data[MAX_PLAYERS]; 
    player_info_t bot_data[10]; 
    player_info_t prize_data[10];

    int n_bots;
    int n_players;
    int n_prizes;

    int tmp_self;

    pthread_t thread_id [MAX_PLAYERS];
    pthread_t thread_countdown_id [MAX_PLAYERS];

    pthread_mutex_t lock_player;
    pthread_mutex_t lock_bot;
    pthread_mutex_t lock_prize;
    pthread_mutex_t lock_free;
    pthread_mutex_t lock_window;




} server_args_t;
