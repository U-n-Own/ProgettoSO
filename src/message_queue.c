/*
 * @file    message_queue.c
 * @brief   Implements a message queue for interprocess communication. The pid of the process
 *          is used as recipient id. Messages are represented as char
 */

#include "common.h"
#include <stdlib.h>
#include <sys/msg.h> 
#include <errno.h>
#include <stdio.h>

#define QUEUE_ID IPC_PRIVATE        /* ID of queue to generate. */

static int queue_id = -1;

void message_queue_create() {
    if (queue_id >= 0)
        return;

    queue_id = msgget(QUEUE_ID, IPC_CREAT | 0600);
    if (queue_id == -1) {
        perror("msgget error");
        exit(1);
    }
}

void message_queue_destroy() {
    if (queue_id == -1)
        return;

    if (msgctl(queue_id, IPC_RMID, 0) == -1) {
        perror("destroy_message_queue error");
        exit(1);
    }
#if DEBUG
    LOGGER_INFO(("Message queue has been destroyed\n"));
#endif
}

void send_message(pid_t recipient_id, char msg, unsigned int parameter) {
    msgbuf message;
    message.mtype = recipient_id;
    message.mtext[0] = msg;
    message.mparameter = parameter;
    if (msgsnd(queue_id, &message, sizeof(message.mtext) + sizeof(message.mparameter), 0) == -1) {
        perror("send_message error");
        exit(1);
    }
}

msgbuf receive_message(pid_t recipient_id, char blocking){
    msgbuf message;
    if (msgrcv(queue_id, &message, sizeof(message.mtext) + sizeof(message.mparameter), recipient_id, blocking ? 0 : IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
            message.mtext[0] = '0';
            return message;
        }
               
        perror("receive_message error");
        exit(1);
    }
    return message;
}

