/* 
 * File:   player.c
 */

#include "common.h"
#include <signal.h>
#include "message_queue.h"
#include "pawn.h"
#include "chessboard.h"
#include "player_list.h"
#include <errno.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>


#define SHM_ID 1937    /* ID of shared memory to generate */

static unsigned int player_no; /* used to access the player list */
static volatile unsigned int player_state = 0;

void player_place_pawn(unsigned int player_no, unsigned int pawn_no);
void give_instructions_to_pawn(unsigned int player_no, unsigned int pawn_no, unsigned int moves_no);
void zero_waiting(unsigned int sem_id);
int * calculate_distance(unsigned int col1, unsigned int col2, unsigned int row1, unsigned int row2);
void start_nanosleep();
/* For every pawn set an array of distances from flags */
void populate_distances_array(unsigned int pawn_no, unsigned int player_no);

char* shm_addr;    /* address of shared memory */
int shm_id_player;
cell_t** chessboard;
char* shm_addr_from_function;

/**
 * @brief Deprecated way to syncronize players
 */
static void signal_handler(int signum){
    player_state = 1;
#if DEBUG
    LOGGER_INFO(("SIGUSR1: %d\n", player_no));
#endif
}

/*
 * @brief   Player handler
 */
void player_handler(int player_n) {
    unsigned int remaining_player = so_num_g;


    msgbuf msg;
    char play = 1;
    unsigned int pawn_no;
    int index;
    struct sembuf sop;

    player_no = player_n;
    players[player_no].pid = getpid();
    
    /* notify the master when ready*/
    send_message(master_pid, MSG_READY, 0);

    /* player waits for messages from the master process */
    while (play) {
     
        msg = receive_message(players[player_no].pid, 1);
        switch (msg.mtext[0]) {
            case MSG_PAWN_PLACEMET:
                pawn_no = msg.mparameter;

                player_place_pawn(player_no, pawn_no);


                
#if DEBUG
                LOGGER_INFO(("Player %d has placed pawn %d\n", player_n, pawn_no));
#endif
                send_message(master_pid, MSG_DONE, 0);
                break;
                
            case MSG_TERMINATE:
                play = 0;
                break;
            
            case MSG_TERMINATE_ROUND:

                break;

            case MSG_FLAG_PLACEMENT:
                send_message(master_pid, MSG_DONE, 0);
                
                break;

            case MSG_START_GAME:

                for (pawn_no = 0; pawn_no < so_num_p; pawn_no++) {
                    give_instructions_to_pawn(pawn_no, player_no, players[player_no].pawns[pawn_no].moves);
                }
                
                break;
        }
    }
#if DEBUG
    LOGGER_INFO(("Player %d has terminated\n", player_no));
#endif
    send_message(master_pid, MSG_EXIT, 0);
    exit(0);
}



/**
 * @brief   Creates num_players players. Players are rappresented as child processes 
 *          of the main process.
 */
void player_create(unsigned int num_players) {
    int player_no;
    pid_t pid;
    msgbuf msg;

    for (player_no = 0; player_no < num_players; ++player_no) {
        pid = fork();
        if (pid < 0) {
            perror("fork error");
            exit(1);
        } else if (pid == 0) {
            player_handler(player_no);
            exit(0);
        } 
    }
    /* Waits for all the players are ready */
    while (player_no) {
        msg = receive_message(main_pid, 1);
        if (msg.mtext[0] == MSG_READY)
            --player_no;
    }
#if DEBUG
    LOGGER_INFO(("All the players are ready\n"));
#endif

}


/**
 * Get shared memory address of chessboard (IPC_PRIVATE), generate
 * random integers for placing the pawns, then check if the cell pointed 
 * by the two numbers is busy, place the pawn if not, change number if 
 * it's busy
 * */
void player_place_pawn(unsigned int player_no, unsigned int pawn_no){
    unsigned int row, col;

    /* Initializing number generator with seed the PID of the current process */
    srand(rand()+ getpid());
    col = rand()%so_altezza;
    row = rand()%so_base;

    switch (player_no) {
        case 0:
            set_chessboard_pawn(col, row, player_no, pawn_no);
            break;

        case 1:
            set_chessboard_pawn(col, row, player_no, pawn_no);
            break;

        case 2:
            set_chessboard_pawn(col, row, player_no, pawn_no);
            break;

        case 3:
            set_chessboard_pawn(col, row, player_no, pawn_no);
            break;
    }
}


/* movement functions */
int move_forward(unsigned int pawn_no, unsigned int player_n) {
    int col_offset = 0, row_offset = 0;
    col_offset += 1;

    /*chiamata a funzione in chessboard*/
    if (move_pawn(pawn_no, player_no, col_offset, row_offset))
        return TRUE;
    else
        return FALSE;
}

int move_backward(unsigned int pawn_no, unsigned int player_n){
    int col_offset = 0, row_offset = 0;
    col_offset -= 1;

    /*chiamata a funzione in chessboard*/
    if (move_pawn(pawn_no, player_no, col_offset, row_offset))
        return TRUE;
    else
        return FALSE;
}

int move_down(unsigned int pawn_no, unsigned int player_n){
    int col_offset = 0, row_offset = 0;
    row_offset += 1;

    /*chiamata a funzione in chessboard*/
    if (move_pawn(pawn_no, player_no, col_offset, row_offset))
        return TRUE;
    else
        return FALSE;
}

int move_up(unsigned int pawn_no, unsigned int player_n){
    int col_offset = 0, row_offset = 0;
    row_offset -= 1;

    /*chiamata a funzione in chessboard*/
    if (move_pawn(pawn_no, player_no, col_offset, row_offset))
        return TRUE;
    else 
        return FALSE;
} 


 void give_instructions_to_pawn(unsigned int pawn_no, unsigned int player_no, unsigned int moves_no) { 
    unsigned int col1, col2, row1, row2, pawn_index;
    int *distance;

    for(moves_no; moves_no > 0; moves_no--) {
        if(!move_up(pawn_no, player_no))
            if(!move_up(pawn_no, player_no))
                if(!move_forward(pawn_no, player_no))
                    if(!move_forward(pawn_no, player_no)) {}
        
    }
    
}
                
void zero_waiting(unsigned int sem_id) {
    struct sembuf sem_op;
    unsigned int sem_val;

    sem_op.sem_num = 0;
    sem_op.sem_op =  0;
    sem_op.sem_flg = 0;
    sem_val = semctl(sem_id, 0, GETVAL);
    
    if (semop(sem_id, &sem_op, 1) == -1)
        perror("semop err!: %d\n");
}


int * calculate_distance(unsigned int col1, unsigned int col2, unsigned int row1, unsigned int row2)  {
    
   
    int * distance_col_row, distance_col, distance_row;
    distance_col_row = (int*) malloc (2 * sizeof(int));
    
    printf("begin calculate distance\n");
    
    distance_col = col2 - col1;
    distance_row = row2 - row1;
    
    distance_col_row[0] = distance_col;
    distance_col_row[1] = distance_row;

    printf("end calculate distance\n");

    return distance_col_row;
}

void populate_distances_array(unsigned int pawn_no, unsigned int player_no) {
    int *distance;
    unsigned int pawn_index;
    int i;

    pawn_index = pawn_no + (player_no * so_num_p);


    distance = (int*) malloc (2 * sizeof(int));
}
