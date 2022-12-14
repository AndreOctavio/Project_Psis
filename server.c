#include "game.h"

WINDOW * message_win;

void initialization (struct sockaddr_un client_addr [10], player_info_t players [10]) {

    for (int i = 0; i < 10; i++) {
        //client_addr [i] = NULL; //se não funcionar usar NULL
        players [i].ch = -1;
    }
}

void moove_player (player_info_t * player, int direction){
    if (direction == KEY_UP){
        if (player->pos_y  != 1){
            player->pos_y --;
        }
    }
    if (direction == KEY_DOWN){
        if (player->pos_y  != WINDOW_SIZE-2){
            player->pos_y ++;
        }
    }
    

    if (direction == KEY_LEFT){
        if (player->pos_x  != 1){
            player->pos_x --;
        }
    }
    if (direction == KEY_RIGHT)
        if (player->pos_x  != WINDOW_SIZE-2){
            player->pos_x ++;
    }
}



int main()
{	
    int n = 0;
    player_info_t player_data [10]; //Array store all of the info of the current players in the game (10 max)

    struct sockaddr_un client_addr [10];
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    initialization (client_addr, player_data);

    int current_players = 0; //Keeps the current amount of players in the game
    int i, found = 0, clear = 0;

    //Create the socket for the server
    int sock_fd;
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    //Check for it was sucessfully created
    if (sock_fd == -1){ 
	    perror("socket: ");
	    exit(-1);
    }

    struct sockaddr_un local_addr;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, SOCKET_NAME);
    unlink(SOCKET_NAME);

    int err = bind(sock_fd, (const struct sockaddr *)&local_addr, sizeof(local_addr)); //bind the address

    //Check for it was sucessfully binded
    if(err == -1) { 
	    perror("bind");
	    exit(-1);
    }

    //ncurses initialization
	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* Creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    /* Create message window */
    WINDOW * message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);
    wrefresh(message_win);

    /* Information about the character */
    int ch, pos_x, pos_y;
    int n_bytes;
    int prizes_to_spawn;

    int  direction;

    message_t msg;

    clock_t time_init;
    double time_passed;

    struct sockaddr_un tmp;

    while (1)
    {

        //Receive message from the clients
        n_bytes = recvfrom(sock_fd, &msg, sizeof(message_t), 0, (struct sockaddr *)&tmp, &client_addr_size);
        n++;
        
        // if (current_players != 0) {
        //     time_passed = (double)(clock() - time_init) / CLOCKS_PER_SEC;
        //     if (time_passed > 5.0) {
        //         prizes_to_spawn = time_passed/5.0;

        //         // for (int n; n < prizes_to_spawn; n++) {
        //         //     if ()
        //         //     prize_spawn();
        //         // }

        //         time_init = clock();
        //     }
        // }

        //Check if received correct structure
        if (n_bytes!= sizeof(message_t)){
            continue;
        }
        
        /*Process the various types of messages*/

        //Connect message
        if ((msg.msg_type == connection) && (current_players < 10)) { //Maximum of 10 players

            if (current_players == 0) {
                time_init = clock ();
            }

            while (!clear) {
                pos_x = rand() % 21;
                pos_y = rand() % 21;
                
                clear = 1;

                for (i = 0; i < 10; i++) {
                    if (player_data[i].ch != -1) {
                        if ((player_data[i].pos_x == pos_x) && (player_data[i].pos_y == pos_y)) {
                            clear = 0;
                            break;
                        }
                    }
                }
            }

            current_players++; //Increase the number of players playing the game
            
            //Keep the new players information
            for (i = 0; i < 10; i++) {
                if (player_data[i].ch == -1) {
                    player_data[i].ch = msg.player[msg.player_num].ch;

                    player_data[i].pos_x = pos_x;
                    player_data[i].pos_y = pos_y;
                    player_data[i].hp = 10;

                    client_addr [i] = tmp;

                    break;
                }
            }

            /* draw mark on new position */
            wmove(my_win, player_data[i].pos_y, player_data [i].pos_x);
            waddch(my_win, player_data[i].ch);
            wrefresh(my_win);	


            msg.msg_type = ball_information;
            msg.player_num = i;

            for (i = 0; i < 10; i++) {
                msg.player[i] = player_data[i];
            }

            //Ball_information message
            sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &tmp, client_addr_size);

        } else if (current_players == 10 && msg.msg_type == connection){ //Already 10 players in the game

            msg.msg_type = lobby_full;

            //Lobby_full message
            sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &tmp, client_addr_size);
            continue;

        }

        //Movement message
        if(msg.msg_type == ball_movement){
            if (player_data[msg.player_num].ch == msg.player[msg.player_num].ch){
                found = 1;
            }

            if (found) {

                ch = player_data[msg.player_num].ch;
                pos_x =  player_data[msg.player_num].pos_x;
                pos_y =  player_data[msg.player_num].pos_y;

                /*deletes old place */
                wmove(my_win, pos_y, pos_x);
                waddch(my_win,' ');

                /* calculates new direction */
                direction = msg.direction;

                /* Print player HP */
                mvwprintw(message_win, 1, 1, "                  ");
                mvwprintw(message_win, 1, 1, "b4_mv %d %d %d", player_data[msg.player_num].pos_x, player_data[msg.player_num].pos_y, direction);
                wrefresh(message_win);
                sleep(0.5);

                /* calculates new mark position */
                moove_player(&player_data[msg.player_num], direction);

                /* Print player HP */
                mvwprintw(message_win, 1, 1, "                  ");
                mvwprintw(message_win, 1, 1, "af_mv %d %d", player_data[msg.player_num].pos_x, player_data[msg.player_num].pos_y);
                wrefresh(message_win);
                sleep(0.5);

                // player_data[msg.player_num].pos_x = pos_x;
                // player_data[msg.player_num].pos_y = pos_y;

                //Check if it the player hit another player
                for(int j = 0 ; j < 10; j++){
                    if((player_data[j].ch != -1) && (j != msg.player_num)) {
                        if (player_data[j].pos_x == pos_x && player_data[j].pos_y == pos_y){ //Player hits another player
                            if (player_data [msg.player_num].hp <= 9) { //If the player has less(or equal to) than 9 lives increment HP
                                player_data [msg.player_num].hp++;
                            }

                            player_data [j].hp--; //Decrease HP of the player that was hit

                            if (player_data [j].hp == 0) { //If the player that was hit has 0 lives than its GAME OVER
                                //Health_0 message;
                                msg.msg_type = health_0;
                                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[j], client_addr_size);
                                player_data [j].ch = -1;
                            }
                        }
                    }
                    // else if (/*chocar com um prize*/){
                    // }
                }
                
                /* draw mark on new position */
                wmove(my_win, player_data[msg.player_num].pos_y, player_data[msg.player_num].pos_x);
                waddch(my_win, ch);
                wrefresh(my_win);	

                //Field_status message
                msg.msg_type = field_status;

                for (i = 0; i < 10; i++) {
                    msg.player[i] = player_data[i];
                }

                /* Print player HP */
                mvwprintw(message_win, 1, 1, "                  ");
                mvwprintw(message_win, 1, 1, "sending");
                wrefresh(message_win);
                sleep(0.5);

                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[msg.player_num], client_addr_size);

                found = 0;
            }
        }

        //Disconnect message
        if (msg.msg_type == disconnect) {
            player_data [msg.player_num].ch = -1;
        }

    }
  	endwin();			/* End curses mode		  */

	return 0;
}