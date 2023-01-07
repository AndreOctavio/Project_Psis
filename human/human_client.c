#include "../game.h"

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


/**
 * @brief thread that handles the game
 * 
 * @param arg
 * @return 0
 */
void * playing_loop_thread(void * arg){

    thread_args_t * thread_args = (thread_args_t *) arg;

    // /* Playing loop */
    int key = -1;
    int n_bytes;

    while(key != 27 && key!= 'q'){

        //printf("waiting for key\n");
        key = wgetch(thread_args->my_win);

        /* Checks if the ball is in game or in some other state */
        if(*thread_args->game_state == in_game){
            /* Checks if pressed key was movement */
            if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){

                /* Set movement message */
                thread_args->msg.msg_type = ball_movement;
                thread_args->msg.direction = key;
                thread_args->msg.player[thread_args->msg.player_num] = *thread_args->player;


                /* Send ball_movement to server */
                n_bytes = sendto(thread_args->socket_fd, &thread_args->msg, sizeof(message_t), 0, (const struct sockaddr *) &thread_args->server_addr, sizeof(thread_args->server_addr));
                if (n_bytes!= sizeof(message_t)){
                    perror("write");
                    exit(-1);
                }
            }

        /* if this is during the countdown, the ball goes back to the game with full health */
        } else if (*thread_args->game_state == countdown){

            pthread_mutex_lock(thread_args->lock);
            *thread_args->game_state = in_game;
            pthread_mutex_unlock(thread_args->lock);

            /* Set continue_game message */
            thread_args->msg.msg_type = continue_game;

            /* Send continue_game to server */
            n_bytes = sendto(thread_args->socket_fd, &thread_args->msg, sizeof(message_t), 0, (const struct sockaddr *) &thread_args->server_addr, sizeof(thread_args->server_addr));
            if (n_bytes!= sizeof(message_t)){
                perror("write");
                exit(-1);
            }
        } else if (*thread_args->game_state == game_over){
            break;
        }

    }

    printf("ending\n");

    /* close tcp socket */
    close(thread_args->socket_fd);
    sleep(1);

    return 0;

}


/**
 * @brief thread vai receber os field_status do servidor e vai atualizar a janela de jogo
 *  se o jogador morrer, a thread vai terminar ao detetar o fecho da socket
 * 
 * @param arg indica da thread. É usado para cada threda saber que valores ler do array
 * @return void* numero de primos encontrados
 */
void * listen_loop_thread(void * arg){
    
    int n_bytes;
    thread_args_t * thread_args = (thread_args_t *) arg;

    while(1){
        //printf("waiting for msg\n");
        /* Receive msg from server */
        n_bytes = recv(thread_args->socket_fd, &thread_args->msg, sizeof(message_t), 0);
        if (n_bytes!= sizeof(message_t)){
            perror("read");
            return 0;
        }

        /* MESSAGE RECEIVED PROCESSING */

        /* Checks if message recv is field_status */
        if(thread_args->msg.msg_type == field_status){

            clear_screen(thread_args->my_win);

            for(int i = 0; i < 10; i++){
                if(thread_args->msg.player[i].ch != -1){

                    /* If drawing this client's character, change color */
                    if(thread_args->msg.player[i].ch == thread_args->character){
                        draw_player(thread_args->my_win, &thread_args->msg.player[i], 1, 4);
                    } else {
                        /* Draw all the players */
                        draw_player(thread_args->my_win, &thread_args->msg.player[i], 1, 1);
                    }
                }
                if (thread_args->msg.bots[i].ch != -1){
                    /* Draw all the bots */
                    draw_player(thread_args->my_win, &thread_args->msg.bots[i], 1, 2);
                }
                if (thread_args->msg.prizes[i].ch != -1){
                    /* Draw all the bots */
                    draw_player(thread_args->my_win, &thread_args->msg.prizes[i], 1, 3);
                }
                thread_args->msg.bots[i].ch = -2;
            }
            wrefresh(thread_args->my_win);
            /* Print player HP in message window */
            show_all_health(thread_args->message_win, thread_args->msg.player);

        } else if (thread_args->msg.msg_type == health_0){
            /* Print text in message window */
            mvwprintw(thread_args->message_win, 1, 1, "                  ");
            mvwprintw(thread_args->message_win, 1, 1, "   GAME OVER :(   ");
            mvwprintw(thread_args->message_win, 2, 1, "                  ");
            mvwprintw(thread_args->message_win, 2, 1, " Press any key to ");
            mvwprintw(thread_args->message_win, 3, 1, " go back, ya got  ");
            mvwprintw(thread_args->message_win, 4, 1, " 10 seconds       ");
            mvwprintw(thread_args->message_win, 5, 1, "                  ");
            wrefresh(thread_args->message_win);
            sleep(1);
            /************  REPLACE WITH TIME LOOP COUNTDOWN  **************/

            *thread_args->game_state = countdown;

            /* countdown */
            for(int i = 9; (i > 0 && *thread_args->game_state == countdown); i--){
                mvwprintw(thread_args->message_win, 4, 1, "                  ");
                mvwprintw(thread_args->message_win, 4, 1, " %d seconds        ", i);
                wrefresh(thread_args->message_win);
                sleep(1);
            }

            if(*thread_args->game_state == countdown){

                pthread_mutex_lock(thread_args->lock);
                *thread_args->game_state = game_over;
                pthread_mutex_unlock(thread_args->lock);

                mvwprintw(thread_args->message_win, 3, 1, "                  ");
                mvwprintw(thread_args->message_win, 3, 1, " exit.            ");
                mvwprintw(thread_args->message_win, 4, 1, "                  ");
                mvwprintw(thread_args->message_win, 5, 1, " See you soon!    ");
                wrefresh(thread_args->message_win);
                sleep(1);

                break;

            } else if (*thread_args->game_state == in_game) {
                
                /* send reconnect message to server */
                thread_args->msg.msg_type = reconnect;
                send(thread_args->socket_fd, &thread_args->msg, sizeof(message_t), 0);

                /* Print player HP in message window */
                show_all_health(thread_args->message_win, thread_args->msg.player);
            }

        }
    }

    /* close tcp socket */
    close(thread_args->socket_fd);
    return 0;

}

