#define WINDOW_SIZE 20
#define SOCKET_NAME "/tmp/sock_game"

typedef enum msg_type_t {connect, ball_information, ball_movement, field_status, health_0, disconnect} msg_type_t;
typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

/*Structure to store player's information*/
typedef struct player_info_t 
{   
    int ch;
    int pos_x, pos_y;
    int HP;
} player_info_t;

typedef struct remote_player_t
{   
    msg_type_t msg_type; 
    char ch; 
    direction_t direction;
} remote_player_t;