/* 
 * @file    master.c
 * @brief   implements the master process
 */
#include "common.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <limits.h>
#include "player.h"
#include "chessboard.h"
#include "message_queue.h"
#include "pawn.h"
#include "master.h"
#include "pawn_list.h"
#include "player_list.h"
#include "flag_list.h"
#include "goal_list.h"

union semun {
    int val;
    struct semid_ds *buf; 
    __u_short *array; 
} argument;


/**
 * @brief   The prototype functions for the master process
 */
void place_flags();
void place_pawns();
void terminate_game();
unsigned int randomize_flags_number();
void goals_assignment(unsigned int flags_no);
void start_round();
unsigned int* create_semaphores(unsigned int player_number);
void init_semaphores(unsigned int sem_id);
void print_scores();

unsigned int flags_number;
unsigned int player_number;


/*
 * @brief   
 */
void master_init() {
    master_pid = main_pid;
}

/*
 * @brief   implements the master process handler
 */
void master_handler() {
    chessboard_reset();
  

    place_pawns();
    place_flags(flags_no);
    goals_assignment(flags_no);

    chessboard_print();

    start_round();
    
    /* Alarm setup for so_max_time */ 
    signal(SIGALRM, alarm_handler);

    /* without sleep, pawns can't complete their moves */
    sleep(1);
    
    printf("\n");
    chessboard_print();
    

    /*
    for(i = 0; i < SO_NUM_G; ++i)
        kill(players[i].pid, SIGUSR1);
    
    */
    
    print_scores();
    terminate_game();
}

/*
 * @brief   Players position their pawns one at a time, alternating.
 * 
 */
void place_pawns() {
    int pawn_no, player_no;
    msgbuf message;
    for (pawn_no = 0; pawn_no < so_num_p; ++pawn_no) {
        for (player_no = 0; player_no < so_num_g; ++player_no) {
            zero_waiting(semaphores_vector[player_no]);
            player_place_pawn(player_no, pawn_no); 
            /*
            send_message(players[player_no].pid, MSG_PAWN_PLACEMET, pawn_no);

            player_place_pawn(player_no);
            do {
                message = receive_message(master_pid, 1);
            } while (message.mtext[0] != MSG_DONE);*/
        }
#if DEBUG
        LOGGER_INFO(("All the players have placed a pawn\n\n"));
#endif
    }
}


/*
 * @brief   Master position flags_no flags
 * @TODO    Notify all player with a message
 */
void place_flags(unsigned int flags_no) {
    msgbuf message;

    unsigned int* scores_vector;
    unsigned int col, row;
    int i;
    
    
    scores_vector = (unsigned int*)malloc(flags_no * sizeof(unsigned int));
    scores_vector = score_assignment(flags_no, so_round_score);
    

    flags = (flag_t*) flag_list_get_address();

    
    for (i = 0; i < flags_no; i++) {
        flags[i].score = scores_vector[i];
    }

    for (i = 0; i < flags_no; i++){
        srand(getpid() + i);
        col = rand()%so_altezza;
        row = rand()%so_base;

        set_chessboard_flags(col, row, i);
    }
}

/*
 * @brief   Sends a terminate message to all the players and waits
 */
void terminate_game() {
    int status;
    unsigned int i, player_no, pawn_no;

    for (pawn_no = 0; pawn_no < so_num_g * so_num_p; ++pawn_no) {
        send_message(pawns[pawn_no].pid, MSG_TERMINATE, 0);
    }

    for (player_no = 0; player_no < so_num_g; ++player_no) {
        send_message(players[player_no].pid, MSG_TERMINATE, 0);
    }

    /* Waits for all the child processes to terninate */
    while (wait(&status) > 0);

#if DEBUG
    LOGGER_INFO(("Game off\n"));
#endif
}

