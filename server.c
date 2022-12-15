#include "game.h"

WINDOW * message_win;

/* moove_player : 
* Function that updates the coordenates of 
* a player with a given direction; 
*/
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

void find_empty (int * x, int * y, player_info_t players[10], player_info_t bots[10], player_info_t prizes [10]) {

    int found = 0;

    while (!found) {
        found = 1;


        /* Use current time as seed for random generator */
        srand(clock());

        *x = (rand() % 18) + 1;
        *y = (rand() % 18) + 1;

        // Check if there's a player or bot in this coordenate
        for (int i = 0; i < 10; i++) {
            if (bots[i].ch != -1) {
                if ((bots[i].pos_x == *x) && (bots[i].pos_y == *y)) {
                    found = 0;
                    break;
                }
            }
            if (players[i].ch != -1) {
                if ((players[i].pos_x == *x) && (players[i].pos_y == *y)) {
                    found = 0;
                    break;
                }
            } 
            if (prizes[i].ch != -1) {
                if ((prizes[i].pos_x == *x) && (prizes[i].pos_y == *y)) {
                    found = 0;
                    break;
                }
            }
        }
    }
}

char ch_checker (char character, player_info_t players [10]) {

    int eureka = 0, lowercase;

    while (!eureka) {

        eureka = 1;

        for (int i = 0; i < 10; i++) {
            if (character == players[i].ch) {
                eureka = 0;

                /* Use current time as seed for random generator */
                srand(clock());

                /* If lowercase = 0 we get an uppercase ch, if lowercase = 1 we get a lowercase ch */
                lowercase = rand() % 2; 
                character = (rand() % 26) + 65 + lowercase * 32;

                break;
            }
        }
    }

    return character;
}

