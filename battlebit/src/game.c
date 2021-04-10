//
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <stdio.h>
#include "game.h"
#include "helper.h"

// STEP 9 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server.  Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.

static game * GAME = NULL;

void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int getOtherPlayer(int player) {
    return player^1;
}

int game_fire(game *game, int player, int x, int y) {


    // Step 5 - This is the crux of the game.  You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    //  - You will need up update the players 'shots' value
    //  - you You will need to see if the shot hits a ship in the opponents ships value.  If so, record a hit in the
    //    current players hits field
    //  - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    //  If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    //  PLAYER_1_WINS or PLAYER_2_WINS depending on who won.

    player_info *shooterInfo = &game -> players[player];
    int other_player_num = getOtherPlayer(player);
    player_info *otherInfo = &game -> players[other_player_num];

    unsigned long long int mask = xy_to_bitval(x,y);
    unsigned long long int other_ships = otherInfo->ships;

    if (mask & other_ships) {
        //hit
        otherInfo->ships = other_ships ^ mask;
        shooterInfo->shots = shooterInfo->shots | mask;
        shooterInfo->hits = shooterInfo->hits | mask;
        if(x == 7 && y == 1) {
            printf("shots:\n");
            helper_print_ull(shooterInfo->shots);
            printf("mask:\n");
            helper_print_ull(mask);
            printf("other board:\n");
            helper_print_ull(otherInfo->ships);
        }
        if (otherInfo->ships == 0) {
            if (player == 0) {
                game->status = PLAYER_0_WINS;
            } else {
                game->status = PLAYER_1_WINS;
            }
        } else {
            if (player == 0) {
                game->status = PLAYER_1_TURN;
            } else {
                game->status = PLAYER_0_TURN;
            }
        }
        return 1;
    } else {
        //miss
        shooterInfo->shots = shooterInfo->shots | mask;
        if (player == 0) {
            game->status = PLAYER_1_TURN;
        } else {
            game->status = PLAYER_0_TURN;
        }

        return 0;
    }

}

unsigned long long int xy_to_bitval(int x, int y) {
    // Step 1 - implement this function.  We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b00000...0001 (the one is in the first position)
    // x:1, y: 0 == 0b00000...10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.
    long long position = 1ull;

    if(x > 7 || x < 0 || y > 7 || y < 0){
        return 0;
    }

    position = position << (y * 8);
    position = position << x;
    return position;
}

struct game * game_get_current() {
    return GAME;
}

int check_ship_bit(struct player_info *player, int x, int y){
    unsigned long long ships = player->ships;
    //printf("\n\ncheck_bit:");
    //helper_print_ull(player->ships);
    unsigned long long board_bit = player->ships & xy_to_bitval(x, y);
    unsigned int n = ((y * 8) + x);
    unsigned long long mask = 1ull << n;
    if(board_bit & mask){
        return -1; //there is already a ship there
    }
    return 1; //no ship!

}

void set_ship_bit(player_info *player, int x, int y){
    unsigned long long ships = player->ships;
    unsigned long long mask = xy_to_bitval(x, y);
    player->ships = ships | mask;
}

int is_digit(char c){
    if(c > 57){
        return -1;
    }
    return 1;
}

int check_spec(char c, char x, char y){
    if((is_digit(x) == 1) && (is_digit(y) == 1)){
        if(c == 'c' || c == 'C' || c == 'b' || c == 'B' ||  c == 'd' || c == 'D' || c == 's' || c == 'S' || c == 'p' || c == 'P'){
            return 1;
        }else{
            return -1;
        }
    }else {
        return -1;
    }
}

int add_ship_horizontal(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    //printf("\n\nadd_ship_h:\n");
    //helper_print_ull(player->ships);
    if (length == 0){ //sucess!
        return 1;
    }else if (((x || y) < 0) || ((x || y) > 7)){ //x or y out of bounds
        return -1;
    }else if ((length + x - 1) > 7) { //ship goes off board
        return -1;
    }else if (check_ship_bit(player, x, y) == -1) { //ship already exists there
        return -1;
    }else{
        if((add_ship_horizontal(player, x+1, y, (length-1)))==1){ //recurse
            set_ship_bit(player, x, y);
            return 1;
        }else{
            return -1;
        }
    }


}

int add_ship_vertical(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    if (length == 0){ //sucess!
        return 1;
    }else if (((x || y) < 0) || ((x || y) > 7)){ //x or y out of bounds
        return -1;
    }else if ((length + y - 1) > 7) { //ship goes off board
        return -1;
    }else if (check_ship_bit(player, x, y) == -1) { //ship already exists there
        return -1;
    }else{
        if((add_ship_vertical(player, x, y+1, (length-1)))==1){ //recurse
            set_ship_bit(player, x, y);
            return 1;
        }else{
            return -1;
        }
    }
}



int game_load_board(struct game *game, int player, char * spec) {
    if(spec == NULL){
        return -1;
    }
    int c = 0;
    int b = 0;
    int d = 0;
    int s = 0;
    int pb = 0;
    int size = 0;
    while (spec[size] != '\0') {
        char curr = spec[size];
        int x, y = 0;
        if(check_spec(curr, spec[size+1], spec[size+2]) == -1){
            return -1;
        }
        x = spec[++size] - '0';
        y = spec[++size] - '0';

        if (curr == 'c' && c == 0){
            if(add_ship_vertical(&game->players[player], x, y, 5) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            c += 1;
        }else if(curr == 'C' && c == 0) {
            if(add_ship_horizontal(&game->players[player], x, y, 5) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            c += 1;
        } else if (curr == 'd' && d == 0){
            if(add_ship_vertical(&game->players[player], x, y, 3) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            d += 1;
        } else if(curr == 'D' && d == 0) {
            if(add_ship_horizontal(&game->players[player], x, y, 3) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            d += 1;
        } else if (curr == 'b' && b == 0){
            if(add_ship_vertical(&game->players[player], x, y, 4) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            b += 1;
        } else if(curr == 'B' && b == 0) {
            if(add_ship_horizontal(&game->players[player], x, y, 4) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            b += 1;
        } else if (curr == 'p' && pb == 0){
            if(add_ship_vertical(&game->players[player], x, y, 2) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            pb += 1;
        } else if(curr == 'P' && pb == 0) {
            if(add_ship_horizontal(&game->players[player], x, y, 2) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            pb += 1;
        } else if (curr == 's' && s == 0){
            if(add_ship_vertical(&game->players[player], x, y, 3) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            s += 1;
        } else if(curr == 'S' && s == 0) {
            if(add_ship_horizontal(&game->players[player], x, y, 3) == -1){
                return -1;
            }
            //helper_print_ull(game->players[player].ships);
            s += 1;
        } else {
            return -1;
        }

        size++;

    }

    if(size == (sizeof(char) * 15) && (c & b & d & s & pb & 1)){
        if ((game->players[player].ships != 0) && (game->players[getOtherPlayer(player)].ships != 0)) {
            game->status = PLAYER_0_TURN;
        }

        return 1;
    }else{
        return -1;
    }
}
