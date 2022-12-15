#include "game.h"



int generate_direction(){
    int direction = rand() % 4;
    switch (direction){
        case 0:
            direction = KEY_UP;
            break;
        case 1:
            direction = KEY_DOWN;
            break;
        case 2:
            direction = KEY_LEFT;
            break;
        case 3:
            direction = KEY_RIGHT;
            break;
    }
    return direction;
}

int main(int argc, char * argv[]){

    int socket_fd, bot_id;
    int num_bots, elapsed_time_bots, elapsed_time_prizes;
    char socket_name[64];
    char character = '*';

    struct sockaddr_un local_client_addr;
	struct sockaddr_un server_addr;

    
    num_bots = atoi(argv[1]);
    strcpy(socket_name, argv[2]);
    printf("Socket name: %s", socket_name);

    if (num_bots == 0) {
        // Error
        perror("Invalid number of bots");
        exit(-1);
    } else if (num_bots > 10) {
        printf("The maximum number of bots is 10\nExiting...\n");
        exit(-1);
    }

    /* UI */
    printf("   ***    Welcome to the game!    *** \n");
    printf("   ***       Bots Activated       *** \n");
    printf("   ***           Enjoy!           *** \n");

    /* Create client socket */
    local_client_addr.sun_family = AF_UNIX;
    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socket_fd == -1){
        perror("socket");
        exit(-1);
    }

    sprintf(local_client_addr.sun_path, "%s_%d", socket_name, getpid());

	unlink(local_client_addr.sun_path);

	int err = bind(socket_fd, (struct sockaddr *) &local_client_addr, sizeof(local_client_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

    /* Server infos */
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, socket_name);

    /* Set connect message */
    message_t msg;

    /* Set bots */
    for(int i = 0; i < num_bots; i++){
        msg.bots[i].ch = '*';
        msg.player[i].ch = -2;
    }

    msg.msg_type = connection;
    msg.player_num = num_bots;  // Here player_num will store the number of bots in the message

    printf("sending CONETC\n");

    /* Set prizes */
    for(int i = 0; i < 5; i++){
        srand(clock());

        /* Generate number between 1 and 5 and store the ASCII value */
        msg.prizes[i].hp = (rand() % 5) + 1;
        msg.prizes[i].ch = msg.prizes[i].hp + 48;
    }

    /* Send connect message to server */
    int n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    if (n_bytes!= sizeof(message_t)){
        perror("write");
        exit(-1);
    }

    printf("SENT COONTE\n");

    elapsed_time_bots = 0;
    elapsed_time_prizes = 0;

    while(1){

        sleep(1);

        elapsed_time_bots++;
        elapsed_time_prizes++;


        if(elapsed_time_bots == 3){
            printf("sending bot movement\n");
            /* Generate new bot´s movements */
            for(int bot_id = 0; bot_id < num_bots; bot_id++){

                /* Set movement message */
                msg.msg_type = ball_movement;
                msg.direction = generate_direction();
                msg.player_num = bot_id;

                /* Send ball_movement to server */
                n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
                if (n_bytes!= sizeof(message_t)){
                    perror("write");
                    exit(-1);
                }
            }
            elapsed_time_bots = 0;
        }

        if(elapsed_time_prizes == 5){
            printf("sending prize_spawn\n");
            /* Generate new prize */
            srand(clock());

            /* Generate number between 1 and 5 and store the ASCII value */
            msg.prizes[0].hp = (rand() % 5) + 1;
            msg.prizes[0].ch = msg.prizes[0].hp + 48;
            
            /* Set movement message */
            msg.msg_type = prize_spawn;

            /* Send prize_spawn to server */
            n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
            if (n_bytes!= sizeof(message_t)){
                perror("write");
                exit(-1);
            }

            elapsed_time_prizes = 0;
        }
    }

    close(socket_fd);

    exit(0);

}