/*
 * @file    chessboard.c
 * @brief   The playing field is a chessboard rappresented as a matrix in shared memory.
 *          Each cell is protected by a semaphore.          
 */

#include "common.h"
#include "chessboard.h"
#include "pawn_list.h"
#include "flag_list.h"
#include "message_queue.h"
#include "master.h"
#include "player_list.h"
#include <sys/ipc.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define SHM_ID 1937      /* ID of shared memory to generate */
#define SHM_ID_GAME_RUNNING 1729 /* ID of shared memory to handle syncronization of rounds */
#define SEM_SET_ID IPC_PRIVATE      /* ID of the semaphore set to generate */

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

unsigned int randomize_col(unsigned int col);
unsigned int randomize_row(unsigned int row);
/* Return 0 if game is not running so pawn can't proceed in execution when game is on */
unsigned int is_running(); 
void start_nanosleep();

/* sigset_t pendingSignals, newMask, oldMask;
 */

/**
 * @brief   This variable is used to settle a seed for random generating placing positions 
 */
int seed;

/**
 * @brief   This variable is used to hold the shared memory segment identifier. 
 */
static int shm_id = -1;

static int shm_id_sync = -1;

/**
 * @brief   This variable is used to hold the semaphore set identifier. 
 */
static int sem_set_id = -1;

/** 
 * @brief   This variable is used to hold the address of the shared memory segment.  
 */
static char* shm_addr;

static char* shm_addr_sync;


/**
 * @brief   define a cells matrix variable. 
 */
static cell_t** cells;

/**
 * @brief   for chessboard managing
 */
static int cols;
static int rows;

/**
 * @brief create and attach the shared memory segment 
 */
void chessboard_create(int basis, int height) {
    int i;
    if (shm_id >= 0)
        return;

    cols = basis;
    rows = height;

    shm_id = shmget(SHM_ID, cols * rows * sizeof (cell_t) + sizeof(char), IPC_CREAT | 0600);
    if (shm_id == -1) {
        perror("shmget chess: ");
        exit(1);
    }

    shm_addr = shmat(shm_id, NULL, 0);
    if (!shm_addr) {
        perror("shmat: ");
        exit(1);
    }
    
    cells = (cell_t**) malloc(rows * sizeof (cell_t *));

    for (i = 0; i < rows; ++i)
        cells[i] = (cell_t*) (shm_addr + i * cols * sizeof (cell_t));


    sem_set_id = semget(SEM_SET_ID, cols * rows, IPC_CREAT | 0600);
    if (sem_set_id == -1) {
        perror("semget :");
        exit(1);
    }
}

/**
 * @brief   Detach and de-allocate the shared memory segment. Remove the semaphore set too.  
 */
void chessboard_destroy() {
    struct shmid_ds shm_desc;

    free(cells);

    semctl(sem_set_id, 0, IPC_RMID);

    if (shmdt(shm_addr) == -1) {
        perror("shmdt: ");
    }

    if (shmctl(shm_id, IPC_RMID, &shm_desc) == -1) {
        perror("shmctl: ");
    }

#if DEBUG
    LOGGER_INFO(("Chessboard has been destroyed\n"));
#endif
}

/**
 * @brief   .   
 */
void chessboard_reset() {
    int row, col, i;

    for (row = 0; row < rows; ++row)
        for (col = 0; col < cols; ++col) {
            cells[row][col].busy = 0;
            cells[row][col].type = 0;
        }
    
    /* initialize the semaphore set to 1. */
    for (i = 0; i < rows * cols; ++i) {
        if (semctl(sem_set_id, i, SETVAL, 1) == -1) {
            perror("reset_chessboard: semctl");
            exit(1);
        }
    }
}

/**
 * @brief   .   
 */
void chessboard_print() {
    int row, col, debug_num_pawns = 0;

    for (row = 0; row < rows; ++row) {
        for (col = 0; col < cols; ++col) {
             if (cells[row][col].busy == 0) { 
                 if (cells[row][col].type == PAWN){
                    if (cells[row][col].data.pawn->player_no == 0) {
                        printf(ANSI_COLOR_GREEN "A " ANSI_COLOR_RESET);
                    } else if (cells[row][col].data.pawn->player_no == 1) {
                        printf(ANSI_COLOR_RED "B " ANSI_COLOR_RESET);
                    } else if (cells[row][col].data.pawn->player_no == 2){
                        printf(ANSI_COLOR_CYAN "C " ANSI_COLOR_RESET);
                    } else if (cells[row][col].data.pawn->player_no == 3) {
                        printf(ANSI_COLOR_MAGENTA "D " ANSI_COLOR_RESET);
                    }
                    
                } else if (cells[row][col].type == FLAG) {
                    /* printf("%d ", cells[row][col].data.flag->score); */
                    printf(ANSI_COLOR_YELLOW "%d " ANSI_COLOR_RESET, cells[row][col].data.flag->score);

                } else if (cells[row][col].type == EMPTY) {
                    printf(". ");
                } else if (cells[row][col].type == P) {
                    printf(ANSI_COLOR_YELLOW ". " ANSI_COLOR_RESET);
                }
            }
        }
        printf("\n");
    }

    /*Debug info at end*/
    for (row = 0; row < rows; ++row) {
        for (col = 0; col < cols; ++col) {

            if (cells[row][col].type == PAWN) { 
                debug_num_pawns++;
            }
        }
    }
    
#if DEBUG 
    LOGGER_INFO(("Number of pawns remained on chessboard: %d\n", debug_num_pawns));
#endif
}