int main()
{	
    player_info_t player_data[10]; // Array to store all of the info of the current players in the game (10 max)
    player_info_t bot_data[10]; // Array to store all of the info about the bots (10 max)
    player_info_t prize_data[10];

    struct sockaddr_un client_addr[10]; // Array to store the players addresses
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    for (int i = 0; i < 10; i++) {
        player_data[i].ch = -1;
        bot_data[i].ch = -1;
        prize_data[i].ch = -1;
    }

    int current_players = 0; // Keeps the current amount of players in the game
    int i, found = 0, clear_to_move = 1, cnt = 0;

    // Create the socket for the server
    int sock_fd;
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    // Check for it was sucessfully created
    if (sock_fd == -1){ 
	    perror("socket: ");
	    exit(-1);
    }

    struct sockaddr_un local_addr;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, SOCKET_NAME);
    unlink(SOCKET_NAME);

    int err = bind(sock_fd, (const struct sockaddr *)&local_addr, sizeof(local_addr)); //bind the address

    // Check for it was sucessfully binded
    if(err == -1) { 
	    perror("bind");
	    exit(-1);
    }

    // ncurses initialization
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
    int pos_x, pos_y;
    char ch;
    int n_bytes;
    int spawn_prizes = 1;

    message_t msg;

    clock_t time_init;
    double time_passed;

    struct sockaddr_un tmp;

    while (1)
    {

        // Receive message from the clients
        n_bytes = recvfrom(sock_fd, &msg, sizeof(message_t), 0, (struct sockaddr *)&tmp, &client_addr_size);
        if (n_bytes!= sizeof(message_t)){
            continue;
        }
        
        /*------PROCESS THE VARIOUS TYPES OF MESSAGES------*/

        /* CONNECT MESSAGE */
        if ((msg.msg_type == connection) && ((current_players < 10) || (msg.bots[0].ch == '*'))) { // Maximum of 10 players

            /* Iterate through bots or do once if human player */
            while (msg.player_num > 0) {

                /* If the client is a bot */
                if (msg.bots[0].ch == '*') {
                    
                    if (spawn_prizes) {

                        for (i = 0; i < 5; i++) {
                            find_empty (&pos_x, &pos_y, player_data, bot_data, prize_data);

                            prize_data[i].ch = msg.prizes[i].ch;
                            prize_data[i].hp = msg.prizes[i].hp;

                            prize_data[i].pos_x = pos_x;
                            prize_data[i].pos_y = pos_y;

                            // Draw prize
                            wmove(my_win, pos_x, pos_y);
                            waddch(my_win, ch);
                            wrefresh(my_win);
                        }

                        spawn_prizes = 0;
                    }

                    find_empty (&pos_x, &pos_y, player_data, bot_data, prize_data);
                    
                    bot_data[msg.player_num - 1].ch = '*';
                    bot_data[msg.player_num - 1].pos_x = pos_x;
                    bot_data[msg.player_num - 1].pos_y = pos_y;

                    ch = '*';

                /* If the client is a player */
                } else {

                    find_empty (&pos_x, &pos_y, player_data, bot_data, prize_data);

                    for (i = 0; i < 10; i++) {
                        if (player_data[i].ch == -1) { // Get the first avaiable space in the array

                            player_data[i].ch = ch_checker(msg.player[msg.player_num].ch, player_data);

                            player_data[i].pos_x = pos_x;
                            player_data[i].pos_y = pos_y;
                            player_data[i].hp = 10;

                            client_addr [i] = tmp;

                            current_players++; // Increase the number of players playing the game
                            ch = player_data[i].ch;

                            break;
                        }
                    }
                }

                // Draw the player/bot
                wmove(my_win, pos_y, pos_x);
                waddch(my_win, ch);
                wrefresh(my_win);	

                /* If the client is a player send ball_information message */
                if (ch != '*') {

                    msg.msg_type = ball_information;
                    msg.player_num = i;

                    for (i = 0; i < 10; i++) { // Copy the current player data
                        msg.player[i] = player_data[i];
                        msg.bots[i] = bot_data[i];
                        msg.prizes[i] = prize_data[i];
                    }

                    /* BALL_INFORMATION MESSAGE */
                    sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &tmp, client_addr_size);

                    msg.player_num = 1;
                }

                msg.player_num--;

            }

        } else if (current_players == 10 && msg.msg_type == connection) { //Already 10 players in the game

            msg.msg_type = lobby_full;

            /* LOBBY_FULL MESSAGE */
            sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &tmp, client_addr_size);
            continue;

        }

        /* BALL_MOVEMENT MESSAGE */
        if(msg.msg_type == ball_movement){

            // Check if the client sending the message is in fact a player in the game
            if (player_data[msg.player_num].ch == msg.player[msg.player_num].ch) {
                
                // Save the old position
                pos_x = player_data[msg.player_num].pos_x;
                pos_y = player_data[msg.player_num].pos_y;

                // Calculate the new position
                moove_player(&player_data[msg.player_num], msg.direction);

                //Check if the player hit another player or a bot
                for(int j = 0 ; j < 10; j++){

                    // Check if the position in the array has a player (different then the player moving)
                    if((player_data[j].ch != -1) && (j != msg.player_num)) {
                        
                        // See if the player is in the position that our current player is moving into
                        if (player_data[j].pos_x == player_data[msg.player_num].pos_x && player_data[j].pos_y == player_data[msg.player_num].pos_y){ //Player hits another player

                            // If the player has less(or equal to) than 9 lives increment HP
                            if (player_data [msg.player_num].hp <= 9) {
                                player_data [msg.player_num].hp++;
                            }

                            player_data [j].hp--; // Decrease HP of the player that was hit

                            if (player_data [j].hp == 0) { // If the player that was hit has 0 lives than its GAME OVER

                                /* HEALTH_0 MESSAGE */
                                msg.msg_type = health_0;
                                msg.player[j].ch = player_data [j].ch;
                                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[j], client_addr_size);
                                wmove(my_win, player_data [j].pos_y, player_data [j].pos_x);
                                waddch(my_win,' ');
                                player_data [j].ch = -1;

                            } 

                            clear_to_move = 0;

                            break;
                        }
                    }

                    // Look for a bot
                    if (bot_data[j].ch != -1) {

                        //Player hits a bot
                        if ((bot_data[j].pos_x == player_data[msg.player_num].pos_x) && (bot_data[j].pos_y == player_data[msg.player_num].pos_y)){
                            clear_to_move = 0;
                            break;
                        } 

                    }
                    // else if (/*chocar com um prize*/){
                    // }
                }

                if (clear_to_move) {

                    // Delete player from old position
                    wmove(my_win, pos_y, pos_x);
                    waddch(my_win,' ');

                    // Draw player in the new position
                    wmove(my_win, player_data[msg.player_num].pos_y, player_data[msg.player_num].pos_x);
                    waddch(my_win, player_data[msg.player_num].ch);
                    wrefresh(my_win);	

                } else { // Keep the old coordenates
        
                    player_data[msg.player_num].pos_x = pos_x;
                    player_data[msg.player_num].pos_y = pos_y;
                }
                
                /* FIELD_STATUS MESSAGE */
                msg.msg_type = field_status;

                for (i = 0; i < 10; i++) { //Copy the current player data
                    msg.player[i] = player_data[i];
                    msg.bots[i] = bot_data[i];
                }

                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[msg.player_num], client_addr_size);

                clear_to_move = 1; 

            } else if (bot_data[msg.player_num].ch == msg.bots[msg.player_num].ch){

                // Save the old position
                pos_x = bot_data[msg.player_num].pos_x;
                pos_y = bot_data[msg.player_num].pos_y;

                // Calculate the new position
                moove_player(&bot_data[msg.player_num], msg.direction);

                //Check if the bot hit another player or a bot
                for(int j = 0 ; j < 10; j++){

                    // Check if the position in the array has a player
                    if(player_data[j].ch != -1) {
                        
                        // See if the player is in the position that the bot is moving into
                        if (player_data[j].pos_x == bot_data[msg.player_num].pos_x && player_data[j].pos_y == bot_data[msg.player_num].pos_y){ //Bot hits another player

                            player_data [j].hp--; // Decrease HP of the player that was hit

                            if (player_data [j].hp == 0) { // If the player that was hit has 0 lives than its GAME OVER

                                /* HEALTH_0 MESSAGE */
                                msg.msg_type = health_0;
                                msg.player[j].ch = player_data [j].ch;
                                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[j], client_addr_size);
                                wmove(my_win, player_data [j].pos_y, player_data [j].pos_x);
                                waddch(my_win,' ');
                                player_data [j].ch = -1;

                            } 

                            clear_to_move = 0;

                            break;
                        }
                    }

                    // Look for a bot
                    if ((bot_data[j].ch != -1) && (bot_data[j].ch != msg.bots[msg.player_num].ch)) {

                        // Bot hits a bot
                        if (bot_data[j].pos_x == bot_data[msg.player_num].pos_x && bot_data[j].pos_y == bot_data[msg.player_num].pos_y){
                            clear_to_move = 0;
                            break;
                        } 

                    }
                    // else if (/*chocar com um prize*/){
                    // }
                }

                if (clear_to_move) {

                    // Delete bot from old position
                    wmove(my_win, pos_y, pos_x);
                    waddch(my_win,' ');

                    // Draw bot in the new position
                    wmove(my_win, bot_data[msg.player_num].pos_y, bot_data[msg.player_num].pos_x);
                    waddch(my_win, bot_data[msg.player_num].ch);
                    wrefresh(my_win);	

                } else { // Keep the old coordenates
        
                    bot_data[msg.player_num].pos_x = pos_x;
                    bot_data[msg.player_num].pos_y = pos_y;
                }
                clear_to_move = 1;
                
            }
        }

        /* DISCONNECT MESSAGE */
        if (msg.msg_type == disconnect) {
            player_data [msg.player_num].ch = -1;

            // Delete player from the screen
            wmove(my_win, player_data [msg.player_num].pos_y, player_data [msg.player_num].pos_x);
            waddch(my_win,' ');
            wrefresh(my_win);
        }

    }

  	endwin();			/* End curses mode */

	return 0;
}