/** 
 * @file    player_list.h
 */

#ifndef PLAYER_LIST_H
#define PLAYER_LIST_H

void player_list_create(unsigned int players_num);
void player_list_free();

extern player_t* players;       /* array containg all the players data */

#endif /* PLAYER_LIST_H */

