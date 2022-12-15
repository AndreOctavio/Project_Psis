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
    int num_bots;
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

    /* Send connect message to server */
    int n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    if (n_bytes!= sizeof(message_t)){
        perror("write");
        exit(-1);
    }

    printf("SENT COONTE\n");

    while(1){

        sleep(3);

        printf("sending movement\n");

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
    }

    close(socket_fd);

    exit(0);

}