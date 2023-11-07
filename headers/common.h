/**
 * @File:   common.h
 */

#ifndef COMMON_H
#define COMMON_H

#define _XOPEN_SOURCE	700

#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <stdio.h>
#define DEBUG 0
#define LOGGER_INFO(x) printf x; fflush(stdout)

#define FALSE 0
#define TRUE 1

#define SEM_ID_G1 7801
#define SEM_ID_G2 7802

#define SEM_ID_G3 7803
#define SEM_ID_G4 7804

#define SEM_ID_GAME 4536

typedef enum {EMPTY, PAWN, FLAG, P} piece_type_t;

typedef struct  {
    unsigned int row;
    unsigned int col;
} position_t;

typedef struct {
    position_t position;        /* flag position */
} goal_t;

typedef struct {
    pid_t pid;
    unsigned int player_no;     
    position_t position;        /* current position on the chessboard */
    unsigned int moves;         /* current number of moves */
    goal_t* goals;              /* flags position */
} pawn_t;

typedef struct {
    unsigned int index;
    position_t position;        /* current position on the chessboard */
    unsigned int score;         /* how flag is worth */
} flag_t;
 
typedef struct {
    pid_t pid;
    unsigned int score;
    pawn_t* pawns;              /* the pawns owned by the player */
} player_t;

typedef struct  {
    long mtype;                 /* message type, must be > 0 */
    char mtext[1];              /* message data */
    unsigned int mparameter;    /* message parameter */
} msgbuf;


extern pid_t main_pid; 
extern pid_t master_pid; 

extern unsigned int* semaphores_vector;

/* parameters*/
extern unsigned int so_num_g;           /* number of player processes */
extern unsigned int so_num_p;           /* number of pawn processes */
extern unsigned int so_max_time;        /* max duration oa a round */
extern unsigned int so_base;            /* basis of the playing field */
extern unsigned int so_altezza;         /* height of the playing field */
extern unsigned int so_flag_min;        /* min number of flags for round */
extern unsigned int so_flag_max;        /* max number of flags for round */
extern unsigned int so_round_score;     /* total score for round */
extern unsigned int so_n_moves;         /* total moves for pawn */
extern unsigned int so_min_hold_nsec;   /* min number of nanoseconds of occupation of a cell by a pawn */
extern unsigned int flags_no;           /* randomized flags number */
#endif /* COMMON_H */