char* chessboard_shm_addr() {
    return shm_addr;
}

void set_chessboard_pawn(unsigned int row, unsigned int col, unsigned int player_no, unsigned pawn_no) {
    seed = clock();
    srand(seed);

    while (cells[row][col].type != EMPTY) {
        col = randomize_col(col);
        row = randomize_row(row);
    }

    lock_cell(row, col, 1);
    cells[row][col].type = PAWN;

    
    pawns[pawn_no + (player_no * so_num_p)].position.row = row;
    pawns[pawn_no + (player_no * so_num_p)].position.col = col;
    pawns[pawn_no + (player_no * so_num_p)].player_no = player_no; 
    /* pawns[pawn_no + (player_no * so_num_p)].moves = so_n_moves; */

    /* assign pawns address to the right cell on the chessboard */
    cells[row][col].data.pawn = &pawns[pawn_no + (player_no * so_num_p)];
}

void set_chessboard_flags(unsigned int row, unsigned int col, int index) {
    int i;
    flags = (flag_t*) flag_list_get_address();

    while (cells[row][col].type != EMPTY) {
        col = randomize_col(col);
        row = randomize_row(row);
    }
    /**
    * We don't need flags to have semaphore
    */
    /*lock_cell(row, col, 1);*/
    cells[row][col].type = FLAG;

    i = 0;
    flags[index].position.col = col;
    flags[index].position.row = row;
    flags[index].index = i;

    cells[row][col].data.flag = &flags[index];
    
}


/**
 * @brief  Lock cell on chessboard 
 */
int lock_cell(int row, int col, int blocking) {
    struct sembuf sem_op;
    
    
    sem_op.sem_num = row*cols + col;
    sem_op.sem_op = -1;
    /* sem_op.sem_flg = blocking ? 0 : IPC_NOWAIT; */ /* Here if we put blocking the process goes in deadlock with others */
    sem_op.sem_flg = IPC_NOWAIT; 
    semop(sem_set_id, &sem_op, 1);
    if (!blocking & errno == EAGAIN)
        return 0;
    else
        return 1;
}

/**
 * @brief  Unlock cell on chessboard 
 */
void unlock_cell(int row, int col){
    struct sembuf sem_op;
    
    
    sem_op.sem_num = row*cols + col;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}


unsigned int randomize_col(unsigned int col) {
    if (col == 1 || col == 0)
        col += 2;

    col = rand()%(so_base - 1);    
    return col;
}

unsigned int randomize_row(unsigned int row) {
    if (row == 1 || row == 0)
        row += 2;
    
    row = rand()%(so_altezza - 1);
    return row;
}

