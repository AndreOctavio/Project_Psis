#include "../game.h"

/* moove_player() : 
* Function that generates a random 
* direction; 
*/
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
void moove_player (int *pos_x, int *pos_y, int direction, WINDOW * msg_win){

    if (direction == KEY_UP){
        if (*pos_y != 1){
            *pos_y = *pos_y - 1;
        }
    }

    if (direction == KEY_DOWN){
        if (*pos_y != WINDOW_SIZE-2){
            *pos_y = *pos_y + 1;
        }
    }

    if (direction == KEY_LEFT){
        if (*pos_x != 1){
            *pos_x = *pos_x - 1;
        }
    }

    if (direction == KEY_RIGHT){
        if (*pos_x != WINDOW_SIZE-2){
            *pos_x = *pos_x + 1;
        }
    }
}

/* find_empty() :
* Function that generates a random coordenate 
* that is not occupied;
*/
int find_empty (int * x, int * y, int n_players, int free [][WINDOW_SIZE - 2]) {

    if (n_players < 30) {

        int found = 0;

        while (!found) {
            found = 1;


            /* Use current time as seed for random generator */
            srand(clock());

            *x = (rand() % 18) + 1;
            *y = (rand() % 18) + 1;

            if (free [*y - 1][*x - 1] == 1) {
                free [*y - 1][*x - 1] = 0;
                return 1;
            }
        }
    } 
    
    else {

        for (int i = 0; i < WINDOW_SIZE - 2; i++) {
            for (int j = 0; j < WINDOW_SIZE - 2; j++) {
                if (free [i][j] == 1) {
                    free [i][j] = 0;
                    *x = j + 1;
                    *y = i + 1;

                    return 1;
                }
            }
        }
    }

    return 0;
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


/* show_all_health()
 * Function prints in the message window the health
 * of all connected players
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

/* send_all_field_status() :
 * Function that sends the status of the field
 * to all connected players;
 */
void send_all_field_status(msg_field_update * msg, int socket[MAX_PLAYERS], player_info_t players[MAX_PLAYERS], int n_bots){

    int n_bytes;

    /* Loop to send msg to all players */
    for (int i = 0; i < (MAX_PLAYERS) - n_bots; i++){
        if (players[i].ch == -1){
            continue;
        }

        n_bytes = send(socket[i], msg, sizeof(msg_field_update), 0);
        if (n_bytes!= sizeof(msg_field_update)){
            perror("Error sending..");
        }
    }

}

void * countdown_loop(void * arg) {

    countdown_thread_t * args = (countdown_thread_t *) arg;

    msg_field_update countdown_msg;

    sleep(10);
    
    player_info_t tmp_player = args->players[args->self];

    /* Take the player out of the game */
    args->players[args->self].ch = -1;
    args->n_players--;
    *args->free_space = 1;

    close(args->all_sockets[args->self]);

    wmove(args->my_win, args->players[args->self].pos_y, args->players[args->self].pos_x);
    waddch(args->my_win, ' ');
    wrefresh(args->my_win);

    pthread_cancel(args->server_thread);

    /* send field_status update to everyone */
    countdown_msg.old_status = tmp_player;
    countdown_msg.new_status.ch = -1;
    countdown_msg.msg_type = field_status;
    countdown_msg.arr_position = args->self;
    send_all_field_status(&countdown_msg, args->all_sockets, args->players, args->n_bots);

    return 0;
}

/* bot_loop() : 
* Function for a thread to work on.
* It takes care of everything that has to
* do with the bots in the game; 
*/
void * bot_loop(void * arg){

    server_args_t * bot = (server_args_t *) arg;

    int pos_x, pos_y, n_bytes, i, clear_to_move = 1;

    msg_field_update msg_update;

    msg_field_update msg;

    countdown_thread_t args_countdown;

    /* Spawn the number of bots given in player_num */
    for (i = 0; i < bot->n_bots; i++) {
        
        pthread_mutex_lock(&bot->lock);

        /* Find an empty space for a bot to spawn */
        find_empty (&pos_x, &pos_y, 0, bot->free_space); // Find an empty space to spawn the bot

        /* Save the bot data */
        bot->bot_data[i].ch = '*';
        bot->bot_data[i].pos_x = pos_x;
        bot->bot_data[i].pos_y = pos_y;

        /* Draw bot */
        wmove(bot->my_win, pos_y, pos_x);
        waddch(bot->my_win, '*');
        wrefresh(bot->my_win);

        /* Send the change to all the clients */
        msg.old_status.ch = -1;
        msg.new_status = bot->bot_data[i];
        msg.msg_type = field_status;
        msg.arr_position = -1;
        send_all_field_status(&msg, bot->con_socket, bot->player_data, bot->n_bots);

        pthread_mutex_unlock(&bot->lock);

    }

    /* Loop that makes the bots move */
    while(1) {

        sleep(3);

        pthread_mutex_lock(&bot->lock);


        for (i = 0; i < bot->n_bots; i++) {

            msg.old_status = bot->bot_data[i];

            /* Save the old position */
            pos_x = bot->bot_data[i].pos_x;
            pos_y = bot->bot_data[i].pos_y;

            /* Calculate the new position */
            moove_player(&pos_x, &pos_y, generate_direction(), bot->message_win);

            /* Check if the bot hit something */
            for(int j = 0 ; j < (MAX_PLAYERS) - bot->n_bots; j++){

                /* Check if the position in the array has a player */
                if((bot->player_data[j].ch != -1) && (j != i)) {
                    
                    /* See if the player is in the position that the bot is moving into */
                    if (bot->player_data[j].pos_x == pos_x && bot->player_data[j].pos_y == pos_y){ //Bot hits another player

                        if (bot->player_data[j].hp == 0) { // If the player has health 0 it means the server is waiting for a
                                                           // signal for him to respawn. The bot can't move into this position.
                                clear_to_move = 0;
                                break;
                        }

                        bot->player_data[j].hp--; // Decrease HP of the player that was hit

                            /* send field_status update to everyone */
                            msg_update.old_status.ch = -1;
                            msg_update.new_status = bot->player_data[j];
                            msg_update.msg_type = field_status;
                            msg_update.arr_position = j;
                            send_all_field_status(&msg_update, bot->con_socket, bot->player_data, bot->n_bots);

                        if (bot->player_data[j].hp == 0) { // If the player that was hit now has 0 lives then its countdown time

                            /* HEALTH_0 MESSAGE */
                            msg_update.msg_type = health_0;

                            n_bytes = send(bot->con_socket[j], &msg_update, sizeof(msg_update), 0);
                            if (n_bytes!= sizeof(msg_field_update)){
                                perror("Error sending 2..");
                                exit(-1);
                            }

                            args_countdown.self = j;
                            args_countdown.server_thread = bot->thread_id[j];

                            args_countdown.players = bot->player_data;
                            args_countdown.free_space = &bot->free_space[bot->player_data[j].pos_y - 1][bot->player_data[j].pos_x - 1];

                            args_countdown.n_players = &bot->n_players;
                            args_countdown.my_win = bot->my_win;

                            args_countdown.n_bots = bot->n_bots;
                            args_countdown.all_sockets = bot->con_socket;

                            pthread_create(&bot->thread_countdown_id[j], NULL, &countdown_loop, (void *) &args_countdown);
                        } 

                        clear_to_move = 0; // Can't move into new position

                        break;
                    }
                }

                /* Check if bot hit another bot */
                if ((bot->bot_data[j].ch != -1) && (j != i) && (j < 10)) {

                    /* Bot hits a bot */
                    if (bot->bot_data[j].pos_x == pos_x && bot->bot_data[j].pos_y == pos_y){

                        clear_to_move = 0; // Can't move into new position
                        break;
                    } 

                }
                
                /* Check if bot hit a prize */
                if ((bot->prize_data[j].ch != -1) && (j < 10)){

                    /* Bot hits a prize */
                    if ((bot->prize_data[j].pos_x == pos_x) && (bot->prize_data[j].pos_y == pos_y)){
                        
                        clear_to_move = 0; // Can't move into new position
                        break;
                    } 

                }
            }

            /* Move into new position */
            if (clear_to_move) {

                /* Delete bot from old position */
                wmove(bot->my_win, bot->bot_data[i].pos_y, bot->bot_data[i].pos_x);
                waddch(bot->my_win,' ');

                /* Update the bot_data */
                bot->free_space[bot->bot_data[i].pos_y - 1][bot->bot_data[i].pos_x - 1] = 1;
                bot->free_space[pos_y - 1][pos_x - 1] = 0;
                bot->bot_data[i].pos_x = pos_x;
                bot->bot_data[i].pos_y = pos_y;

                /* Draw bot in the new position */
                wmove(bot->my_win, pos_y, pos_x);
                waddch(bot->my_win, '*');
                wrefresh(bot->my_win);	

                /* Send the change to all the players in the game */
                msg.new_status = bot->bot_data[i];
                msg.msg_type = field_status;
                msg.arr_position = -1;

            }
            send_all_field_status(&msg, bot->con_socket, bot->player_data, bot->n_bots);
            clear_to_move = 1;
        }

        /* Update the message window */
        show_all_health(bot->message_win, bot->player_data);

        pthread_mutex_unlock(&bot->lock);
    }
}

/* prize_loop() : 
* Function for a thread to work on.
* It takes care of everything that has to
* do with the prizes in the game; 
*/
void * prize_loop(void * arg){
    
        server_args_t * prize = (server_args_t *) arg;
        msg_field_update msg;

        /* podemos meter tudo num array de prizes só
            tipo player_info_t prize_data[10]
            dps só enviar o field status */
    
        int pos_x, pos_y, i;
    
        /* Spawn the number of prizes given in player_num */
        for (i = 0; i < 5; i++) {

            srand(clock());
            
            pthread_mutex_lock(&prize->lock);

            /* Find an empty space for a player to spawn */
            find_empty(&pos_x, &pos_y, 0, prize->free_space); // Find an empty space to spawn the prize
    
            /* Save the prize data */
            prize->prize_data[i].hp = (rand() % 5) + 1;
            prize->prize_data[i].pos_x = pos_x;
            prize->prize_data[i].pos_y = pos_y;
            prize->prize_data[i].ch = prize->prize_data[i].hp + 48;
            prize->n_prizes++;
    
            /* Draw prize */
            wmove(prize->my_win, pos_y, pos_x);
            waddch(prize->my_win, prize->prize_data[i].ch);
            wrefresh(prize->my_win);

            /* Send the change to all the players in the game */
            msg.old_status.ch = -1;
            msg.new_status = prize->prize_data[i];
            msg.msg_type = field_status;
            msg.arr_position = -1;
            send_all_field_status(&msg, prize->con_socket, prize->player_data, prize->n_bots);

            pthread_mutex_unlock(&prize->lock);
    
        }

        /* Prize spawning loop */
        while(1) {
    
            sleep(5);

            srand(clock());
    
            pthread_mutex_lock(&prize->lock);
    
            for (i = 0; i < 10; i++) {
                
                if (prize->prize_data[i].ch == -1) {

                    /* Find an empty space to spawn the prize */
                    find_empty (&pos_x, &pos_y, prize->n_players, prize->free_space);
    
                    /* Save the prize data */
                    prize->prize_data[i].hp = (rand() % 5) + 1;
                    prize->prize_data[i].pos_x = pos_x;
                    prize->prize_data[i].pos_y = pos_y;
                    prize->prize_data[i].ch = prize->prize_data[i].hp + 48;
                    prize->n_prizes++;
    
                    /* Draw prize */
                    wmove(prize->my_win, pos_y, pos_x);
                    waddch(prize->my_win, prize->prize_data[i].ch);
                    wrefresh(prize->my_win);

                    /* Send the change to all the players in the game */
                    msg.old_status.ch = -1;
                    msg.new_status = prize->prize_data[i];
                    msg.msg_type = field_status;
                    msg.arr_position = -1;
                    send_all_field_status(&msg, prize->con_socket, prize->player_data, prize->n_bots);
                    
                    break;
                }
            }

            pthread_mutex_unlock(&prize->lock);
    
        }
    
        return NULL;
}

/* player_loop() : 
* Function for a thread to work on.
* It takes care of everything that has to
* do with the clients in the game; 
*/
void * player_loop(void * arg){

    server_args_t * player = (server_args_t *) arg;

    int self = player->tmp_self;
    player->thread_id[self] = pthread_self();

    int pos_x, pos_y, n_bytes, clear_to_move = 1;

    message_t msg;
    msg_field_update msg_update;

    countdown_thread_t args_countdown;

    while (1) {

        /* Receive message from the clients */
        n_bytes = recv(player->con_socket[self], &msg, sizeof(message_t), 0);

        /* Socket closed or error */
        if(n_bytes != sizeof(message_t)){

            if(n_bytes == 0) { // Client closed the socket
                
                pthread_mutex_lock(&player->lock);

                /* Take the player out of the game */
                player->player_data[self].ch = -1;
                player->n_players--;
                player->free_space[player->player_data[self].pos_y - 1][player->player_data[self].pos_x - 1] = 1;

                close(player->con_socket[self]);

                wmove(player->my_win, player->player_data[self].pos_y, player->player_data[self].pos_x);
                waddch(player->my_win, ' ');
                wrefresh(player->my_win);

                /* Update the message window */
                show_all_health(player->message_win, player->player_data);

                pthread_mutex_unlock(&player->lock);

                return 0;
            }
            continue;
        }

        /* CONNECT MESSAGE */
        if (msg.msg_type == connection) {

            pthread_mutex_lock(&player->lock);

            /* Find an empty space to spawn a player */
            find_empty (&pos_x, &pos_y, player->n_players, player->free_space); // Find an empty space to spawn the prize

            /* Check if the ch choosen by the player is being used, if it is we assign a random ch to the player*/
            player->player_data[self].ch = ch_checker(msg.player[0].ch, player->player_data);

            /* Save the player data */
            player->player_data[self].pos_x = pos_x;
            player->player_data[self].pos_y = pos_y;
            player->player_data[self].hp = 10;

            /* Draw the player */
            wmove(player->my_win, pos_y, pos_x);
            waddch(player->my_win, player->player_data[self].ch);
            wrefresh(player->my_win);	

            show_all_health(player->message_win, player->player_data);

            /* Prepare to send ball_information message */
            msg.msg_type = ball_information;
            msg.player_num = self;
            
            /* Copy the current data of the game */
            memcpy(msg.player, player->player_data, sizeof(player_info_t) * MAX_PLAYERS);
            memcpy(msg.bots, player->bot_data, sizeof(player_info_t) * 10);
            memcpy(msg.prizes, player->prize_data, sizeof(player_info_t) * 10);

            /* BALL_INFORMATION MESSAGE */
            n_bytes = send(player->con_socket[self], &msg, sizeof(msg), 0);

            if (n_bytes!= sizeof(message_t)){
                perror("Error sending 3..");
                exit(-1);
            }

            /* Send the change to all the players in the game */
            msg_update.old_status.ch = -1;
            msg_update.new_status = player->player_data[self];
            msg_update.msg_type = field_status;
            msg_update.arr_position = self;
            send_all_field_status(&msg_update, player->con_socket, player->player_data, player->n_bots);

            pthread_mutex_unlock(&player->lock);
        }

        /* BALL_MOVEMENT MESSAGE */
        else if(msg.msg_type == ball_movement){
        
            /* Check if the client sending the message is in fact
            a player in the game (anti-cheat) */
            if (player->player_data[msg.player_num].ch == msg.player[msg.player_num].ch) { 

                pthread_mutex_lock(&player->lock);

                /* Save the current position of the player */
                msg_update.old_status = player->player_data[self];

                pos_x = player->player_data[self].pos_x;
                pos_y = player->player_data[self].pos_y;

                /* Calculate the new position */
                moove_player(&pos_x,&pos_y, msg.direction, player->message_win);

                /* Check if the player hit another player */
                for(int j = 0 ; j < (MAX_PLAYERS) - player->n_bots; j++){

                    /* Check if the position in the array has a player (different then the player moving) */
                    if((player->player_data[j].ch != -1) && (j != self)) {
                        
                        /* See if the player is in the position that our current player is moving into */
                        if (player->player_data[j].pos_x == pos_x && player->player_data[j].pos_y == pos_y){ //Player hits another player

                            /*If the player that was hit has 0 lives then its GAME OVER */
                            if (player->player_data[j].hp == 0) {
    
                                clear_to_move = 0;
                                break;
                            }

                            /* If the player has less(or equal to) than 9 lives increment HP */
                            if (player->player_data [self].hp <= 9) {
                                player->player_data [self].hp++;
                            }

                            player->player_data [j].hp--; // Decrease HP of the player that was hit

                            /* send field_status update to everyone */
                            msg_update.old_status.ch = -1;
                            msg_update.new_status = player->player_data[j];
                            msg_update.msg_type = field_status;
                            msg_update.arr_position = j;
                            send_all_field_status(&msg_update, player->con_socket, player->player_data, player->n_bots);
                            
                            /* If the player that was hit has 0 lives then its countdown time */
                            if (player->player_data[j].hp == 0) {

                                /* HEALTH_0 MESSAGE */
                                msg_update.msg_type = health_0;

                                n_bytes = send(player->con_socket[j], &msg_update, sizeof(msg_update), 0);
                                if (n_bytes!= sizeof(msg_field_update)){
                                    perror("Error sending 4..");
                                    exit(-1);
                                }

                                args_countdown.self = j;
                                args_countdown.server_thread = player->thread_id[j];

                                args_countdown.players = player->player_data;
                                args_countdown.free_space = &player->free_space[player->player_data[j].pos_y - 1][player->player_data[j].pos_x - 1];

                                args_countdown.n_players = &player->n_players;
                                args_countdown.my_win = player->my_win;

                                args_countdown.n_bots = player->n_bots;
                                args_countdown.all_sockets = player->con_socket;

                                pthread_create(&player->thread_countdown_id[j], NULL, &countdown_loop, (void *) &args_countdown);
                            } 

                            clear_to_move = 0; // Can't move into new position

                            break;
                        }
                    }

                    /* Check if the player hit a bot */
                    if ((player->bot_data[j].ch != -1) && (j < 10)) {

                        /* Player hits a bot */
                        if ((player->bot_data[j].pos_x == pos_x) && (player->bot_data[j].pos_y == pos_y)){

                            clear_to_move = 0; // Can't move into new position
                            break;
                        } 

                    }
                    
                    /* Check if the player hit a prize */
                    if ((player->prize_data[j].ch != -1) && (j < 10)){

                        /* Player hits a prize */
                        if ((player->prize_data[j].pos_x == pos_x) && (player->prize_data[j].pos_y == pos_y)){

                            /* Increase player hp by the value of the prize */
                            player->player_data[self].hp += player->prize_data[j].hp;

                            /* The hp of the player must not reach values bigger than 10 */
                            if (player->player_data[self].hp > 10) {

                                player->player_data[self].hp = 10;

                            }
                            
                            /* Take the prize out of the game */
                            player->prize_data[j].ch = -1;
                            player->n_prizes--;

                            break;
                        } 

                    }
                }
                /* Move into new position */
                if (clear_to_move) {

                    /* Delete player from old position */
                    wmove(player->my_win, player->player_data[self].pos_y, player->player_data[self].pos_x);
                    waddch(player->my_win,' ');

                    player->free_space[player->player_data[self].pos_y - 1][player->player_data[self].pos_x - 1] = 1;
                    player->free_space[pos_y - 1][pos_x - 1] = 0;
                    player->player_data[self].pos_x = pos_x;
                    player->player_data[self].pos_y = pos_y;

                    /* Draw player in the new position */
                    wmove(player->my_win, pos_y, pos_x);
                    waddch(player->my_win, player->player_data[self].ch);
                    wrefresh(player->my_win);	

                }
                
                /* FIELD_STATUS MESSAGE */
                msg_update.new_status = player->player_data[self];
                msg_update.msg_type = field_status;
                msg_update.arr_position = self;
                send_all_field_status(&msg_update, player->con_socket, player->player_data, player->n_bots);

                clear_to_move = 1;

            } 

            /* Update the message window */
            show_all_health(player->message_win, player->player_data);
            

            pthread_mutex_unlock(&player->lock);
        } 

        /* Reconnect msg handling */
        else if (msg.msg_type == reconnect) {

            pthread_cancel(player->thread_countdown_id[self]);

            pthread_mutex_lock(&player->lock);

            /* Reset the health of the player (Respawn)*/
            player->player_data[self].hp = 10;

            /* Send the change to all the players in the game */
            msg_update.old_status.ch = -1;
            msg_update.new_status = player->player_data[self];
            msg_update.msg_type = field_status;
            msg_update.arr_position = self;
            send_all_field_status(&msg_update, player->con_socket, player->player_data, player->n_bots);

            pthread_mutex_unlock(&player->lock);

        }
    }

}

/* main() :
 * Initializes Data Stream socket to accept
 * connections from clients and launches the threads
 */
int main(int argc, char *argv[])
{	
    int i, port, n_bytes;
    message_t msg;

    server_args_t serv_arg;

    port = atoi(argv[1]);

    if (port < 1023 || port > 65353) {
        perror("Port error..try a port between 1023 and 65353. \n");
	    exit(-1);
    }

    serv_arg.n_bots = atoi(argv[2]);

    if (serv_arg.n_bots < 1 || serv_arg.n_bots > 10) {
        perror("Bot number is invalid.. try a number between 1 and 10. \n");
	    exit(-1);
    }

    int players_max = (MAX_PLAYERS) - serv_arg.n_bots;
    int N_THREADS = players_max + 2; // The two extra threads are for prizes and bots
    
    serv_arg.n_prizes = 0;
    serv_arg.n_players = 0; 

    /* Create the socket for the server and check errors */
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1){ 
	    perror("socket error..");
	    exit(-1);
    }

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int new_con;

    /* Bind the adress and check for errors */
    int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if(err == -1) { 
	    perror("bind error..");
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
    
    /* Creates a window and draws a border */
    serv_arg.my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(serv_arg.my_win, 0 , 0);	
	wrefresh(serv_arg.my_win);

    /* Create message window */
    serv_arg.message_win = newwin(10, WINDOW_SIZE, WINDOW_SIZE, 0);
    box(serv_arg.message_win, 0 , 0);
    wrefresh(serv_arg.message_win);

    mvwprintw(serv_arg.message_win, 6, 1, "------------------");
    mvwprintw(serv_arg.message_win, 7, 1, "Port: %d ", port);
    wrefresh(serv_arg.message_win);

    /* Initialize the data arrays with -1 in ch */
    for (i = 0; i < (MAX_PLAYERS); i++) {
        serv_arg.player_data[i].ch = -1;
        serv_arg.free_space[i/(WINDOW_SIZE - 2)][i - i/(WINDOW_SIZE - 2) * (WINDOW_SIZE - 2)] = 1; // i/(WINDOW_SIZE - 2) gives the number of the line,
                                                                                                   // i - number of the line * (WINDOW_SIZE - 2) gives the column
        if (i < 10) {
            serv_arg.bot_data[i].ch = -1;
            serv_arg.prize_data[i].ch = -1;
        }
    }

    /* Initialize the mutex */
    if (pthread_mutex_init(&serv_arg.lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    pthread_t thread_id[N_THREADS];

    /* Launch two threads, one for the bots and one for the prizes */
    pthread_create(&thread_id[N_THREADS - 2], NULL, &bot_loop, (void *) &serv_arg);
    pthread_create(&thread_id[N_THREADS - 1], NULL, &prize_loop, (void *) &serv_arg);

    int n_players_copy, n_prizes_copy;

    /* Accept connections from clients */
    while (1)
    {   
        new_con = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);

        pthread_mutex_lock(&serv_arg.lock);

        n_players_copy = serv_arg.n_players;
        n_prizes_copy = serv_arg.n_prizes;

        pthread_mutex_unlock(&serv_arg.lock);

        /* check if accept returned errors */
        if (new_con == -1) {
            perror("Accept error..");
        } 
        /* check if field is full */
        else if (n_players_copy < players_max - n_prizes_copy) { 

            for (i = 0; i < players_max; i++) {
                if (serv_arg.player_data[i].ch == -1) { // Get the first avaiable space in player_data

                    serv_arg.tmp_self = i;
                    serv_arg.con_socket[i] = new_con;

                    break;
                }
            }



            /* Lauch thread for the new player */
            pthread_create(&thread_id[i], NULL, &player_loop, (void *) &serv_arg);

            pthread_mutex_lock(&serv_arg.lock);

            /* Increase the number of players */
            serv_arg.n_players = serv_arg.n_players + 1;

            pthread_mutex_unlock(&serv_arg.lock);
        } 
        /* send loby_full message if the field is full */
        else {

            /* LOBBY_FULL MESSAGE */
            msg.msg_type = lobby_full;

            n_bytes = send(new_con, &msg, sizeof(msg), 0);

            if (n_bytes!= sizeof(message_t)){
                perror("Error sending 6..");
                exit(-1);
            }
        }
    }

    /* Delete the mutex */
    if (pthread_mutex_destroy(&serv_arg.lock) != 0) {                                     
        perror("pthread_mutex_destroy() error");                                    
        exit(2);                                                                    
    }  

    /* End curses mode */
  	endwin();

	return 0;
}