/* main */
int main(int argc, char *argv[]){

    char character[64];
    player_info_t player;
    message_t msg;

	struct sockaddr_in server_addr;
    char server_address [100];
    int socket_fd, port;
    int n_bytes;

    strcpy(server_address, argv[1]);
    port = atoi(argv[2]);
    
    /* Player selects its character */
    printf("   *    Welcome to the game!    *   \n");
    printf("Select your character and press \"Enter\": \n");
    scanf("%s", character);
    while(!((character[0] >= 'a' && character[0] <= 'z') || (character[0] >= 'A' && character[0] <= 'Z'))) {
        printf("Character must be a letter, try again: \n");
        scanf(" %s", character);
    }

    player.ch = character[0];


    /* START OF TCP SETUP AND CONNECT */
    /* Create client socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1){
        perror("socket");
        exit(-1);
    }

    /* Set server address */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SOCK_PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_address);

	if(inet_pton(AF_INET, server_address, &server_addr.sin_addr) < 1){
		printf("no valid address: \n");
		exit(-1);
	}

    /* Send connection request to server */
    if(connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");
    /* END OF TCP SETUP AND CONNECT */



    /* Set connect message */
    msg.msg_type = connection;
    msg.player[0] = player;
    msg.player_num = 0;

    /* Set correct values to initialize bots and prizes section */
    for(int i = 0; i < 10; i++){
        msg.bots[i].ch = -1;
        msg.prizes[i].ch = -1;
    }

    /* Send connect message to server */
    n_bytes = send(socket_fd, &msg, sizeof(message_t), 0);
    if (n_bytes!= sizeof(message_t)){
        perror("write");
        exit(-1);
    }



    /* Receive Ball_information or lobby_full message from server */
    n_bytes = recv(socket_fd, &msg, sizeof(message_t), 0);
    if (n_bytes != sizeof(message_t)){
        perror("read");
        exit(-1);
    }

    /* if the lobby is full exit program */
    if (msg.msg_type == lobby_full){
        printf("The Lobby is full, try again later\n");
        sleep(5);
        close(socket_fd);
        exit(0);
    }

    /* if the character already exists inform on new ch */
    if(character[0] != msg.player[msg.player_num].ch){
        printf("Your Character already exists, you will be %c\n", msg.player[msg.player_num].ch);
        sleep(2);
    }



    character[0] = msg.player[msg.player_num].ch;

    /* Set player variable */
    player = msg.player[msg.player_num];

    /* SETUP NCURSES */
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
    draw_player(my_win, &player, true, 4);
    wrefresh(message_win);

    /* Setup arguments to call threads */
    struct thread_args_t args_thread;
    game_state_t game_state = in_game;
    pthread_mutex_t lock;

    args_thread.socket_fd = socket_fd;
    args_thread.my_win = my_win;
    args_thread.message_win = message_win;
    args_thread.server_addr = server_addr;
    args_thread.player = &player;
    args_thread.character = character[0];
    args_thread.msg = msg;
    args_thread.game_state = &game_state;
    args_thread.lock = &lock;

    /* Create threads */
    pthread_t thread_id[2];
    pthread_create(&thread_id[0], NULL, &playing_loop_thread, (void *) &args_thread);
    pthread_create(&thread_id[1], NULL, &listen_loop_thread, (void *) &args_thread);

    pthread_join(thread_id[0], NULL);
    pthread_join(thread_id[1], NULL);

    endwin();

    printf("Disconnected\n");

    exit(0);

}