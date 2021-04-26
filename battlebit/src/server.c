//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

static game_server *SERVER;
pthread_mutex_t lock;


void init_server() {
    if (SERVER == NULL) {
        SERVER = calloc(1, sizeof(struct game_server));
    } else {
        printf("Server already started");
    }
}

int handle_client_connect(int player) {
    // STEP 8 - This is the big one: you will need to re-implement the REPL code from
    // the repl.c file, but with a twist: you need to make sure that a player only
    // fires when the game is initialized and it is there turn.  They can broadcast
    // a message whenever, but they can't just shoot unless it is their turn.
    //
    // The commands will obviously not take a player argument, as with the system level
    // REPL, but they will be similar: load, fire, etc.
    //
    // You must broadcast informative messages after each shot (HIT! or MISS!) and let
    // the player print out their current board state at any time.
    //
    // This function will end up looking a lot like repl_execute_command, except you will
    // be working against network sockets rather than standard out, and you will need
    // to coordinate turns via the game::status field.
    int client_socket = SERVER->player_sockets[player];
    struct game * game_curr = game_get_current();
    int read_size;
    int buffer_size = 2000;
    int loaded = 1;
    char raw_buffer[buffer_size];
    //struct char_buff * raw_buffer = cb_create(buffer_size);
    struct char_buff * output = cb_create(buffer_size);
    char message[100] = {0};
    sprintf(message,"Welcome to battleBit server Player %d\n", player);
    send(client_socket, message, strlen(message), 0);
    cb_reset(output); //resets to ""
    cb_append(output, "battleBit (? for help) > ");
    cb_write(client_socket, output);

    while (game_curr->status != PLAYER_0_WINS && game_curr->status != PLAYER_1_WINS) {
        while ((read_size = recv(client_socket, raw_buffer, buffer_size, 0)) > 0) {
            //take raw_buffer and dump into input char bf (tokenize?)
            //mimic repl_execture_cmd()... pass through the current player (so that client cannot fire for the other player)... ignore cb_free()
            //big ugly if statements

            pthread_mutex_lock(&lock); //mutex lock
            cb_reset(output);

            struct char_buff *input = cb_create(buffer_size);
            cb_append(input, raw_buffer);
            char *command = cb_tokenize(input, " \r");

            if (command) {
                char *arg1 = cb_next_token(input);
                char *arg2 = cb_next_token(input);
                if (strcmp(command, "exit") == 0) {
                    cb_append(output, "\ngoodbye, you scrub!");
                    cb_write(client_socket, output);
                    exit(EXIT_SUCCESS);
                } else if (strcmp(command, "?") == 0) {
                    cb_append(output, "? - show help\n");
                    cb_append(output, "load <string> - load a ship layout file for the given player\n");
                    cb_append(output, "show - shows the board for the given player\n");
                    cb_append(output, "fire [0-7] [0-7] - fire at the given position\n");
                    cb_append(output, "say <string> - Send the string to all players as part of a chat\n");
                    cb_append(output, "exit - quit the server\n");
                } else if (strcmp(command, "load") == 0) {
                    if (loaded == 1) {
                        game_load_board(game_get_current(), player, arg1);
                        loaded = 0;
                        char message[100] = {0};
                        if (game_curr->players[(player^1)].ships == 0) {
                            sprintf(message, "Waiting on Player %d", (player^1));
                        } else {
                            sprintf(message, "All player boards loaded\nPlayer 0 turn");
                        }
                        cb_append(output, message);
                    } else {
                        cb_append(output, "another configuration already loaded\n");
                    }

                } else if (strcmp(command, "show") == 0) {
                    struct char_buff *boardBuffer = cb_create(buffer_size);
                    repl_print_board(game_get_current(), player, boardBuffer);
                    char message[100] = {0};
                    sprintf(message, "%s", boardBuffer->buffer);
                    cb_append(output, message);
                    cb_free(boardBuffer);
                } else if (strcmp(command, "fire") == 0) {
                    if (game_curr->status == ((player == 0) ? PLAYER_0_TURN : PLAYER_1_TURN)) {
                        int x = atoi(arg1);
                        int y = atoi(arg2);
                        if (x < 0 || x >= BOARD_DIMENSION || y < 0 || y >= BOARD_DIMENSION) {
                            char message[100] = {0};
                            sprintf("Invalid coordinate: %i %i\n", x, y);
                            cb_append(output, message);
                        } else {
                            char message[100] = {0};
                            sprintf(message, "Player %d fires at %d, %d", player, x, y);
                            cb_append(output, message);
                            printf("Player %i fired at %i %i\n", player + 1, x, y);  //+1?
                            int result = game_fire(game_get_current(), player, x, y);
                            if (result) {
                                printf("  HIT!!!");
                                cb_append(output, " - HIT");
                            } else {
                                printf("  Miss");
                                cb_append(output, " - MISS");
                            }
                            server_broadcast(output);
                            cb_reset(output);
                        }
                    } else {
                        if (game_curr->status == INITIALIZED || game_curr->status == CREATED) {
                            if (loaded == 0) {
                                char message[100] = {0};
                                int other = player ^1;
                                sprintf(message, "Waiting on Player %d\n", other);
                                cb_append(output, message);
                            } else {
                                cb_append(output, "Game has not begun!\n");
                            }
                        } else {
                            char message[100] = {0};
                            int other = player ^1;
                            sprintf(message, "Waiting on Player %d\n", other);
                            cb_append(output, message);
                        }
                    }
                } else if (strcmp(command, "say") == 0) {
                    char message[100] = {0};
                    sprintf(message, "Player %d says: ", player);
                    cb_append(output, message);
                    cb_append(output, arg1);
                    cb_append(output, arg2);
                    char * msg;
                    while((msg = cb_next_token(input)) > 0) {
                        cb_append(output, msg);
                    }
                    server_broadcast(output);
                    cb_reset(output);
                } else {
                    char message[100] = {0};
                    sprintf(message, "Unknown Command: %s\n", command);
                    cb_append(output, message);
                }
            }

            if (game_curr->status == PLAYER_0_WINS || game_curr->status == PLAYER_0_WINS) {
                cb_reset(output);
                char endMSG[100] = {0};
                sprintf(endMSG, " - Player %d Wins!\n", ((game_curr->status == PLAYER_0_WINS) ? 0 : 1));
                cb_append(output, endMSG);
                //cb_append(output, "\nclosing connection");
                server_broadcast(output);
                for (int i = 0; i < 2; i++) {
                    close(SERVER->player_sockets[i]);
                    pthread_join(SERVER->player_threads[i], NULL);
                }
                break;
            }
            cb_append(output, "\nbattleBit (? for help) > ");
            //cb_append(output, "\nclosing connection");
            cb_write(client_socket, output);
            pthread_mutex_unlock(&lock); //mutex_unlock
        }
    }


    //use mutexes: (declared outside of this func) pthread_mutex_lock and pthread_mutex_unlock around anywhere data is being manipulated
    //(so only one client can change data at a time)... just inside of this func works... dont need fine grain locking
    //protect data in game.c... 2 threads cannot be even looking at the data at the same time

}

