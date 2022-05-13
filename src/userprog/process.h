#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);


void push_argument(void **esp,int argc,int*argv); //function for arfs
#endif /* userprog/process.h */
