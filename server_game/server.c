#include "../game.h"

int generate_direction(){
    int direction = rand() % 4;
    switch (direction){
        case 0:
            direction = KEY_UP;
            break;
        case 1:
            direction = KEY_DOWN;
            break;
        case 2:
            direction = KEY_LEFT;
            break;
        case 3:
            direction = KEY_RIGHT;
            break;
    }
    return direction;
}

/* moove_player() : 
* Function that updates the coordenates of 
* a player with a given direction; 
*/
void moove_player (int *pos_x, int *pos_y, int direction){

    if (direction == KEY_UP){
        if (pos_y  != 1){
            pos_y --;
        }
    }

    if (direction == KEY_DOWN){
        if (pos_y  != WINDOW_SIZE-2){
            pos_y ++;
        }
    }

    if (direction == KEY_LEFT){
        if (pos_x  != 1){
            pos_x --;
        }
    }

    if (direction == KEY_RIGHT)
        if (pos_x  != WINDOW_SIZE-2){
            pos_x ++;
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

void * bot_loop(void * arg){

    server_args_t * bot = (server_args_t *) arg;

    int pos_x, pos_y, i, clear_to_move = 1;

    message_t msg;

    /* Spawn the number of bots given in player_num */
    for (i = 0; i < bot->n_bots; i++) {
        
        pthread_mutex_lock(&bot->lock);

        find_empty (&pos_x, &pos_y, bot->player_data, bot->bot_data, bot->prize_data); // Find an empty space to spawn the bot

        /* Save the bot data */
        bot->bot_data[i].ch = '*';
        bot->bot_data[i].pos_x = pos_x;
        bot->bot_data[i].pos_y = pos_y;

        /* Draw bot */
        wmove(bot->my_win, pos_y, pos_x);
        waddch(bot->my_win, '*');
        wrefresh(bot->my_win);

        pthread_mutex_unlock(&bot->lock);

    }

    while(1) {

        sleep(3);

        pthread_mutex_lock(&bot->lock);

        for (i = 0; i < bot->n_bots; i++) {

            /* Save the old position */
            pos_x = bot->bot_data[i].pos_x;
            pos_y = bot->bot_data[i].pos_y;

            /* Calculate the new position */
            moove_player(&pos_x, &pos_y, generate_direction());

            /* Check if the bot hit something */
            for(int j = 0 ; j < 10; j++){

                /* Check if the position in the array has a player */
                if((bot->player_data[j].ch != -1) && (j != i)) {
                    
                    /* See if the player is in the position that the bot is moving into */
                    if (bot->player_data[j].pos_x == pos_x && bot->player_data[j].pos_y == pos_y){ //Bot hits another player

                        bot->player_data[j].hp--; // Decrease HP of the player that was hit

                        if (bot->player_data[j].hp == 0) { // If the player that was hit has 0 lives then its GAME OVER

                            /* HEALTH_0 MESSAGE */
                            msg.msg_type = health_0;

                            if (send(bot->con_socket[j], &msg, sizeof(msg), 0) < 0) {
                                printf("Can't send\n");
                                return -1;
                            }
                            
                            wmove(bot->my_win, bot->player_data[j].pos_y, bot->player_data[j].pos_x);
                            waddch(bot->my_win,' ');

                            /* Take the player out of the game*/
                            bot->player_data[j].ch = -1;
                            bot->n_players--;
                        } 

                        clear_to_move = 0; // Can't move into new position

                        break;
                    }
                }

                /* Check if bot hit another bot */
                if ((bot->bot_data[j].ch != -1) && (j != i)) {

                    /* Bot hits a bot */
                    if (bot->bot_data[j].pos_x == pos_x && bot->bot_data[j].pos_y == pos_y){

                        clear_to_move = 0; // Can't move into new position
                        break;
                    } 

                }
                
                /* Check if bot hit a prize */
                if (bot->prize_data[j].ch != -1){

                    /* Bot hits a prize */
                    if ((bot->prize_data[j].pos_x == pos_x) && (bot->prize_data[j].pos_y == pos_y)){
                        
                        clear_to_move = 0; // Can't move into new position
                        break;
                    } 

                }
            }

            if (clear_to_move) { // Go into new position

                /* Delete bot from old position */
                wmove(bot->my_win, bot->bot_data[i].pos_y, bot->bot_data[i].pos_x);
                waddch(bot->my_win,' ');

                /* Update the bot_data */
                bot->bot_data[i].pos_x = pos_x;
                bot->bot_data[i].pos_y = pos_y;

                /* Draw bot in the new position */
                wmove(bot->my_win, pos_y, pos_x);
                waddch(bot->my_win, '*');
                wrefresh(bot->my_win);	

            }

            clear_to_move = 1;
        }

        /* Update the message window */
        show_all_health(bot->message_win, bot->player_data);

        pthread_mutex_unlock(&bot->lock);
    }
}

void * prize_loop(void * arg){
    
        server_args_t * prize = (server_args_t *) arg;
    
        int pos_x, pos_y, i;
    
        /* Spawn the number of prizes given in player_num */
        for (i = 0; i < 5; i++) {
            
            pthread_mutex_lock(&prize->lock);
    
            find_empty(&pos_x, &pos_y, prize->player_data, prize->bot_data, prize->prize_data); // Find an empty space to spawn the prize
    
            /* Save the prize data */
            prize->prize_data[i].hp = (rand() % 5) + 1;
            prize->prize_data[i].pos_x = pos_x;
            prize->prize_data[i].pos_y = pos_y;
            prize->prize_data[i].ch = prize->prize_data[0].hp + 48;
            prize->n_prizes++;
    
            /* Draw prize */
            wmove(prize->my_win, pos_y, pos_x);
            waddch(prize->my_win, prize->prize_data[i].ch);
            wrefresh(prize->my_win);
    
            pthread_mutex_unlock(&prize->lock);
    
        }
    
        while(1) {
    
            sleep(5);
    
            pthread_mutex_lock(&prize->lock);
    
            for (i = 0; i < prize->n_prizes; i++) {
    
                
                if (prize->prize_data[i].ch == -1) {

                    /* Find a new empty space to spawn the prize */
                    find_empty (&pos_x, &pos_y, prize->player_data, prize->bot_data, prize->prize_data);
    
                    /* Save the prize data */
                    prize->prize_data[i].hp = (rand() % 5) + 1;
                    prize->prize_data[i].pos_x = pos_x;
                    prize->prize_data[i].pos_y = pos_y;
                    prize->prize_data[i].ch = prize->prize_data[0].hp + 48;
                    prize->n_prizes++;
    
                    /* Draw prize */
                    wmove(prize->my_win, pos_y, pos_x);
                    waddch(prize->my_win, prize->prize_data[i].ch);
                    wrefresh(prize->my_win);
                }
            }
            pthread_mutex_unlock(&prize->lock);
    
        }
    
        return NULL;
}

void * player_loop(void * arg){

    server_args_t * player = (server_args_t *) arg;

    int self = player->tmp_self;
    int pos_x, pos_y, n_bytes;

    message_t msg;

    while (1) {

        /* Receive message from the clients */
        n_bytes = recv(player->con_socket[self], &msg, sizeof(message_t), 0);

        if (n_bytes!= sizeof(message_t)){
            continue;
        }

        /* CONNECT MESSAGE */
        if (msg.msg_type == connection) {
            find_empty (&pos_x, &pos_y, player->player_data, player->bot_data, player->prize_data); // Find an empty space to spawn the prize

            /* Check if the ch choosen by the player is being used, if it is we assign a random ch to the player*/
            player->player_data[self].ch = ch_checker(msg.player[0].ch, player->player_data);

            /* Save the prize data */
            player->player_data[self].pos_x = pos_x;
            player->player_data[self].pos_y = pos_y;
            player->player_data[self].hp = 10;

            /* Draw the player */
            wmove(player->my_win, pos_y, pos_x);
            waddch(player->my_win, player->player_data[self].ch);
            wrefresh(player->my_win);	

            /* Prepare to send ball_information message */
            msg.msg_type = ball_information;
            msg.player_num = self;
            
            /* Copy the current data of the game */
            for (int i = 0; i < 10; i++) { 
                msg.player[i] = player->player_data[i];
                msg.bots[i] = player->bot_data[i];
                msg.prizes[i] =player->prize_data[i];
            }

            /* BALL_INFORMATION MESSAGE */
            send(player->con_socket[self], &msg, sizeof(msg), 0);
        }
    }
        



}


int main(int argc, char *argv[])
{	

    char remote_addr_str_saved [100];
    char remote_addr_str_tmp [100];

    int current_players = 0; // Keeps the current amount of players in the game
    int n_prizes = 0; // Keeps the number the current amount of prizes in the game
    int clear_to_move = 1; // Flag to check if the player/bot can move
    int spawn_prizes = 1, spawn_bots = 1; // Flags, only spawn the initial bots and prizes ONCE

    int i;

    message_t msg;

    struct sockaddr_in tmp;

    /* Information about the character */
    int pos_x, pos_y;
    int n_bytes;

    /* Create the socket for the server */
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Check for it was sucessfully created */ 
    if (sock_fd == -1){ 
	    perror("socket: ");
	    exit(-1);
    }

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(SOCK_PORT);
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int new_con;

    int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)); //bind the address

    /* Check for it was sucessfully binded */
    if(err == -1) { 
	    perror("bind");
	    exit(-1);
    }

    if ((listen(sock_fd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    } else {
        printf("Server listening..\n");
    }

    /* ncurses initialization */
	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    server_args_t serv_arg;
    
    /* Creates a window and draws a border */
    serv_arg.my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(serv_arg.my_win, 0 , 0);	
	wrefresh(serv_arg.my_win);

    /* Create message window */
    serv_arg.message_win = newwin(10, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(serv_arg.message_win, 0 , 0);
    wrefresh(serv_arg.message_win);

    mvwprintw(serv_arg.message_win, 6, 1, "------------------");
    mvwprintw(serv_arg.message_win, 7, 1, "Address:          ");
    mvwprintw(serv_arg.message_win, 8, 1, "\"/temp/sock_game\"");
    wrefresh(serv_arg.message_win);

    /* Initialize the data arrays with -1 in ch */
    for (i = 0; i < 10; i++) {
        serv_arg.player_data[i].ch = -1;
        serv_arg.bot_data[i].ch = -1;
        serv_arg.prize_data[i].ch = -1;
    }

    serv_arg.n_prizes = 0;
    serv_arg.n_bots = argv[1];
    serv_arg.n_players = 0; 

    if (pthread_mutex_init(&serv_arg.lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    pthread_t thread_id[N_THREADS];
    pthread_create(&thread_id[10], NULL, &bot_loop, (void *) &serv_arg);

    while (1)
    {   
        if (serv_arg.n_players < 10) {
            new_con = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);

            for (i = 0; i < 10; i++) {
                if (serv_arg.player_data[i].ch == -1) { // Get the first avaiable space in player_data

                    serv_arg.tmp_self = i;
                    serv_arg.con_socket[i] = new_con;

                    break;
                }
            }
        
            pthread_create(&thread_id[serv_arg.n_players], NULL, &player_loop, (void *) &serv_arg);

            pthread_mutex_lock(&serv_arg.lock);

            serv_arg.n_players++;

            pthread_mutex_unlock(&serv_arg.lock);
        }
        
        







        
        /*------PROCESS THE VARIOUS TYPES OF MESSAGES------*/

        /* CONNECT MESSAGE */
        if (msg.msg_type == connection) {
            
            /* If the client is a prize */
            if ((msg.prizes[0].ch != -1) && (spawn_prizes)){
                
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
            
            if (inet_ntop(AF_INET, &client_addr[msg.player_num].sin_addr, remote_addr_str_saved, 100) == NULL){
			    perror("converting saved addr: ");
		    }

            if (inet_ntop(AF_INET, &tmp.sin_addr, remote_addr_str_tmp, 100) == NULL){
			    perror("converting tmp addr: ");
		    }

            /* Check if the client sending the message is in fact
            a player in the game and is sending from the correct addr (anti-cheat) */
            if ((player_data[msg.player_num].ch == msg.player[msg.player_num].ch) && (strcmp(remote_addr_str_saved, remote_addr_str_tmp) == 0)) {  
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

            /* Update the message window */
            show_all_health(message_win, player_data);
        }

    }

    /* End curses mode */
  	endwin();

	return 0;
}