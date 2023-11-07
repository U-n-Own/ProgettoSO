/* 
 * File:   player.h
 */

#ifndef PLAYER_H
#define PLAYER_H

/*
 * @brief   The prototype functions for the players processes
 */
void player_handler(int player_id);
void player_create(unsigned int num);
void player_place_pawn(unsigned int player_no, unsigned int pawn_no);


#endif /* PLAYER_H */

