/**
 * @file    goal_list.h
 */

#ifndef GOAL_LIST_H
#define GOAL_LIST_H

void goal_list_create(unsigned int goal_num);
void goal_list_free();

extern goal_t* goals;

#endif /* GOAL_LIST_H */
