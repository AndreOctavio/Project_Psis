#include "../game.h"

WINDOW * message_win;

/* moove_player() : 
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

/* find_empty() :
* Function that generates a random coordenate 
* that is not occupied;
*/
void find_empty (int * x, int * y, player_info_t players[10], player_info_t bots[10], player_info_t prizes [10]) {

    int found = 0;

    while (!found) {
        found = 1;


        /* Use current time as seed for random generator */
        srand(clock());

        *x = (rand() % 18) + 1;
        *y = (rand() % 18) + 1;

        for (int i = 0; i < 10; i++) {
            if (bots[i].ch != -1) {
                /* Check if there's a bot in this coordenate */
                if ((bots[i].pos_x == *x) && (bots[i].pos_y == *y)) {
                    found = 0;
                    break;
                }
            }
            if (players[i].ch != -1) {
                /* Check if there's a players in this coordenate */
                if ((players[i].pos_x == *x) && (players[i].pos_y == *y)) {
                    found = 0;
                    break;
                }
            } 
            if (prizes[i].ch != -1) {
                /* Check if there's a prizes in this coordenate */
                if ((prizes[i].pos_x == *x) && (prizes[i].pos_y == *y)) {
                    found = 0;
                    break;
                }
            }
        }
    }
}

/* ch_checker() :
* Function that checks if a character is already 
* being used, if it is assigns one that isn't;
*/
char ch_checker (char character, player_info_t players[10]) {

    int eureka = 0, lowercase;

    /* Run loop until we get a ch that isn't being used */
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


/* show_all_health() :
 * Function prints in the message window
 * the health of all connected players;
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


int main()
{	

    /* player_data - stores all the info regarding the current players;
    *  bot_data - stores all the info regarding the bots in the game; 
    *  prize_data - stores all the info regarding the prizes in the game; */
    player_info_t player_data[10]; 
    player_info_t bot_data[10]; 
    player_info_t prize_data[10]; 

    struct sockaddr_un client_addr[10]; // Array to store the players addresses
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    /* Initialize the data arrays with -1 in ch */
    for (int i = 0; i < 10; i++) {
        player_data[i].ch = -1;
        bot_data[i].ch = -1;
        prize_data[i].ch = -1;
    }

    int current_players = 0; // Keeps the current amount of players in the game
    int n_prizes = 0; // Keeps the number the current amount of prizes in the game
    int clear_to_move = 1; // Flag to check if the player/bot can move
    int spawn_prizes = 1, spawn_bots = 1; // Flags, only spawn the initial bots and prizes ONCE

    int i;

    message_t msg;

    struct sockaddr_un tmp;

    /* Information about the character */
    int pos_x, pos_y;
    int n_bytes;

    /* Create the socket for the server */
    int sock_fd;
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    /* Check for it was sucessfully created */ 
    if (sock_fd == -1){ 
	    perror("socket: ");
	    exit(-1);
    }

    struct sockaddr_un local_addr;
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, SOCKET_NAME);
    unlink(SOCKET_NAME);

    int err = bind(sock_fd, (const struct sockaddr *)&local_addr, sizeof(local_addr)); //bind the address

    /* Check for it was sucessfully binded */
    if(err == -1) { 
	    perror("bind");
	    exit(-1);
    }

    /* ncurses initialization */
	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* Creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    /* Create message window */
    WINDOW * message_win = newwin(10, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(message_win, 0 , 0);
    wrefresh(message_win);

    mvwprintw(message_win, 6, 1, "------------------");
    mvwprintw(message_win, 7, 1, "Address:          ");
    mvwprintw(message_win, 8, 1, "\"/temp/sock_game\"");
    wrefresh(message_win);

    /* Information about the character */
    int pos_x, pos_y;
    int n_bytes;
    int spawn_prizes = 1, spawn_bots = 1, n_prizes = 0;

    message_t msg;

    struct sockaddr_un tmp;

    while (1)
    {

        /* Receive message from the clients */
        n_bytes = recvfrom(sock_fd, &msg, sizeof(message_t), 0, (struct sockaddr *)&tmp, &client_addr_size);
        if (n_bytes!= sizeof(message_t)){
            continue;
        }
        
        /*------PROCESS THE VARIOUS TYPES OF MESSAGES------*/

        /* CONNECT MESSAGE */
        if (msg.msg_type == connection) { 

            /* If the client is a bot */
            if ((msg.bots[0].ch != -1) && (spawn_bots)) {
                
                /* Spawn the number of bots given in player_num */
                for (i = 0; i < msg.player_num; i++) {
                    
                    find_empty (&pos_x, &pos_y, player_data, bot_data, prize_data); // Find an empty space to spawn the bot

                    /* Save the bot data */
                    bot_data[i].ch = '*';
                    bot_data[i].pos_x = pos_x;
                    bot_data[i].pos_y = pos_y;

                    /* Draw bot */
                    wmove(my_win, pos_y, pos_x);
                    waddch(my_win, '*');
                    wrefresh(my_win);

                }

                spawn_bots = 0; // We only do this process ONCE

            } 
            
            /* If the client is a prize */
            else if ((msg.prizes[0].ch != -1) && (spawn_prizes)){
                
                /* Spawn 5 prizes */
                for (i = 0; i < 5; i++) {

                    find_empty (&pos_x, &pos_y, player_data, bot_data, prize_data); // Find an empty space to spawn the prize

                    /* Save the prize data */
                    prize_data[i].ch = msg.prizes[i].ch;
                    prize_data[i].hp = msg.prizes[i].hp;

                    prize_data[i].pos_x = pos_x;
                    prize_data[i].pos_y = pos_y;

                    /* Draw prize */
                    wmove(my_win, pos_y, pos_x);
                    waddch(my_win, prize_data[i].ch);
                    wrefresh(my_win);

                    n_prizes++; // Increment the current number of prizes
                }

                spawn_prizes = 0; // We only do this process ONCE 

            } 
            
            /* If the client is a player */
            else if ((msg.player[0].ch != -1) && (current_players < 10)){

                find_empty (&pos_x, &pos_y, player_data, bot_data, prize_data); // Find an empty space to spawn the prize

                for (i = 0; i < 10; i++) {
                    if (player_data[i].ch == -1) { // Get the first avaiable space in player_data

                        /* Check if the ch choosen by the player is being used, if it is we assign a random ch to the player*/
                        player_data[i].ch = ch_checker(msg.player[0].ch, player_data);

                        /* Save the prize data */
                        player_data[i].pos_x = pos_x;
                        player_data[i].pos_y = pos_y;
                        player_data[i].hp = 10;

                        client_addr [i] = tmp;

                        current_players++; // Increase the number of current players playing the game

                        /* Draw the player */
                        wmove(my_win, pos_y, pos_x);
                        waddch(my_win, player_data[i].ch);
                        wrefresh(my_win);	

                        break;
                    }
                }

                /* Prepare to send ball_information message */
                msg.msg_type = ball_information;
                msg.player_num = i;
                
                /* Copy the current data of the game */
                for (i = 0; i < 10; i++) { 
                    msg.player[i] = player_data[i];
                    msg.bots[i] = bot_data[i];
                    msg.prizes[i] = prize_data[i];
                }

                /* BALL_INFORMATION MESSAGE */
                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &tmp, client_addr_size);

            } 

            /* Connect from a player and we already have 10 players in the game */
            else if ((msg.player[0].ch != -1) && (current_players == 10)) {

                msg.msg_type = lobby_full;

                /* LOBBY_FULL MESSAGE */
                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &tmp, client_addr_size);

            }
            
            /* Update the message window */
            show_all_health(message_win, player_data);

        } 

        /* BALL_MOVEMENT MESSAGE */
        if(msg.msg_type == ball_movement){

            /* Check if the client sending the message is in fact a player in the game and is sending from the correct addr (anti-cheat) */
            if ((player_data[msg.player_num].ch == msg.player[msg.player_num].ch) && (client_addr[msg.player_num] == tmp)) {
                
                /* Save the old position */
                pos_x = player_data[msg.player_num].pos_x;
                pos_y = player_data[msg.player_num].pos_y;

                /* Calculate the new position */
                moove_player(&player_data[msg.player_num], msg.direction);

                /* Check if the player hit another player */
                for(int j = 0 ; j < 10; j++){

                    /* Check if the position in the array has a player (different then the player moving) */
                    if((player_data[j].ch != -1) && (j != msg.player_num)) {
                        
                        /* See if the player is in the position that our current player is moving into */
                        if (player_data[j].pos_x == player_data[msg.player_num].pos_x && player_data[j].pos_y == player_data[msg.player_num].pos_y){ //Player hits another player

                            /* If the player has less(or equal to) than 9 lives increment HP */
                            if (player_data [msg.player_num].hp <= 9) {
                                player_data [msg.player_num].hp++;
                            }

                            player_data [j].hp--; // Decrease HP of the player that was hit

                            if (player_data [j].hp == 0) { // If the player that was hit has 0 lives then its GAME OVER

                                /* HEALTH_0 MESSAGE */
                                msg.msg_type = health_0;

                                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[j], client_addr_size);

                                /* Take the player out of the game */
                                wmove(my_win, player_data [j].pos_y, player_data [j].pos_x);
                                waddch(my_win,' ');

                                player_data [j].ch = -1;
                                current_players--;
                            } 

                            clear_to_move = 0; // Can't move into new position

                            break;
                        }
                    }

                    /* Check if the player hit a bot */
                    if (bot_data[j].ch != -1) {

                        /* Player hits a bot */
                        if ((bot_data[j].pos_x == player_data[msg.player_num].pos_x) && (bot_data[j].pos_y == player_data[msg.player_num].pos_y)){

                            clear_to_move = 0; // Can't move into new position
                            break;
                        } 

                    }
                    
                    /* Check if the player hit a prize */
                    if (prize_data[j].ch != -1){

                        /* Player hits a prize */
                        if ((prize_data[j].pos_x == player_data[msg.player_num].pos_x) && (prize_data[j].pos_y == player_data[msg.player_num].pos_y)){

                            /* Increase player hp by the value of the prize */
                            player_data[msg.player_num].hp += prize_data[j].hp;

                            /* The hp of the player must not reach values bigger than 10 */
                            if (player_data[msg.player_num].hp > 10) {

                                player_data[msg.player_num].hp = 10;

                            }
                            
                            /* Take the prize out of the game */
                            prize_data[j].ch = -1;
                            n_prizes--;

                            break;
                        } 

                    }
                }

                if (clear_to_move) { // Go into new position

                    /* Delete player from old position */
                    wmove(my_win, pos_y, pos_x);
                    waddch(my_win,' ');

                    /* Draw player in the new position */
                    wmove(my_win, player_data[msg.player_num].pos_y, player_data[msg.player_num].pos_x);
                    waddch(my_win, player_data[msg.player_num].ch);
                    wrefresh(my_win);	

                } else { // Keep the old coordenates
        
                    player_data[msg.player_num].pos_x = pos_x;
                    player_data[msg.player_num].pos_y = pos_y;
                }
                
                /* FIELD_STATUS MESSAGE */
                msg.msg_type = field_status;

                for (i = 0; i < 10; i++) { //Copy the current data
                    msg.player[i] = player_data[i];
                    msg.bots[i] = bot_data[i];
                    msg.prizes[i] = prize_data[i];
                }

                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[msg.player_num], client_addr_size);

                clear_to_move = 1;

            } 
            
            /* Check if the moviment message is being sent by a bot */
            else if (bot_data[msg.player_num].ch == msg.bots[msg.player_num].ch){

                /* Save the old position */
                pos_x = bot_data[msg.player_num].pos_x;
                pos_y = bot_data[msg.player_num].pos_y;

                /* Calculate the new position */
                moove_player(&bot_data[msg.player_num], msg.direction);

                /* Check if the bot hit a player */
                for(int j = 0 ; j < 10; j++){

                    /* Check if the position in the array has a player */
                    if(player_data[j].ch != -1) {
                        
                        /* See if the player is in the position that the bot is moving into */
                        if (player_data[j].pos_x == bot_data[msg.player_num].pos_x && player_data[j].pos_y == bot_data[msg.player_num].pos_y){ //Bot hits another player

                            player_data [j].hp--; // Decrease HP of the player that was hit

                            if (player_data [j].hp == 0) { // If the player that was hit has 0 lives then its GAME OVER

                                /* HEALTH_0 MESSAGE */
                                msg.msg_type = health_0;
        
                                sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr[j], client_addr_size);
                                wmove(my_win, player_data [j].pos_y, player_data [j].pos_x);
                                waddch(my_win,' ');

                                /* Take the player out of the game*/
                                player_data [j].ch = -1;
                                current_players--;
                            } 

                            clear_to_move = 0; // Can't move into new position

                            break;
                        }
                    }

                    /* Check if bot hit another bot */
                    if (bot_data[j].ch != -1 && j != msg.player_num) {

                        /* Bot hits a bot */
                        if (bot_data[j].pos_x == bot_data[msg.player_num].pos_x && bot_data[j].pos_y == bot_data[msg.player_num].pos_y){

                            clear_to_move = 0; // Can't move into new position
                            break;
                        } 

                    }
                    
                    /* Check if bot hit a prize */
                    if (prize_data[j].ch != -1){

                        /* Bot hits a prize */
                        if ((prize_data[j].pos_x == bot_data[msg.player_num].pos_x) && (prize_data[j].pos_y == bot_data[msg.player_num].pos_y)){
                            
                            clear_to_move = 0; // Can't move into new position
                            break;
                        } 

                    }
                }

                if (clear_to_move) { // Go into new position

                    /* Delete bot from old position */
                    wmove(my_win, pos_y, pos_x);
                    waddch(my_win,' ');

                    /* Draw bot in the new position */
                    wmove(my_win, bot_data[msg.player_num].pos_y, bot_data[msg.player_num].pos_x);
                    waddch(my_win, bot_data[msg.player_num].ch);
                    wrefresh(my_win);	

                } else { // Keep the old coordenates
        
                    bot_data[msg.player_num].pos_x = pos_x;
                    bot_data[msg.player_num].pos_y = pos_y;
                }
                clear_to_move = 1;
                
            }

            /* Update the message window */
            show_all_health(message_win, player_data);
        }

        /* PRIZE_SPAWN MESSAGE */
        if (msg.msg_type == prize_spawn) {

            if (n_prizes < 10) { // Check if we have less than 10 prizes in the field
                
                for (i = 0; i < 10; i++) { // Look for a free position in the data array
                    
                    if (prize_data[i].ch == -1) { 

                        find_empty (&pos_x, &pos_y, player_data, bot_data, prize_data); // Find an empty space to place the new prize

                        /* Save the prize data*/
                        prize_data[i].ch = msg.prizes[0].ch;
                        prize_data[i].hp = msg.prizes[0].hp;

                        prize_data[i].pos_x = pos_x;
                        prize_data[i].pos_y = pos_y;

                        /* Draw prize */
                        wmove(my_win, pos_y, pos_x);
                        waddch(my_win, prize_data[i].ch);
                        wrefresh(my_win);

                        n_prizes++; // Increase the amount of prizes in the game

                        break;
                    }
                }
            }
        }

        /* DISCONNECT MESSAGE */
        if (msg.msg_type == disconnect) {
            player_data [msg.player_num].ch = -1;

            /* Delete player from the screen */
            wmove(my_win, player_data[msg.player_num].pos_y, player_data[msg.player_num].pos_x);
            waddch(my_win,' ');
            wrefresh(my_win);

            current_players--; // Decrease the amount of players in the game
        }

    }

    /* End curses mode */
  	endwin();

	return 0;
}