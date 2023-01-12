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
void show_all_health(WINDOW * message_win, player_info_t player_data[MAX_PLAYERS]){

    int i, j = 0;

    /* Clear message window */
    for(i = 1; i < 6; i++){
        mvwprintw(message_win, i, 1, "                  ");
    }

    /* Print health of first 10 players */
    for (i = 0; i < 10; i++){
        if (player_data[i].ch != -1){
            if(player_data[i].hp == 0){
                mvwprintw(message_win, j % 5 + 1, j / 5 * 11 + 1, "%c->Dead", player_data[i].ch);
            } else {
                mvwprintw(message_win, j % 5 + 1, j / 5 * 11 + 1, "%c-> %d ", player_data[i].ch, player_data[i].hp);
            }
            j++;
        }
    }
    
    wrefresh(message_win);

}


/**
 * @brief thread that handles keyboard input from the user
 * 
 * @param arg arguments needed for the thread
 * @return nothing
 */
void * keyboard_thread(void * arg){

    thread_args_t * thread_args = (thread_args_t *) arg;
    message_t msg;
    
    msg.player_num = thread_args->player_id;

    // /* Playing loop */
    int key = -1;
    int n_bytes;

    while(key != 27 && key!= 'q'){

        key = wgetch(thread_args->my_win);

        /* Checks if the ball is in game or in some other state */
        if(*thread_args->game_state == in_game){
            /* Checks if pressed key was movement */
            if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){

                /* Set movement message */
                msg.msg_type = ball_movement;
                msg.direction = key;
                msg.player[msg.player_num] = *thread_args->player;


                /* Send ball_movement to server */
                n_bytes = sendto(thread_args->socket_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *) &thread_args->server_addr, sizeof(thread_args->server_addr));
                if (n_bytes!= sizeof(message_t)){
                    perror("write");
                    exit(-1);
                }
            }

        /* if this is during the countdown, the ball goes back to the game with full health */
        } else if (*thread_args->game_state == countdown){


            /* Set game state to in_game so that the communication thread knows a key has been pressed */
            pthread_mutex_lock(thread_args->lock);
            *thread_args->game_state = in_game;
            pthread_mutex_unlock(thread_args->lock);

        /* if the game is over, close thread */
        } else if (*thread_args->game_state == game_over){
            break;
        }

    }

    /* close tcp socket */
    close(thread_args->socket_fd);

    return 0;

}


/**
 * @brief this thread will receive the messages from the server and update the game window
 * If the player dies, the thread will end when it detects the closure of the socket
 * 
 * @param arg arguments needed for the thread
 * @return nothing
 */
