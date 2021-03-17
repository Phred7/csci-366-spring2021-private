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

int game_load_board(struct game *game, int player, char * spec) {
    // Step 2 - implement this function.  Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //

    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //
    // if it is invalid, you should return -1
    // return -1;

    if(spec == NULL){
        return -1;
    }
    int c, b, d, s, pb, size = 0;
    while (spec[size] != '\0') {
        char curr = spec[size];
        c = (((curr == 'c' || curr == 'C')) ^ c == 1 ^ c == -1) ? 1 : 0; //can use ternary to call add ship???
        b = (((curr == 'b' || curr == 'B')) ^ b == 1 ^ c == -1) ? 1 : 0;
        d = (((curr == 'd' || curr == 'D')) ^ d == 1 ^ c == -1) ? 1 : 0;
        s = (((curr == 's' || curr == 'S')) ^ s == 1 ^ c == -1) ? 1 : 0;
        pb = (((curr == 'p' || curr == 'P')) ^ pb == 1 ^ c == -1) ? 1 : 0;
        size++;
    }

    if(size == (sizeof(char) * 15) && (c & b & d & s & pb & 1)){
        return 1;
    }else{
        return -1;
    }

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

/*void set_ship_bit(player_info *player, int x, int y){
    //helper_print_ull(player->ships);
    unsigned long long ships = player->ships;
    unsigned int n = ((y * 8) + x);
    unsigned long long mask = 1ull << n;
    player->ships = ships | mask;
    //helper_print_ull(player->ships);

}*/

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
    }else if ((length + x) > 7) { //ship goes off board
        return -1;
    }else if (check_ship_bit(player, x, y) == -1) { //ship already exists there
        return -1;
    }else{
        return add_ship_horizontal(player, x+1, y, (length-1));
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
    }else if ((length + y) > 7) { //ship goes off board
        return -1;
    }else if (check_ship_bit(player, x, y) == -1) { //ship already exists there
        return -1;
    }else{
        return add_ship_horizontal(player, x, y+1, (length-1));
    }
}