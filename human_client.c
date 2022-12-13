#include "game.h"


int main(){

    char character;

    /* Player selects its character */
    printf("Select your character and press \"Enter\": \n");
    scanf("%c", &character);

    player_info_t player;
    player.ch = character;

    /* Setup message */
    message_t msg;
    msg.msg_type = 0;
    msg.player = player;

    int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (socket_fd == -1){
        perror("socket: ");
        exit(-1);
    }

    /* Create client socket */
    struct sockaddr_un local_client_addr;
    local_client_addr.sun_family = AF_UNIX;
    sprintf(local_client_addr.sun_path, "%s_%d", SOCKET_NAME, getpid());

	unlink(local_client_addr.sun_path);
	int err = bind(socket_fd, (struct sockaddr *) &local_client_addr, sizeof(local_client_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

    printf("Socket created | Ready to send | Ready to recieve\n");

    /* Server infos */
	struct sockaddr_un server_addr;
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCKET_NAME);

    /* Send connect message to server */
    int n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    if (n_bytes!= sizeof(message_t)){
        perror("write");
        exit(-1);
    }

    /* Receive Ball_information message from server */

    n_bytes = recvfrom(socket_fd, &msg, sizeof(message_t), 0, NULL, NULL);

    if (n_bytes!= sizeof(message_t)){
        perror("read");
        exit(-1);
    }

    /* Set player's position */
    player.pos_x = msg.player.pos_x;
    player.pos_y = msg.player.pos_y;

    /* ncurses initialization */
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    /* Creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);
    wrefresh(my_win);

    /* Information about the character */
    int ch, pos_x, pos_y;
    direction_t  direction;


    close(socket_fd);

    return 0;

}