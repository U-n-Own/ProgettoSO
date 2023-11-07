/*
 * @file    master.h
 */

#ifndef MASTER_H_H
#define MASTER_H_H


void master_handler();
void master_init();
unsigned int randomize_flags_number();
void score_control(unsigned int player_no, unsigned int flag_score);
void init_semaphores(unsigned int player_no);
unsigned int * create_semaphores(unsigned int player_number);

#endif /* MASTER_H_H */

