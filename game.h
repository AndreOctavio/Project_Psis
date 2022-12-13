#include <stdlib.h>
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

typedef enum msg_type_t {connect, ball_information, ball_movement, field_status, health_0, disconnect, lobby_full} msg_type_t;
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

/*Structure to store player's information*/
typedef struct player_info_t 
{   
    int ch;
    int pos_x, pos_y;
    int HP;

    struct sockaddr_un client_addr;
    socklen_t client_addr_size;
} player_info_t;

typedef struct message_t
{   
    msg_type_t msg_type; 
    player_info_t player;
    WINDOW game_state;
    direction_t direction;
} message_t;