void server_broadcast(char_buff *msg) {
    puts(msg);
    for(int i = 0; i < 2; i++) {
        int client_socket = SERVER->player_sockets[i];
        cb_write(client_socket, msg);
    }


    // send message to all players
}

int run_server() {
    // STEP 7 - implement the server code to put this on the network.
    // Here you will need to initalize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously
    int server_socket_fd = socket(AF_INET,
                                  SOCK_STREAM,
                                  IPPROTO_TCP);
    if (server_socket_fd == -1) {
        printf("Could not create socket\n");
    }

    int yes = 1;
    setsockopt(server_socket_fd,
               SOL_SOCKET,
               SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server;

    // fill out the socket information
    server.sin_family = AF_INET;
    // bind the socket on all available interfaces
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9876);

    int request = 0;
    if (bind(server_socket_fd,
            // Again with the cast
             (struct sockaddr *) &server,
             sizeof(server)) < 0) {
        puts("Bind failed");
    } else {
        puts("Bind worked!");
        listen(server_socket_fd, 3);

        //Accept an incoming connection
        puts("Waiting for incoming connections...");


        struct sockaddr_in client;
        socklen_t size_from_connect;
        int client_socket_fd;
        int request_count = 0;
        pthread_t player0, player1;
        while ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client, &size_from_connect)) > 0) {
            if((request_count == 0) || (request_count == 1)){
                SERVER->player_sockets[request_count] = client_socket_fd;
                SERVER->player_threads[request_count] = (request_count == 0) ? player0 : player1;
                /*char message[100] = {0};
                sprintf(message,"connected as player %d\n", request_count);
                send(client_socket_fd, message, strlen(message), 0);*/
                pthread_create(((request_count == 0) ? &player0 : &player1), NULL,  (void *)handle_client_connect,  request_count);


            } else {
                char message[100] = {0};
                sprintf(message,"too many connections: %d\n\n", request_count);
                send(client_socket_fd, message, strlen(message), 0);
                close(client_socket_fd);
                //break;
            }
            request_count++;
            /*if (request_count > 1) {
                break;
            }*/

        }
    }
}

int server_start() {
    // STEP 6 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL
    init_server();

    pthread_mutex_init(&lock, NULL);
    pthread_t svr;
    SERVER->server_thread = &svr;
    pthread_create(&svr, NULL, (void *)run_server, NULL);
    //pthread_create(SERVER->server_thread, NULL, (void *)run_server, NULL);

}
