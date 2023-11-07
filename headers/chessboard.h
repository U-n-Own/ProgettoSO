/* 
 * @file   chessboard.h
 * @brief   common macros and definitions for the chessboard
 */

#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include "common.h"

typedef struct cell {
    char busy;
    piece_type_t type;
    union {
        pawn_t *pawn;
        flag_t *flag;
    } data;
} cell_t;

void chessboard_create(int basis, int height);
void chessboard_destroy();
void chessboard_reset();
void chessboard_print();
char* chessboard_shm_addr();
void set_chessboard_pawn(unsigned int row, unsigned int col, unsigned int player_no, unsigned int pawn_no);
void set_chessboard_flags(unsigned int row, unsigned int col, int index);
int move_pawn(unsigned int pawn_no, unsigned int player_no, int col_offset, int row_offset);



int lock_cell(int row, int col, int blocking);
void unlock_cell(int row, int col);

#endif /* CHESSBOARD_H */

