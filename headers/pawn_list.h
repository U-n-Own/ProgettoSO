/** 
 * @file:   pawn_list.h
 */

#ifndef PAWN_LIST_H
#define PAWN_LIST_H

void pawn_list_create(unsigned int pawns_num);
void pawn_list_free();
char* pawn_list_get_addr();

extern pawn_t* pawns;           /* array containg all the pawns data */

#endif /* PAWN_LIST_H */

