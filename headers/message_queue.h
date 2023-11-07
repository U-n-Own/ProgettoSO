/* 
 * @file    message_queue.h
 * @brief   common macros and definitions for the message queue
 */

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <sys/types.h>

#include "common.h"

#define MSG_EXIT 'e'
#define MSG_READY 'r'
#define MSG_TERMINATE 't'
#define MSG_TERMINATE_ROUND 'o'
#define MSG_PAWN_PLACEMET 'p'
#define MSG_DONE 'd'
#define MSG_FLAG_PLACEMENT 'f'
#define MSG_START_GAME 's'
#define MSG_FLAG_CAPTURED 'c'
#define MSG_PLAYER_NUMBER 'n'

void message_queue_create();
void message_queue_destroy();
void send_message(pid_t recipient_id, char msg, unsigned int parameter);
msgbuf receive_message(pid_t recipient_id, char bloking);

#endif /* MESSAGE_QUEUE_H */
