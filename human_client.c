#include "game.h"

WINDOW * message_win;


void draw_player(WINDOW *win, player_info_t * player, int delete){
    int ch;
    if(delete){
        ch = player->ch;
    }else{
        ch = ' ';
    }
    int p_x = player->x;
    int p_y = player->y;
    wmove(win, p_y, p_x);
    waddch(win,ch);
    wrefresh(win);
}


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
    keypad(my_win, true);
    /* creates a window and draws a border */
    message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);	
	wrefresh(message_win);


    /* Playing loop */
    int key = -1;
    while(key != 27 && key!= 'q'){
        key = wgetch(my_win);

        msg.msg_type = ball_movement;

        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){
            msg.player = player;
            msg.direction = key;

            /* Send message to server */
            n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));

            if (n_bytes!= sizeof(message_t)){
                perror("write");
                exit(-1);
            }

            /* Receive message from server */
            n_bytes = recv(socket_fd, &msg, sizeof(message_t), 0);

            if (n_bytes!= sizeof(message_t)){
                perror("read");
                exit(-1);
            }
            
            my_win = &msg.game_state;
            wrefresh(my_win);

            mvwprintw(&msg.message_win, 1,1,"%c key pressed", key);
            wrefresh(&msg.message_win);	

        }


    }


    close(socket_fd);

    return 0;

}