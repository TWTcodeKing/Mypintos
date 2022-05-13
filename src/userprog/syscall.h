#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
void syscall_init (void);
void*check_user_ptr(void * ptr); //judge whether the ptr is valid(method 2,PHYS_BASE judge)
void exit_with_error();
#endif /* userprog/syscall.h */