void * communication_thread(void * arg){
    
    int n_bytes, color;
    thread_args_t * thread_args = (thread_args_t *) arg;
    msg_field_update msg;
    message_t msg_send;
    player_info_t player_data[MAX_PLAYERS];

    /* Receiving loop */
    while(1){
        
        /* Receive msg from server */
        n_bytes = recv(thread_args->socket_fd, &msg, sizeof(msg_field_update), 0);
        if (n_bytes!= sizeof(msg_field_update)){
            perror("read_field_status");
            return 0;
        }

        /* MESSAGE RECEIVED PROCESSING */

        /* Checks if message recv is field_status */
        if(msg.msg_type == field_status){
            
            /* If drawing this client's character, change color */
            if(msg.new_status.ch == thread_args->player->ch){
                color = 4;
            } else if (msg.new_status.ch == '*'){
                color = 2;
            } else if (msg.new_status.ch >= '1' && msg.new_status.ch <= '5'){
                color = 3;
            } else {
                color = 1;
            }

            pthread_mutex_lock(thread_args->lock);
            /* Draw the player in the game window */
            if(msg.old_status.ch != -1){
                draw_player(thread_args->my_win, &msg.old_status, 0, color);
            }
            if(msg.new_status.ch != -1){
                draw_player(thread_args->my_win, &msg.new_status, 1, color);
            }

            /* Refresh the window */
            wrefresh(thread_args->my_win);

            /* update local array of the player data */
            player_data[msg.arr_position] = msg.new_status;

            /* Print player HP in message window */
            show_all_health(thread_args->message_win, player_data);

            pthread_mutex_unlock(thread_args->lock);


            






            // /* This section of code will draw the field status and update the windows */
            // clear_screen(thread_args->my_win);

            // /* Draw all the players, bots and prizes */
            // for(int i = 0; i < 10; i++){

            //     /* If the player is in the game, draw it */
            //     if(thread_args->msg.player[i].ch != -1){

            //         /* If drawing this client's character, change color */
            //         if(thread_args->msg.player[i].ch == thread_args->character){
            //             draw_player(thread_args->my_win, &thread_args->msg.player[i], 1, 4);
            //         } else {
            //             /* Draw all the players */
            //             draw_player(thread_args->my_win, &thread_args->msg.player[i], 1, 1);
            //         }
            //     }
            //     /* If the bot is in the game, draw it */
            //     if (thread_args->msg.bots[i].ch != -1){
            //         /* Draw all the bots */
            //         draw_player(thread_args->my_win, &thread_args->msg.bots[i], 1, 2);
            //     }
            //     /* If the prize is in the game, draw it */
            //     if (thread_args->msg.prizes[i].ch != -1){
            //         /* Draw all the bots */
            //         draw_player(thread_args->my_win, &thread_args->msg.prizes[i], 1, 3);
            //     }
            // }

        /* If the message received is health_0 start the countdown */
        } else if (msg.msg_type == health_0){

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

            /* Set game_state to countdown such that the keyboard thread 
             * knows how to handle keyboard input */
            pthread_mutex_lock(thread_args->lock);
            *thread_args->game_state = countdown;
            pthread_mutex_unlock(thread_args->lock);

            /* countdown */
            for(int i = 9; (i > 0); i--){

                /* Checks if the keyboard thread has changed the state of the game
                 * If the keyboard thread has detected an input it will change the game_state to in_game */
                pthread_mutex_lock(thread_args->lock);
                if(*thread_args->game_state != countdown){
                    pthread_mutex_unlock(thread_args->lock);
                    break;
                }
                pthread_mutex_unlock(thread_args->lock);

                mvwprintw(thread_args->message_win, 4, 1, "                  ");
                mvwprintw(thread_args->message_win, 4, 1, " %d seconds        ", i);
                wrefresh(thread_args->message_win);
                sleep(1);
            }

            /* If the game_state is still countdown, it means that the countdown has ended
             * was not interrupted and the game is over */
            pthread_mutex_lock(thread_args->lock);
            if(*thread_args->game_state == countdown){

                /* Set game_state to game_over such that the keyboard thread 
                 * knows how to handle keyboard input */
                *thread_args->game_state = game_over;
                pthread_mutex_unlock(thread_args->lock);

                mvwprintw(thread_args->message_win, 3, 1, "                  ");
                mvwprintw(thread_args->message_win, 3, 1, " exit.            ");
                mvwprintw(thread_args->message_win, 4, 1, "                  ");
                mvwprintw(thread_args->message_win, 5, 1, " See you soon!    ");
                wrefresh(thread_args->message_win);
                sleep(1);

                break;

            /* If the game_state is in_game, it means that the keyboard thread has detected
             * an input and the game is still going on */
            } else if (*thread_args->game_state == in_game) {

                pthread_mutex_unlock(thread_args->lock);
                
                /* send reconnect message to server */
                msg_send.msg_type = reconnect;
                send(thread_args->socket_fd, &msg_send, sizeof(message_t), 0);

                // /* Print player HP in message window */
                // show_all_health(thread_args->message_win, thread_args->msg.player);
                continue;
            }
            pthread_mutex_unlock(thread_args->lock);

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
	server_addr.sin_port = htons(port);
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
        perror("read_ball_info");
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

    /* start color mode */
    start_color();
    /* initialize color pairs */
    init_pair(1, COLOR_BLUE, COLOR_BLACK);      // other players in the game
    init_pair(2, COLOR_RED, COLOR_BLACK);       // bot
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);    // prize
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // client's player

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);
	wrefresh(my_win);    // refresh the screen
    keypad(my_win, true);

    /* Create message window */
    WINDOW * message_win = newwin(7, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);
    wrefresh(message_win);


    /* Draw all the players, bots and prizes */
    for(int i = 0; i < 10; i++){

        /* If the player is in the game, draw it */
        if(msg.player[i].ch != -1){

            /* If drawing this client's character, change color */
            if(msg.player[i].ch == character[0]){
                draw_player(my_win, &msg.player[i], 1, 4);
            } else {
                /* Draw all the players */
                draw_player(my_win, &msg.player[i], 1, 1);
            }
        }
        /* If the bot is in the game, draw it */
        if (msg.bots[i].ch != -1){
            /* Draw all the bots */
            draw_player(my_win, &msg.bots[i], 1, 2);
        }
        /* If the prize is in the game, draw it */
        if (msg.prizes[i].ch != -1){
            /* Draw all the bots */
            draw_player(my_win, &msg.prizes[i], 1, 3);
        }
    }

    wrefresh(my_win);    // refresh the screen


    /* Setup arguments to call threads */
    struct thread_args_t args_thread;
    game_state_t game_state = in_game;
    pthread_mutex_t lock;
    msg_field_update msg_field_status;

    args_thread.socket_fd = socket_fd;
    args_thread.my_win = my_win;
    args_thread.message_win = message_win;
    args_thread.server_addr = server_addr;
    args_thread.player = &player;
    args_thread.player_id = msg.player_num;

    //printf ("player_id: %d\n", args_thread.player_id);
    fflush(stdout);

    sleep(3);

    args_thread.game_state = &game_state;
    args_thread.lock = &lock;

    /* Create threads */
    pthread_t thread_id[2];
    pthread_create(&thread_id[0], NULL, &keyboard_thread, (void *) &args_thread);
    pthread_create(&thread_id[1], NULL, &communication_thread, (void *) &args_thread);

    /* Wait for threads to finish */
    pthread_join(thread_id[0], NULL);
    pthread_join(thread_id[1], NULL);

    /* close ncurses */
    endwin();

    printf("Disconnected\n");

    exit(0);

}