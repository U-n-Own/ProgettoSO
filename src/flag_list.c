/**
 * @file    flag_list.c
 */

#include "common.h"
#include "chessboard.h"
#include <sys/shm.h>
#include <math.h>


#define FLAGS_SHM_ID IPC_PRIVATE      

flag_t* flags;
int i, flag_point, random_score, col, row;
unsigned int* scores_vector;

static int flags_shm_id = -1;
static char* flags_shm_addr;

unsigned int* score_assignment(unsigned int flags_no, unsigned int max_score);


void flag_list_create(unsigned int flags_num){
    flags_shm_id = shmget(FLAGS_SHM_ID, flags_num * sizeof (flag_t), IPC_CREAT | 0600);
    if (flags_shm_id == -1) {
        perror("shmget flag: ");
        exit(1);
    }

    flags_shm_addr = shmat(flags_shm_id, NULL, 0);
    if (!flags_shm_addr) {
        perror("shmat: ");
        exit(1);
    }


}

void flag_list_free(){
    struct shmid_ds shm_desc;
    
    if (shmdt(flags_shm_addr) == -1) {
        perror("shmdt: ");
    }
    if (shmctl(flags_shm_id, IPC_RMID, &shm_desc) == -1) {
        perror("shmctl: ");
    }
#if DEBUG
    LOGGER_INFO(("Flag list has been removed\n"));
#endif   
}

/* Divides not-equally the values of max_score between flags*/
unsigned int* score_assignment(unsigned int flags_no, unsigned int max_score) {
    unsigned  int random_int, i, j, flag_score_sum, points_to_be_assigned, max_point_back, min, index;
    unsigned  int* scores_vector;

    index = 0;
    min = 0;
    flag_score_sum = 0;
    points_to_be_assigned = 0;
    i = 0;
    max_point_back = max_score;


    scores_vector = (unsigned int*) malloc(flags_no * sizeof(unsigned int));
    srand(getpid());
    

    while (max_score > 0 && i < flags_no) {

        random_int = rand() % (int)(ceil((max_score/(flags_no/5)))  + 1); /* HERE THERE ARE SOME PROBLEMS with division */
        /* random_int = rand() % (int)(ceil((max_score/(flags_no)))); /* With this version there aren't problem with division but point are given innacurately */
        scores_vector[i] = random_int;
        i++;
        
        if (max_score - random_int > 0)
            max_score -= random_int;
        else
            break;
        
    }

    for (i = 0; i < flags_no; i++) {
        flag_score_sum += scores_vector[i];
    }

    points_to_be_assigned = max_point_back - flag_score_sum;

    if (max_point_back - flag_score_sum > 0) {
        points_to_be_assigned = max_point_back - flag_score_sum;
        for (j = 0; j < flags_no; j++) {
            if (scores_vector[j] < scores_vector[j + 1]) {
                min = scores_vector[j];
                index = j;
            }
        }
        scores_vector[index] += points_to_be_assigned;
    }
    return scores_vector;
}

char* flag_list_get_address() {
    return flags_shm_addr;
}




