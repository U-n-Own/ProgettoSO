/**
 * @file:   flag_list.h
 */

#ifndef FLAG_LIST_H
#define FLAG_LIST_H

void flag_list_create(unsigned int flags_num);
void flag_list_free();
char* flag_list_get_address();
unsigned int* score_assignment(unsigned int flags_no, unsigned int max_score);

extern flag_t* flags;           /* array containg all the flags data */

#endif /* FLAG_LIST_H */

