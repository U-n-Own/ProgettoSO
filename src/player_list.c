/**
 * @file    player_list
 * @brief   The player list is rappresented as an array of player_t in shared memory. 
 *          The list contains all the info about players and their pawns.
 */

#include "common.h"
#include "player_list.h"
#include <sys/shm.h>

#define PLAYERS_SHM_ID IPC_PRIVATE      

player_t* players;

static int players_shm_id = -1;
static char* players_shm_addr;

void player_list_create(unsigned int players_num){
    int i;
    
    players_shm_id = shmget(PLAYERS_SHM_ID, players_num * sizeof(player_t), IPC_CREAT | 0600);
    if (players_shm_id == -1) {
        perror("shmget player: ");
        exit(1);
    }

    players_shm_addr = shmat(players_shm_id, NULL, 0);
    if (!players_shm_addr) {
        perror("shmat: ");
        exit(1);
    }

    players = (player_t*) (players_shm_addr);

}

void player_list_free(){
    struct shmid_ds shm_desc;

    if (shmdt(players_shm_addr) == -1) {
        perror("shmdt: ");
    }
    if (shmctl(players_shm_id, IPC_RMID, &shm_desc) == -1) {
        perror("shmctl: ");
    }
    
#if DEBUG
    LOGGER_INFO(("Player list has been removed\n"));
#endif   
}
