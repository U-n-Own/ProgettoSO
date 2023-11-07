/**
 * @file    goal_list.c
 */

#include "common.h"
#include <sys/shm.h>

#define GOALS_SHM_ID IPC_PRIVATE      

goal_t* goals;


static int goals_shm_id = -1;
static char* goals_shm_addr;

void goal_list_create(unsigned int goals_num){
    goals_shm_id = shmget(GOALS_SHM_ID, goals_num * sizeof (goal_t), IPC_CREAT | 0600);
    if (goals_shm_id == -1) {
        perror("shmget goal: ");
        exit(1);
    }


    goals_shm_addr = shmat(goals_shm_id, NULL, 0);
    if (!goals_shm_addr) {
        perror("shmat: ");
        exit(1);
    }
    
    goals = (goal_t*) (goals_shm_addr);
}

void goal_list_free(){
    struct shmid_ds shm_desc;
    
    if (shmdt(goals_shm_addr) == -1) {
        perror("shmdt: ");
    }
    if (shmctl(goals_shm_id, IPC_RMID, &shm_desc) == -1) {
        perror("shmctl: ");
    }
#if DEBUG
    LOGGER_INFO(("Goal list has been removed\n"));
#endif   
}

