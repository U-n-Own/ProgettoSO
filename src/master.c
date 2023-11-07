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
#include <semaphore.h>
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
#include <sys/shm.h>


#define SHM_ID_GAME_RUNNING 1729       /* ID of shared memory to attach the master */

#define DEBUG_CYCLE_OFF 1


union semun {
    int val;
    struct semid_ds *buf; 
    __u_short *array; 
} argument;

union semun_game {
    int val;
    struct semid_ds *buf; 
    __u_short *array; 
} game_sem;


/**
 * @brief   The prototype functions for the master process
 */
void place_flags();
void place_pawns();
void terminate_game();
void terminate_round();
void restart_game();
unsigned int randomize_flags_number();
void goals_assignment(unsigned int flags_no);
void start_round();
unsigned int* create_semaphores(unsigned int player_number);
void init_semaphores(unsigned int sem_id);
void print_scores();
void alarm_handler();
/* Function that terminate round when all flags are captured */
void all_flag_captured();
/* Function to rename! we use this to get a variable string in shared memory and use this variable to syncronize game */
void master_set_game_on();
void master_set_game_off();
void destroy_game_on_shm();
/* Master initialize semaphore for round syncronization at 1 */
unsigned int * init_game_semaphore();
int get_game_sem_value();
/* Use semop to set binary semaphore at 1 */
void resume_game ();
/* Use semop to set binary semaphore at 0 */
int pause_game();



/* Use this for countdown of all flags until this is equal to flags_no */
int counter_flags = 0; 
int roundon = 0;
int round_number = 0;

unsigned int game_sem_id;

/**
 * @brief this variable is for get msg of flag get by pawn 
*/

msgbuf msg;

unsigned int flags_number;
/*unsigned int player_number;*/

/**
 * @brief   This variable is used to hold the shared memory segment identifier. 
 */
static int shm_id = -1;

/** 
 * @brief   This variable is used to hold the address of the shared memory segment.  
 */
static char* shm_addr;


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

    
    
    /**
     * Game semaphore initialization.
     * */
    init_game_semaphore();


    /**
     * Resete chessboard before starting game.
     * */
    chessboard_reset();
  

    /**
     * Placing pawns, flags and flags position will be 
     * stored in the goals structure for each pawn.
     * */
    place_pawns();


      /**
     * First game start
     * */
    restart_game();
}


void destroy_game_on_shm(){
    struct shmid_ds shm_desc;

    if (shmdt(shm_addr) == -1) {
        perror("shmdt: ");
    }

    if (shmctl(shm_id, IPC_RMID, &shm_desc) == -1) {
        perror("shmctl: ");
    }
}


void master_set_game_on() {
    
    if (shm_id >= 0)
        return;
    
    shm_id = shmget(SHM_ID_GAME_RUNNING, sizeof(char*)*10, IPC_CREAT | 0600);
    if (shm_id == -1) {
        perror("shmget master: gameon ");
        exit(1);
    }

    /* attach the master at end of shared memory */
    shm_addr = shmat(shm_id, NULL, 0);
    if (shm_addr == (char *) -1) {
        perror("shmat: attached failed in master ");
        exit(1);
    }

    shm_addr = "Running";

    printf("\n\nshm addr is on: %s\n\n", shm_addr);

}


void all_flag_captured(){

    while(roundon) {

    
        msg = receive_message(master_pid, 0);


        if(msg.mtext[0] == MSG_FLAG_CAPTURED){
            counter_flags++;
            printf("\nNumber of flags captured on number of total flags: %d/%d\n", counter_flags, flags_no);
        }


        if(counter_flags == flags_no){
            terminate_round();
            counter_flags = 0;

            print_scores();
            restart_game();

        }
    }
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
            send_message(players[player_no].pid, MSG_PAWN_PLACEMET, pawn_no);

            do {
                message = receive_message(master_pid, 1);
            } while (message.mtext[0] != MSG_DONE);
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
    int player_no;
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

   
    for (player_no = 0; player_no < so_num_g; ++player_no) {
        send_message(players[player_no].pid, MSG_FLAG_PLACEMENT, player_no);

        do {
            message = receive_message(master_pid, 1);
        } while (message.mtext[0] != MSG_DONE);
    }
#if DEBUG
        LOGGER_INFO(("All the players have placed a pawn\n\n"));
#endif

}



/*
 * @brief   Sends a terminate message to all the players and waits
 */
void terminate_game() {

    int status = 0;
    unsigned int i, player_no, pawn_no;


    pause_game();

    
    
    


    for (pawn_no = 0; pawn_no < so_num_g * so_num_p; ++pawn_no) {
        send_message(pawns[pawn_no].pid, MSG_TERMINATE, 0);
    }

    for (player_no = 0; player_no < so_num_g; ++player_no) {
        send_message(players[player_no].pid, MSG_TERMINATE, 0);
    }


    printf("\n\nFinal state of game\n\n");
    chessboard_print();
    print_scores();

    semctl(game_sem_id, 0, IPC_RMID);

    /* Waits for all the child processes to terninate */
    printf("\n\n\n\nWaiting for child to terminate\n\n\n");

    while (wait(&status) > 0);


#if DEBUG
    LOGGER_INFO(("Game off\n"));
#endif
}


