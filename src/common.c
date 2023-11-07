/**
 * @file    common.c
 */

#include "common.h"

pid_t master_pid;     

unsigned int so_num_g;           /* number of player processes */
unsigned int so_num_p;           /* number of pawn processes */
unsigned int so_max_time;        /* max duration oa a round */
unsigned int so_base;            /* basis of the playing field */
unsigned int so_altezza;         /* height of the playing field */
unsigned int so_flag_min;        /* min number of flags for round */
unsigned int so_flag_max;        /* max number of flags for round */
unsigned int so_round_score;     /* total score for round */
unsigned int so_n_moves;         /* total moves for pawn */
unsigned int so_min_hold_nsec;   /* min number of nanoseconds of occupation of a cell by a pawn */
unsigned int flags_no;           /* randomized flags number */

unsigned int* semaphores_vector;