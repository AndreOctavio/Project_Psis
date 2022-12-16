#include "../game.h"

int main(int argc, char * argv[]){

    int socket_fd;
    char socket_name[64];

    struct sockaddr_un local_client_addr;
	struct sockaddr_un server_addr;

    
    strcpy(socket_name, argv[1]);
    printf("Socket name: %s", socket_name);

    /* UI */
    printf("   ***    Welcome to the game!    *** \n");
    printf("   ***      Prizes Activated      *** \n");
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

    msg.msg_type = connection;

    printf("sending CONETC\n");

    /* Set prizes */
    for(int i = 0; i < 5; i++){
        srand(clock());

        /* Generate number between 1 and 5 and store the ASCII value */
        msg.prizes[i].hp = (rand() % 5) + 1;
        msg.prizes[i].ch = msg.prizes[i].hp + 48;
    }

    /* Set bots and player to deactivated */
    for(int i = 0; i < 10; i++){
        msg.bots[i].ch = -1;
        msg.player[i].ch = -1;
    }

    /* Send connect message to server */
    int n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    if (n_bytes!= sizeof(message_t)){
        perror("write");
        exit(-1);
    }

    printf("SENT COONTE\n");


    while(1){

        sleep(5);

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

        
    }

    close(socket_fd);

    exit(0);

}