int move_pawn(unsigned int pawn_no, unsigned int player_no, int col_offset, int row_offset) {
    unsigned int row, col, old_row, old_col, flag_index_to_remove, remaining_moves;


    /* old values */
    old_row = pawns[pawn_no + (player_no * so_num_p)].position.row;
    old_col = pawns[pawn_no + (player_no * so_num_p)].position.col;

    /* new values */
    row = pawns[pawn_no + (player_no * so_num_p)].position.row + row_offset;
    col = pawns[pawn_no + (player_no * so_num_p)].position.col + col_offset;
    
    
    remaining_moves = pawns[pawn_no + (player_no * so_num_p)].moves;



    /* start_nanosleep(); */

    
    /* printf("pawn %d of player %d has %d remaining moves\n", pawn_no, player_no, remaining_moves); */

    if (row < so_altezza && col < so_base && is_running() && remaining_moves > 0) {
        if (cells[row][col].type == EMPTY || cells[row][col].type == P) {
            
            /*sigaddset(&newMask, SIGALRM);
            sigprocmask(SIG_BLOCK, &newMask, &oldMask);
            sigpending(&pendingSignals);

            /* printf("sigismember: %d\n", sigismember(&newMask, SIGALRM)); 

            if (sigismember(&newMask, SIGALRM)){
                if (is_running()) {
                    pause_game();
                }
            }*/

            lock_cell(row, col, 1);

            /* set new coordinates for the pawn */
            pawns[pawn_no + (player_no * so_num_p)].position.col = col;
            pawns[pawn_no + (player_no * so_num_p)].position.row = row;

            
            /* set the newly occupied to type pawn */
            cells[row][col].type = PAWN;

            /* set old cell to empty */
            cells[old_row][old_col].type = P;


            /* Move the 'pawn address' to the cell in shared memory */
            cells[row][col].data.pawn = &pawns[pawn_no + (player_no * so_num_p)];

    
                    
            /* decrease pawn remaining moves */
            /* if number of moves is 0 can't move */
            pawns[pawn_no + (player_no * so_num_p)].moves--;

            
            /* Relese resource after finished operation on the new */
            /* unlock previously occupied cell */
            unlock_cell(old_row, old_col);
          
            /* Restore the old mask.*/
  
            /*if (sigprocmask(SIG_UNBLOCK, &newMask, NULL) < 0) {
                perror("sigprocmask error: ");
            }*/
            return TRUE;

        } else if (cells[row][col].type == FLAG) {
               
            /*sigaddset(&newMask, SIGALRM);
            sigprocmask(SIG_BLOCK, &newMask, &oldMask);
    

            sigpending(&pendingSignals);
            if (sigismember(&pendingSignals, SIGALRM)){

                printf("\nSIGALARM IS BLOCKED HERE\n");
                pause_game();
                
            }*/

            /*IDEA:
            * Dato che in questo momento la pedina potrebbe essere fermata dal master per colpa del segnale di fine gioco
            * perchè non mettere un semaforo condiviso tra tutte le pedine che indica se il gioco è ancora attivo oppure il master
            * ha terminato il round o il gioco, cosi da impedire eventuali segnali asincroni che danneggiano lo stato delle pedine 
            * */
           
            /* lock the current cell in which the pawn is placed */
            lock_cell(row, col, 1);
            

            /* set new coordinates for the pawn */
            pawns[pawn_no + (player_no * so_num_p)].position.col = col;
            pawns[pawn_no + (player_no * so_num_p)].position.row = row;
            


            /* assign the score of the pawn that was placed in the cell to the player */
            score_control(player_no, cells[row][col].data.flag->score);

            /* Remember to remove the flags from shared memory */

            
            /* Send a msg to the master for notify that flag is taken and master assign score */
            send_message(master_pid, MSG_FLAG_CAPTURED, 0);
            
            /* write changes to the chessboard */ 
            cells[row][col].data.pawn = &pawns[pawn_no + (player_no * so_num_p)];
            
            
            
            /* set old cell to empty and new cell to pawn */
            cells[old_row][old_col].type = P;
            cells[row][col].type = PAWN;

            
            /* decrease pawn remaining moves */
            pawns[pawn_no + (player_no * so_num_p)].moves--;

      
            /* start_nanosleep(); */

            /* unlock previously occupied cell */
            unlock_cell(old_row, old_col); 
/* 
            if (sigprocmask(SIG_UNBLOCK, &newMask, NULL) < 0) {
                perror("sigprocmask error: ");
            }
 */
            return TRUE;

        /* handle case when cells[row][col].type == PAWN */
        } else if (cells[row][col].type == PAWN) {
            return FALSE;
        }

    } else {
    
        /* printf("out of bounds pawn: %d at row: %d - col: %d\n", pawn_no, row, col); */
        return FALSE;
    }

}

void semaphores_print() {
    int row, col, debug_num_pawns = 0;

    for (row = 0; row < rows; ++row) {
        for (col = 0; col < cols; ++col) {
            printf("\nSEM_ID:\t%d", cells[row][col].type);
        }
    }
}

/* Maybe we should delete this and use a semaphore */
unsigned int is_running() {

    unsigned int game_sem_id;

    if ((game_sem_id = semget(SEM_ID_GAME, 1, 0)) == -1) {
        perror("semget");
        exit(1);
    }

    /* printf("is_running game_sem_id: %d\n", game_sem_id); */
    if (semctl(game_sem_id, 0, GETVAL) == -1) {
        printf("getval error chess in pid: %d \n", getpid());
        return 0;/* grab the semaphore set created by seminit.c: */
 
    } else {
        return 1;
    }
}

/**
 * @brief   Struct for nanosleep
 */
struct timespec tim;
void start_nanosleep() {
    tim.tv_sec = 0;
    tim.tv_nsec = so_min_hold_nsec;

    nanosleep(&tim, NULL);
}