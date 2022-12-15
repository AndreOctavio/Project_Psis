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

#define WINDOW_SIZE 20
#define SOCKET_NAME "/tmp/sock_game"

typedef enum msg_type_t {connection, ball_information, ball_movement, field_status, health_0, disconnect, lobby_full, prize_spawn} msg_type_t;
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

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