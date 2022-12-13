#include "game.h"

int main()
{	
    player_info_t player_data [10]; //Array store all of the info of the current players in the game (10 max)
    int current_players = 0; //Keeps the current amount of players in the game

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

    /* Information about the character */
    int ch, pos_x, pos_y;
    int n_bytes;

    direction_t  direction;
    struct sockaddr_un client_addr;
    socklen_t client_addr_size = sizeof(struct sockaddr_un);

    message_t msg;

    while (1)
    {
        //Receive message from the clients
        n_bytes = recvfrom(sock_fd, &msg, sizeof(message_t), 0, (const struct sockaddr *)&client_addr, &client_addr_size);
        
        //Check if received correct structure
        if (n_bytes!= sizeof(message_t)){
            continue;
        }
        
        /*Process the various types of messages*/

        //Connect message
        if ((msg.msg_type == connect) && (current_players < 10)) { //Maximum of 10 players
            current_players++;

            player_data [current_players - 1].ch = msg.ch;
            player_data [current_players - 1].pos_x = WINDOW_SIZE/2;
            player_data [current_players - 1].pos_y = WINDOW_SIZE/2;
            player_data [current_players - 1].HP = 10;

            msg.msg_type = ball_information;
            msg.player = player_data [current_players - 1];
            
            //Ball_information message
            sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr, client_addr_size);

        } else if (current_players == 10){ //Already 10 players in the game

            msg.msg_type = lobby_full;
            sendto(sock_fd, &msg, sizeof(msg), 0, (const struct sockaddr *) &client_addr, client_addr_size);
            continue;

        }

        //Movement message
        if(msg.msg_type == 1){
            //STEP 4
            for (i = 0; i < 100; i++) {
                if (char_data[i].ch == msg.ch){
                    break;
                }
            }

            if (i != 100) {
                ch = char_data[i].ch;
                pos_x =  char_data[i].pos_x;
                pos_y =  char_data[i].pos_y;

                /*deletes old place */
                wmove(my_win, pos_x, pos_y);
                waddch(my_win,' ');

                /* claculates new direction */
                direction = msg.direction;

                /* claculates new mark position */
                new_position(&pos_x, &pos_y, direction);

                char_data[i].pos_x = pos_x;
                char_data[i].pos_y = pos_y;

            }

            int same = -1;  

            for(int i = 0 ; i< n_chars; i++){
                if(char_data[i].pos_x == pos_x && char_data[i].pos_x){
                    same ++;
                }
            }

            msg.msg_type = 2;  

            if (same > 0){
                msg.msg_type = 3;              
            } 

            sendto(sock_fd, &msg, sizeof(msg), 0, 
                    (const struct sockaddr *) &client_addr, client_addr_size);

        }
        
        /* draw mark on new position */
        wmove(my_win, pos_x, pos_y);
        waddch(my_win,ch| A_BOLD);
        wrefresh(my_win);			
    }
  	endwin();			/* End curses mode		  */

	return 0;
}