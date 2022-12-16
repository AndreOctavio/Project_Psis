#include "../game.h"

WINDOW * message_win;
WINDOW * my_win;

/* draw_player()
 * Function draws a player, a bot or a prize in the game window
 * whith the given color
 */
void draw_player(WINDOW *win, player_info_t * player, int delete, int color){

    int p_x = player->pos_x;
    int p_y = player->pos_y;
    int ch;

    if(delete){
        ch = player->ch;
    }else{
        ch = ' ';
    }

    wmove(win, p_y, p_x);
    wattron(win, COLOR_PAIR(color));
    waddch(win, ch);
    wattroff(win, COLOR_PAIR(color));
}

/* clear_screen()
 * Function clears the game window printing blank spaces
 * in every position
 */
void clear_screen(WINDOW *win){

    for(int i = 1; i < WINDOW_SIZE - 1; i++){
        mvwprintw(win, i, 1, "                  ");
    }
}

/* show_all_health()
 * Function prints in the message window the health of all connected players
 */
void show_all_health(WINDOW * message_win, player_info_t player_data[10]){

    int i, j = 0;

    for(i = 1; i < 6; i++){
        mvwprintw(message_win, i, 1, "                  ");
    }

    for (i = 0; i < 10; i++){
        if (player_data[i].ch != -1){
            mvwprintw(message_win, j % 5 + 1, j / 5 * 11 + 1, "%c-> %d ", player_data[i].ch, player_data[i].hp);
            j++;
        }
    }
    wrefresh(message_win);

}

/* main */
int main(int argc, char *argv[]){

    int socket_fd;
    char character[64];
    char socket_name[64];
    player_info_t player;

    struct sockaddr_un local_client_addr;
	struct sockaddr_un server_addr;

    if(argc != 2 || strcmp(argv[1], "/tmp/sock_game") != 0){
        printf("Incorrect Arguments, please write server address\n");
        printf("This Game's Server Adress is \"/tmp/sock_game\"\n");
        exit(-1);
    }

    strcpy(socket_name, argv[1]);
    printf("Socket name: %s", socket_name);
    
    /* Player selects its character */
    printf("   ***    Welcome to the game!    ***   \n");
    printf("Select your character and press \"Enter\": \n");
    scanf("%s", character);
    while(!((character[0] >= 'a' && character[0] <= 'z') || (character[0] >= 'A' && character[0] <= 'Z'))) {
        printf("Character must be a letter, try again: \n");
        scanf(" %s", character);
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

    sprintf(local_client_addr.sun_path, "%s_%d", socket_name, getpid());
    //printf("Socket name: %s", socket_name);

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
    msg.player[0] = player;
    msg.player_num = 0;

    for(int i = 0; i < 10; i++){
        msg.bots[i].ch = -1;
        msg.prizes[i].ch = -1;
    }


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

    character[0] = msg.player[msg.player_num].ch;

    /* Set player variable */
    player = msg.player[msg.player_num];


    /* ncurses initialization */
    initscr();              // initialize ncurses
    cbreak();               // disable line buffering
    keypad(stdscr, TRUE);   // enable full keyboard mapping
    noecho();               // disable echoing of typed characters

    /* check if terminal supports color */
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }

    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);
	wrefresh(my_win);    // refresh the screen
    keypad(my_win, true);

    /* Create message window */
    WINDOW * message_win = newwin(7, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);
    wrefresh(message_win);

    /* Draw player in main window */
    draw_player(my_win, &player, true, 1);

    /* Print player HP in message window */
    mvwprintw(message_win, 1, 1, "%c-> %d", player.ch, player.hp);
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

                        /* If drawing this client's character, change color */
                        if(msg.player[i].ch == character[0]){
                            draw_player(my_win, &msg.player[i], 1, 4);
                        } else {
                            /* Draw all the players */
                            draw_player(my_win, &msg.player[i], 1, 1);
                        }
                    }
                    if (msg.bots[i].ch != -1){
                        /* Draw all the bots */
                        draw_player(my_win, &msg.bots[i], 1, 2);
                    }
                    if (msg.prizes[i].ch != -1){
                        /* Draw all the bots */
                        draw_player(my_win, &msg.prizes[i], 1, 3);
                    }
                    msg.bots[i].ch = -2;
                }
                wrefresh(my_win);
                /* Print player HP in message window */
                show_all_health(message_win, msg.player);

            } else if (msg.msg_type == health_0){
                /* Print player HP in message window */
                mvwprintw(message_win, 1, 1, "                  ");
                mvwprintw(message_win, 1, 1, "   GAME OVER :(   ");
                mvwprintw(message_win, 2, 1, "                  ");
                mvwprintw(message_win, 2, 1, "Just be better... ");
                mvwprintw(message_win, 3, 1, "                  ");
                mvwprintw(message_win, 4, 1, "                  ");
                mvwprintw(message_win, 5, 1, "                  ");
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