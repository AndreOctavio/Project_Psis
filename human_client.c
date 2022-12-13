#include "game.h"

WINDOW * message_win;
WINDOW * my_win;


void draw_player(WINDOW *win, player_info_t * player, int delete){

    int p_x = player->pos_x;
    int p_y = player->pos_y;
    int ch;

    if(delete){
        ch = player->ch;
    }else{
        ch = ' ';
    }

    wmove(win, p_y, p_x);
    waddch(win, ch);
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
    msg.player[0] = player;
    msg.player_num = 0;


    /* Create client socket */
    struct sockaddr_un local_client_addr;
    local_client_addr.sun_family = AF_UNIX;
    int socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socket_fd == -1){
        perror("socket: ");
        exit(-1);
    }

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

    printf("Received message from server %c\n", msg.player[msg.player_num].ch); // DEBUG ---------------
    scanf("%c", &character); // DEBUG ---------------

    /* Set player's position */
    player.pos_x = msg.player[msg.player_num].pos_x;
    player.pos_y = msg.player[msg.player_num].pos_y;



    /* ncurses initialization */
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);

	wrefresh(my_win);
    keypad(my_win, true);

    draw_player(my_win, &player, true);


    /* Create message window */
    WINDOW * message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);
    wrefresh(message_win);

    /* Print player HP */
    mvwprintw(message_win, 1, 1, "%c %d", player.ch, player.hp);
    wrefresh(message_win);




    /* Playing loop */
    int key = -1;
    while(key != 27 && key!= 'q'){
        key = wgetch(my_win);


        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){

            msg.msg_type = ball_movement;
            msg.direction = key;
            msg.player[msg.player_num] = player;

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

            if(msg.msg_type == field_status){
                wclear();
                box(my_win, 0 , 0);
                for(int i; i < 10, i++){
                    if(i != -1){
                        /* Draw all the players */
                        draw_player(my_win, &player, 1);
                    }
                }
            }
            

            mvwprintw(message_win, 1,1,"%c key pressed", key);
            wrefresh(message_win);	

        }


    }


    close(socket_fd);

    return 0;

}