/*
*  @brief   Generates a random number of flags between so_flag_min and so_flag_max
*/
unsigned int randomize_flags_number(){
    unsigned int flags_number;

    flags_number = 0;

    srand(getpid());
    if (so_flag_max - so_flag_min == 0)
        flags_number = so_flag_max;
    else if (so_flag_max - so_flag_min > 0)
        flags_number = (rand()% ((so_flag_max + 1) - so_flag_min) + so_flag_min);


    return flags_number;
}


void goals_assignment(unsigned int flags_no) {
    int pawn_no, player_no, flag_i;
    for (player_no = 0; player_no < so_num_g; player_no++) {
        for (pawn_no = 0; pawn_no < so_num_p; pawn_no++) {
            for (flag_i = 0; flag_i < flags_no; flag_i++) {
                goals[flag_i].position = flags[flag_i].position;
                pawns[pawn_no].goals = goals;
            }
        }
    }
}

void start_round(){
    unsigned int player_no, playing, score;
    msgbuf msg;
    pid_t pid;

    for (player_no = 0; player_no < so_num_g; player_no++) {
        send_message(players[player_no].pid, MSG_START_GAME, player_no);
    }
}





void score_control(unsigned int player_no, unsigned int flag_score) {
    players[player_no].score += flag_score;
}


unsigned int * create_semaphores(unsigned int player_number) {
    unsigned int sem_id1, sem_id2, sem_id3, sem_id4, i;
    unsigned int *sem_id_vector;
    
    sem_id_vector = (unsigned int*) malloc (player_number * sizeof(unsigned int));

    if(so_num_g == 2){
        sem_id1 = semget(SEM_ID_G1, 1, IPC_CREAT | 0600);
        sem_id2 = semget(SEM_ID_G2, 1, IPC_CREAT | 0600);
        sem_id_vector[0] = sem_id1;
        sem_id_vector[1] = sem_id2;
    } else {
        sem_id1 = semget(SEM_ID_G1, 1, IPC_CREAT | 0600);
        sem_id2 = semget(SEM_ID_G2, 1, IPC_CREAT | 0600);
        sem_id3 = semget(SEM_ID_G3, 1, IPC_CREAT | 0600);
        sem_id4 = semget(SEM_ID_G4, 1, IPC_CREAT | 0600);
        sem_id_vector[0] = sem_id1;
        sem_id_vector[1] = sem_id2;
        sem_id_vector[2] = sem_id3;
        sem_id_vector[3] = sem_id4;
    }

    for (i = 0; i < player_number; i++) {
        if (sem_id_vector[player_number] == -1)
            printf("error creating semaphore\n");
    }
    
    return sem_id_vector;
}


void init_semaphores(unsigned int sem_id) {
    argument.val = so_num_p;
    if (semctl(sem_id, 0, SETVAL, argument) == -1) {
        printf("error setting val\n");
    }
}


void print_scores() {
    unsigned int player_no, max_score = 0, winner = 0;
    printf("Scores\n");
    for (player_no = 0; player_no < so_num_g; player_no++) {
        if(players[player_no].score > max_score){
            max_score = players[player_no].score;
            winner = player_no;
        }
        printf("Player %d score = %d\n", player_no, players[player_no].score);
    }

    printf("The winner is player %d with %d points!\n",winner, max_score);      
}


/*
unsigned int* calculate_distance(unsigned int pawn_no, unsigned int flag_no) {
    unsigned int* distances;
    unsigned int player_no, flag_i, minimum_distance, min_d;
    distances = (unsigned int*) malloc (flags_number * sizeof(unsigned int));

    for (flag_i = 0; flag_i < flags_no; flag_i++) {
        
    }
}*/

 

void  alarm_handler(int sig){

    signal(SIGALRM, SIG_IGN);          /* ignore this signal       */
    printf("Time exeeded so_max_time: %d\n", so_max_time);
    signal(SIGALRM, alarm_handler);     /* reinstall the handler    */
    exit(0);
}        
    

