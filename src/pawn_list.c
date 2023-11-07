/**
 * @file    pawn_list.c.
 */

#include "common.h"
#include "pawn_list.h"
#include <sys/shm.h>

pawn_t* pawns;

static int pawns_shm_id = -1;
static char* pawns_shm_addr;

#define PAWNS_SHM_ID IPC_PRIVATE      

void pawn_list_create(unsigned int pawns_num) {
    pawns_shm_id = shmget(PAWNS_SHM_ID, pawns_num * sizeof (pawn_t), IPC_CREAT | 0600);
    if (pawns_shm_id == -1) {
        perror("shmget pawn: ");
        exit(1);
    }

    pawns_shm_addr = shmat(pawns_shm_id, NULL, 0);
    if (!pawns_shm_addr) {
        perror("shmat: ");
        exit(1);
    }
    
    pawns = (pawn_t*) (pawns_shm_addr);
}

void pawn_list_free() {
    struct shmid_ds shm_desc;
    if (shmdt(pawns_shm_addr) == -1) {
        perror("shmdt: ");
    }
    if (shmctl(pawns_shm_id, IPC_RMID, &shm_desc) == -1) {
        perror("shmctl: ");
    }
#if DEBUG
    LOGGER_INFO(("Pawn list has been removed\n"));
#endif   
}

char* pawn_list_get_addr(){
    return pawns_shm_addr;
}