/**
 * @file    pawn.c
 */
#include "common.h"
#include "message_queue.h"
#include "pawn_list.h"
#include <signal.h>
#include <stdlib.h>
#include "player_list.h"
#include "chessboard.h"

static unsigned int pawn_no; /* used to access the player list */
static volatile unsigned int pawn_state = 1;
static unsigned int player_no;

/* int move_backward(unsigned int pawn_no, unsigned int player_n);
int move_forward(unsigned int pawn_no, unsigned int player_n);
int move_up(unsigned int pawn_no, unsigned int player_n);
int move_down(unsigned int pawn_no, unsigned int player_n); */


void begin_pawn_movement(unsigned int pawn_no, unsigned int player_no);

/**
 * @brief Deprecated way to syncronize pawns
 */
static void signal_handler(int signum) {
    pawn_state = 0;
#if DEBUG
    LOGGER_INFO(("SIGUSR1: %d\n", pawn_no));
#endif  
}

void pawn_handler(unsigned int pawn_n) {
    msgbuf msg;

    pawn_no = pawn_n;
    pawns[pawn_no].pid = getpid();
    
    /* notify the master when ready */
    send_message(master_pid, MSG_READY, 0);

    while (pawn_state) {
        msg = receive_message(pawns[pawn_no].pid, 1);

        switch (msg.mtext[0]) {
            case MSG_TERMINATE:
                pawn_state = 0;
                break;

            case MSG_TERMINATE_ROUND:

                break;

        }
    }

#if DEBUG
    LOGGER_INFO(("Pawn %d of Player %d has terminated\n", pawn_no, pawns[pawn_no].player_no));
#endif
    send_message(master_pid, MSG_EXIT, 1);
    exit(0);
}

/**
 * @brief   Creates num_pawns players. Pawns are rappresented as child processes 
 *          of the main process.
 */
void pawn_create(unsigned num_pawns) {
    msgbuf msg;
    int pawn_no;
    pid_t pid;

    for (pawn_no = 0; pawn_no < num_pawns; ++pawn_no) {
        pid = fork();
        if (pid < 0) {
            perror("fork error");
            exit(1);
        } else if (pid == 0) {
            pawn_handler(pawn_no);
            exit(0);
        }
    }
    
    /* Waits for all the player are ready */
    while (pawn_no) {
        msg = receive_message(main_pid, 1);
        if (msg.mtext[0] == MSG_READY)
            --pawn_no;
    }
#if DEBUG
    LOGGER_INFO(("All the pawns are ready\n"));
#endif
}
