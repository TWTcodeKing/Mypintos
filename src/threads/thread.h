#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/Fixed_Point_Arithmetic.h"
#include "threads/synch.h"
/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
#ifdef USERPROG
    struct child{
       tid_t tid;   /*tid of thread*/
       struct list_elem child_elem;
       int exit_status;  /*exit status*/
       bool finished;      /*whether it is successfully called */
       struct semaphore sema;  /*semaphore to control parent process's waiting(control parent sema)*/
    };

    struct process_file{
       int fd;
       struct file*file;
       struct list_elem file_elem;
    };
#endif 
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority in ready list. */
    int ticks_blocked;
    int base_priority;                  /* priority before donated*/
    struct list locks_holding;           /* locks current thread is holdingg*/
    struct lock *lock_waiting4;         /* lock current thread is waiting*/

    struct list_elem allelem;           /* List element for all threads list. */

    int recent_cpu;                     /*recent cpu used*/
    int nice;                          /*nice attributes*/
    fixed_t load_avg;                        /*load_avg attributes*/
    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */


#ifdef USERPROG
    /* Owned by userprog/process.c. */

    uint32_t *pagedir;                  /* Page directory. */
    int st_exit;                        /* exit status*/
    struct list childs;                 /*The list of all child processes created by this process*/
    struct child * thread_child;        /*child element as when as a child*/
    struct thread * parent;              /*parent of this process*/
    bool success;                        //??remain understanding
    struct semaphore sema;              /*control child sema*/
    

    struct list files;                  /*打开的所有文件*/
    int max_file_fd;                    //store max file_fd
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);
void blocked_thread_check(struct thread*t,void *aux UNUSED);
int thread_get_priority (void);
void thread_set_priority (int);
bool thread_priority_cmp(const struct list_elem *ele,const struct list_elem *cur,void*aux UNUSED);
bool cond_priority_cmp(const struct list_elem*a,const struct list_elem * b,void * au UNUSED);

/*functions for mlfqs*/
int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);
void mlfqs_Recent_Cpu_Inc();
void mlfqs_update_load_avg();
void mlfqs_update_priority(struct thread*t);

void  thread_update_priority(struct thread*t);
void thread_priority_donate(struct thread*t);
void thread_hold_lock(struct lock*lock);
void thread_release_lock(struct lock*lock);

#ifdef USERPROG
struct lock file_code_lock; //文件系统代码的全局锁,统一时间只能有一个进程访问文件管理代码
void aquire_file_lock();
void release_file_lock();
#endif
#endif /* threads/thread.h */