/*
 * @brief   Sends a terminate message to all the players 
 */
void terminate_round() {

    int status;
    unsigned int i, player_no, pawn_no;

    pause_game();

    for (pawn_no = 0; pawn_no < so_num_g * so_num_p; ++pawn_no) {
        send_message(pawns[pawn_no].pid, MSG_TERMINATE_ROUND, 0);
    }

    for (player_no = 0; player_no < so_num_g; ++player_no) {
        send_message(players[player_no].pid, MSG_TERMINATE_ROUND, 0);
    }
 
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
                players[player_no].pawns[pawn_no].goals = goals;
            }
        }
    }
}

void start_round(){
    unsigned int player_no, playing, score;
    msgbuf msg;
    pid_t pid;
    struct sigaction sig_alrm;
    sig_alrm.sa_handler = alarm_handler;

    /* master start/resume game */
    resume_game();


    if(sigaction(SIGALRM, &sig_alrm, 0) == -1)
        perror("error in sigaction");

    


    for (player_no = 0; player_no < so_num_g; player_no++) {
        send_message(players[player_no].pid, MSG_START_GAME, player_no);
    }


    alarm(so_max_time);
}


void score_control(unsigned int player_no, unsigned int flag_score) {
    players[player_no].score += flag_score;
}


unsigned int * init_game_semaphore(){
    union semun_game game_sem;
    struct semid_ds buf;
    struct sembuf game_sb;

    game_sem_id = semget(SEM_ID_GAME, 1, IPC_CREAT | 0600);

    if (game_sem_id > 0) {
        printf("game_sem_id: %d\n", game_sem_id);
        game_sb.sem_op = 1;
        game_sb.sem_flg = 0;
        game_sem.val = 1;
    }


    if (semctl(game_sem_id, 0, SETVAL) == -1) {
        printf("game_sem error!\n");
        semctl(game_sem_id, 0, IPC_RMID); /* remove semaphore if SETVAL fails */
    } 
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
    unsigned int player_no, max_score = 0, winner = 0, pawn_no;
    int * list_number_moves_remained;
    unsigned int all_moves;

    list_number_moves_remained = (int*) malloc(sizeof(int)*so_num_g);

    printf("\n--------------------------------------------------------\n");
    printf("\n-Round %d has terminated\n", round_number);
    printf("\n-Scores:\n");
    for (player_no = 0; player_no < so_num_g; player_no++) {
        if(players[player_no].score > max_score){
            max_score = players[player_no].score;
            winner = player_no;
        }
        
        printf("\n-Player %d score = %d\n", player_no, players[player_no].score);
    }

    printf("\n-The winner is player %d with %d points!\n",winner, max_score); 
    

    for (player_no = 0; player_no < so_num_g; player_no++) {
        for (pawn_no = 0; pawn_no < so_num_p; pawn_no++) {
            list_number_moves_remained[player_no] += players[player_no].pawns[pawn_no].moves;
        }
        printf("\nPlayer %d has %d moves remaining.\n", player_no, list_number_moves_remained[player_no]);
    }

    printf("\n");
}


void restart_game() {
    unsigned int pawn_no, player_no, player_number;

    for (player_number = 0; player_number < so_num_g; player_number++) {
        init_semaphores(semaphores_vector[player_number]);
    }

    place_flags(flags_no);
    goals_assignment(flags_no);



    round_number++;

    
    printf("\n\n\nRound %d has started\n\n\n", round_number);
    chessboard_print();


    start_round();
    all_flag_captured();
}



void alarm_handler (int sig){
    /* Check that all paws have finished their operation */
    /* pause_game(); */

    /* vengono ignorati altri arrivi del segnale */
    /*signal(SIGALRM, SIG_IGN);*/  
    /* ripristino del signal handler */
    /*signal(SIGALRM, alarm_handler);*/
    
    /* Mandare un segnale alle pedine per aspettare finiscano di muoversi */

    printf("\nTime exeeded: %d\tseconds\n", so_max_time);
    
    terminate_game();
}


/**
 * @brief   .   
 */
int pause_game () {
    struct sembuf sem_op;
    
    roundon = 0;

    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0; 
    if (semop(game_sem_id, &sem_op, 1) == -1) {
        printf("semop error: pause game");
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief   .   
 */
void resume_game () {
    struct sembuf sem_op;

    roundon = 1;

    printf("resume_game -> game_sem_id: %d\n", game_sem_id);
    
    /* wait on the semaphore */
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    if((semop(game_sem_id, &sem_op, 1) == -1))
        perror("error in start game");
}



    


