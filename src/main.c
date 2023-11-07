/**
 * @file    main.c
 */

#include "common.h"
#include "master.h"
#include "player.h"
#include "pawn_list.h"
#include "player_list.h"
#include "flag_list.h"
#include <time.h>



time_t begin, end;
pid_t main_pid;


unsigned int player_number;
void parameter_init(char* file);
void data_structures_init();
void data_structures_free();
void pawns_assignment();
int* read_variables_from_file(char* file);

int main(int argc, char** argv){
    
    begin = time(NULL);    
    main_pid = getpid();

    parameter_init(argv[1]);
    data_structures_init();
    master_init();
    
    player_create(so_num_g);
    pawn_create(so_num_g * so_num_p);
    
    pawns_assignment();

   
    master_handler();
    
    
    
    data_structures_free();
    
    end = time(NULL);
    printf("## ELAPSED TIME ##: %ld\n", end - begin);

    return 0;
}

void parameter_init(char* file) {
    int *variables = read_variables_from_file(file); 

    so_num_g = variables[0];
    so_num_p = variables[1];
    so_max_time = variables[2];
    so_base = variables[3];
    so_altezza = variables[4];
    so_flag_min = variables[5];
    so_flag_max = variables[6];
    so_round_score = variables[7];
    so_n_moves = variables[8];
    so_min_hold_nsec = variables[9];

    flags_no = randomize_flags_number();
}

void data_structures_init(){
    message_queue_create();
    chessboard_create(so_base, so_altezza);
    player_list_create(so_num_g);
    pawn_list_create(so_num_g * so_num_p);
    flag_list_create(flags_no); 
    goal_list_create(flags_no);

    semaphores_vector = create_semaphores(so_num_g);
    for (player_number = 0; player_number < so_num_g; player_number++) {
        init_semaphores(semaphores_vector[player_number]);
    }
}

void data_structures_free(){
    pawn_list_free();
    player_list_free();
    flag_list_free();
    goal_list_free();
    chessboard_destroy();
    message_queue_destroy();

    for (player_number = 0; player_number < so_num_g; player_number++) {
        semctl(semaphores_vector[player_number], 0, IPC_RMID);
    }
}

void pawns_assignment() {
    unsigned int total_moves = 0;

    int i, j;
    for(i = 0; i < so_num_g; ++i) {
        total_moves = 0;
        players[i].pawns = (pawn_t*) (pawn_list_get_addr() + i * so_num_p * sizeof (pawn_t));  /*scorre con indirizzo di memoria*/
        for(j = 0; j < so_num_p; ++j) {
            players[i].pawns[j].moves = so_n_moves;
            players[i].pawns[j].player_no = i;
            
            total_moves = total_moves + players[i].pawns[j].moves;
        }
            





        printf("player %d has a total of %d moves\n", i, total_moves);
    }
}


int* read_variables_from_file(char* file) {
    int temp;
    int* var_array = (int*) malloc(sizeof(int*) * 10);
    int i = 0;


    FILE *my_file = fopen(file, "r");
    if (my_file == NULL) {
        perror("Error: Failed to open file.\n");
        return NULL;
    }

    for (i; i < 10; ++i) {
        if (fscanf(my_file, "%d,", &temp) == 1) {
            var_array[i] = temp;
        } else {
            break; 
        }
    }

    fclose(my_file);
    return var_array;
}

