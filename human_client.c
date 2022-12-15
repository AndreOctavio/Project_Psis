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

void clear_screen(WINDOW *win){

    for(int i = 1; i < WINDOW_SIZE - 1; i++){
        mvwprintw(win, i, 1, "                  ");
    }
    wrefresh(win);
}

int main(int argc, char *argv[]){

    int socket_fd;
    char character[64];
    //char socket_name[64];
    player_info_t player;

    struct sockaddr_un local_client_addr;
	struct sockaddr_un server_addr;

    if(argc != 2){
        printf("Incorrect Arguments, please write server address\n");
        exit(-1);
    }
    int n;
    /* Player selects its character */
    printf("   ***    Welcome to the game!    ***   \n");
    printf("Select your character and press \"Enter\": \n");
    scanf("%s", character);
    while(!((character[0] > 'a' && character[0] < 'z') || (character[0] > 'A' && character[0] < 'Z'))) {
        printf("Character must be a letter, try again: \n");
        n = scanf(" %s", character);
    }

    

    printf("Socket name:");
    player.ch = character[0];

    /* Create client socket */
    local_client_addr.sun_family = AF_UNIX;
    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socket_fd == -1){
        perror("socket");
        exit(-1);
    }

    sprintf(local_client_addr.sun_path, "%s_%d", SOCKET_NAME, getpid());
    //printf("Socket name: %s", socket_name);

	unlink(local_client_addr.sun_path);

	int err = bind(socket_fd, (struct sockaddr *) &local_client_addr, sizeof(local_client_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

    /* Server infos */
    //printf("Socket name: %s", socket_name);
    //strcpy(socket_name, argv[2]);
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, SOCKET_NAME);


    /* Set connect message */
    message_t msg;
    msg.msg_type = connection;
    msg.player[1] = player;
    msg.player_num = 1;


    /* Send connect message to server */
    int n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    if (n_bytes!= sizeof(message_t)){
        perror("write");
        exit(-1);
    }

    /* Receive Ball_information or lobby_full message from server */
    n_bytes = recvfrom(socket_fd, &msg, sizeof(message_t), 0, NULL, NULL);
    if (n_bytes!= sizeof(message_t)){
        perror("read");
        exit(-1);
    }

    if (msg.msg_type == lobby_full){
        printf("The Lobby is full, try again later\n");
        sleep(5);
        close(socket_fd);
        exit(0);
    }

    if(character[0] != msg.player[msg.player_num].ch){
        printf("Your Character already exists, you will be %c\n", msg.player[msg.player_num].ch);
        sleep(2);
    }

    /* Set player variable */
    player = msg.player[msg.player_num];


    /* ncurses initialization */
    initscr();              // initialize ncurses
    cbreak();               // disable line buffering
    keypad(stdscr, TRUE);   // enable full keyboard mapping
    noecho();               // disable echoing of typed characters


    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);
	wrefresh(my_win);    // refresh the screen
    keypad(my_win, true);

    /* Create message window */
    WINDOW * message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);
    wrefresh(message_win);

    /* Draw player in main window */
    draw_player(my_win, &player, true);

    /* Print player HP in message window */
    mvwprintw(message_win, 1, 1, "%c %d", player.ch, player.hp);
    wrefresh(message_win);



    /* Playing loop */
    int key = -1;

    while(key != 27 && key!= 'q'){

        key = wgetch(my_win);

        /* Checks if pressed key was movement */
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){

            /* Set movement message */
            msg.msg_type = ball_movement;
            msg.direction = key;
            msg.player[msg.player_num] = player;

            /* Send ball_movement to server */
            n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
            if (n_bytes!= sizeof(message_t)){
                perror("write");
                exit(-1);
            }

            /* Receive field_status from server */
            n_bytes = recv(socket_fd, &msg, sizeof(message_t), 0);
            if (n_bytes!= sizeof(message_t)){
                perror("read");
                exit(-1);
            }

            /* MESSAGE RECEIVED PROCESSING */

            /* Checks if message recv is field_status */
            if(msg.msg_type == field_status){

                clear_screen(my_win);

                for(int i = 0; i < 10; i++){
                    if(msg.player[i].ch != -1){
                        /* Draw all the players */
                        draw_player(my_win, &msg.player[i], 1);
                    }
                    if (msg.bots[i].ch != -1){
                        /* Draw all the bots */
                        draw_player(my_win, &msg.bots[i], 1);
                    }
                    if (msg.prizes[i].ch != -1){
                        /* Draw all the bots */
                        draw_player(my_win, &msg.prizes[i], 1);
                    }
                    msg.bots[i].ch = -2;
                }
                /* Print player HP in message window */
                mvwprintw(message_win, 1, 1, "                  ");
                mvwprintw(message_win, 1, 1, "%c %d", msg.player[msg.player_num].ch, msg.player[msg.player_num].hp);
                wrefresh(message_win);
            } else if (msg.msg_type == health_0){
                /* Print player HP in message window */
                mvwprintw(message_win, 1, 1, "                  ");
                mvwprintw(message_win, 1, 1, "%c %d", msg.player[msg.player_num].ch, msg.player[msg.player_num].hp);
                wrefresh(message_win);
                sleep(1);
                mvwprintw(message_win, 1, 1, "                  ");
                mvwprintw(message_win, 1, 1, "    GAME OVER :(  ");
                mvwprintw(message_win, 2, 1, "                  ");
                mvwprintw(message_win, 2, 1, "Just get better...");
                wrefresh(message_win);
                /************  REPLACE WITH TIME LOOP COUNTDOWN  **************/
                sleep(5);
                endwin();
                close(socket_fd);
                exit(0);
            }
        }
    }


    /************  GO BACK TO THE BEGINNIG  **************/
    msg.msg_type = disconnect;

    /* Send ball_movement to server */
    n_bytes = sendto(socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    if (n_bytes!= sizeof(message_t)){
        perror("write");
        exit(-1);
    }

    endwin();
    close(socket_fd);

    exit(0);

}