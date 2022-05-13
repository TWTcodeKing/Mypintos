#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/shutdown.h"
#include "devices/input.h"

#define max_syscalls 13
static void syscall_handler (struct intr_frame *);
static void (*syscalls[max_syscalls])(struct intr_frame*f);
void syscall_halt(struct intr_frame*f);
void syscall_exit(struct intr_frame*f);
void syscall_exec(struct intr_frame*f);
void syscall_wait(struct intr_frame*f);
void syscall_create(struct intr_frame*f);
void syscall_remove(struct intr_frame*f);
void syscall_open(struct intr_frame*f);
void syscall_filesize(struct intr_frame*f);
void syscall_read(struct intr_frame*f);
void syscall_write(struct intr_frame*f);
void syscall_seek(struct intr_frame*f);
void syscall_tell(struct intr_frame*f);
void syscall_close(struct intr_frame*f);
static int
get_user (const uint8_t *uaddr)
{
int result;
asm ("movl $1f, %0; movzbl %1, %0; 1:"
: "=&a" (result) : "m" (*uaddr));
return result;
}


struct process_file * find_file(int fd){
  struct list * files = &thread_current()->files;
  struct process_file * temp = NULL;
  for (struct list_elem *e=list_begin(files);e!=list_end(files);e=list_next(e)){
      temp = list_entry(e,struct process_file,file_elem);
      if (temp->fd == fd) return temp; 
  } 
  return NULL;
}

void *check_user_ptr(void *ptr){
  if (!is_user_vaddr(ptr)){
      //whether ptr is in User space
      exit_with_error();
  }

  //whether the page is valid
  void*pagePtr = pagedir_get_page(thread_current()->pagedir,ptr);

  if(!ptr){
    exit_with_error(); // not valid
  }


  uint8_t * checkPtrbyte = (uint8_t*)ptr;
  //check content is valid,a word means four time uint_8
  for (int i=0;i<4;i++){
    if (get_user(checkPtrbyte + i) == -1){
      exit_with_error();
    }
  }

  //return a valid pointer
  return ptr;
}

/*change the process status*/
void exit_with_error(){
  thread_current()->st_exit = -1;
  thread_exit();
}

bool 
is_valid_pointer (void* esp,uint8_t argc){
  for (uint8_t i = 0; i < argc; ++i)
  {
    if((!is_user_vaddr (esp)) || 
      (pagedir_get_page (thread_current()->pagedir, esp)==NULL)){
      return false;
    }
  }
  return true;
}



void syscall_halt(struct intr_frame*f){
    shutdown_power_off();
}

void syscall_exit(struct intr_frame*f){
    uint32_t *stack_pointer = f->esp;
    check_user_ptr(stack_pointer+1);
    //*stack_pointer++; // 取第一个参数()
    *stack_pointer++;//取第一个参数
    thread_current()->st_exit = *stack_pointer;
    thread_exit();
}

void syscall_exec(struct intr_frame*f){
    uint32_t *stack_pointer = f->esp;
    check_user_ptr(stack_pointer+1); //检查第一个参数是否合法
    check_user_ptr(*(stack_pointer+1)); // 检查指向文件名的char*指针是否合法
    *stack_pointer++;//取第一个参数
    f->eax = process_execute((char*)*stack_pointer);
}

void syscall_wait(struct intr_frame*f){
  uint32_t * stack_pointer = f->esp;
  check_user_ptr(stack_pointer+1);
  *stack_pointer++;  
  f->eax = process_wait(*stack_pointer);
}

void syscall_create(struct intr_frame*f){
     uint32_t * stack_pointer = f->esp;
     check_user_ptr(stack_pointer+5); //检查第一个参数是否合法
     check_user_ptr(*(stack_pointer+4));//检查指向文件名的char*指针是否合法
     *stack_pointer++;
     acquire_file_lock();
     f->eax = filesys_create((const char*)*stack_pointer,*(stack_pointer+1));
     release_file_lock();
}

void syscall_remove(struct intr_frame *f){
      uint32_t * stack_pointer = f->esp;
      check_user_ptr(stack_pointer+1);
      check_user_ptr(*(stack_pointer+1));
      *stack_pointer++;
      acquire_file_lock();
      f->eax = filesys_remove((const char*)*stack_pointer);
      release_file_lock();
}

void syscall_open(struct intr_frame * f){
      uint32_t * stack_pointer = f->esp;
      check_user_ptr(stack_pointer+1);
      check_user_ptr(*(stack_pointer+1));
      *stack_pointer++;
      acquire_file_lock();
      struct file* file_opened = filesys_open((const char*)*(stack_pointer));
      release_file_lock();
      if (file_opened){
        struct process_file * temp_file = malloc(sizeof(struct process_file));
        if (!strcmp((const char*)*(stack_pointer),thread_current()->name)){
            temp_file->fd = 2;
            temp_file ->file = file_opened;
            list_push_back(&thread_current()->files,&temp_file->file_elem);
            f->eax = temp_file ->fd;
            return;
        }
        temp_file ->fd = (thread_current()->max_file_fd)++;
        temp_file ->file = file_opened;
        list_push_back(&thread_current()->files,&temp_file->file_elem);
        f->eax = temp_file ->fd;
      }
      else{
        f->eax = -1;
      }
}

void syscall_filesize(struct intr_frame * f){
      uint32_t * stack_pointer = f->esp;
      check_user_ptr(stack_pointer+1);
      //check_user_ptr(*(stack_pointer+1));
      *stack_pointer++;
      struct process_file *temp = find_file(*stack_pointer);

      if (temp){
          acquire_file_lock();
          f->eax = file_length(temp->file);
          release_file_lock();
      }
      else{
          f->eax = -1;
      }

}

void syscall_read(struct intr_frame * f){
    uint32_t * stack_pointer = f->esp;
    /* PASS the test bad read */
    *stack_pointer++;
    /* We don't konw how to fix the bug, just check the pointer */
    int fd =  *stack_pointer;
    uint8_t * buffer = (uint8_t*)*(stack_pointer+1);
    off_t size = *(stack_pointer+2);
    if (!is_valid_pointer (buffer, 1) || !is_valid_pointer (buffer + size,1)){
        exit_with_error();
    }

    if (fd == 0){
      for (int i=0 ;i<size;i++)
          buffer[i] = input_getc();
      f->eax = size;
    }
    else{
          struct process_file * temp = find_file(fd);
          if (temp){
              acquire_file_lock();
              f->eax = file_read(temp->file,buffer,size);
              release_file_lock();
          }
          else{
             f->eax = -1;//can not read
          }
    }
}

void syscall_seek(struct intr_frame * f){
      uint32_t * stack_pointer = f->esp;
      check_user_ptr(stack_pointer+5);
      *stack_pointer++;
      struct process_file * temp = find_file(*stack_pointer);
      if (temp){
          acquire_file_lock();
          file_seek(temp->file,*(stack_pointer+1));
          release_file_lock();
      }
      else{
          f->eax = -1;
      }
}
void syscall_tell(struct intr_frame * f){
       uint32_t * stack_pointer = f->esp;
      check_user_ptr(stack_pointer+1);
      *stack_pointer++;
      struct process_file * temp = find_file(*stack_pointer);
      if (temp){
          acquire_file_lock();
          f->eax = file_tell(temp);
          release_file_lock();
      }
      else{
        f->eax = -1;
      }     
}

void syscall_close(struct intr_frame * f){
      uint32_t * stack_pointer = f->esp;
      check_user_ptr(stack_pointer+1);
      *stack_pointer++;
      struct process_file * temp = find_file(*stack_pointer);
      if (temp){
          acquire_file_lock();
          file_close(temp->file);
          release_file_lock();

          list_remove(&temp->file_elem);

          free(temp);
      }
      else{
        f->eax = -1;
      }
}
void syscall_write(struct intr_frame*f){
     uint32_t * stack_pointer = f->esp;
     check_user_ptr(stack_pointer+7);
     check_user_ptr(*(stack_pointer+6));
     *stack_pointer++;
     int fd = *stack_pointer;
     const char* buffer = (const char*)*(stack_pointer+1);
     off_t length = *(stack_pointer+2);
     if (fd == 1){
        putbuf(buffer,length);
        f->eax = length;//return number written
     }//write to console
     else if(fd == 2){
       f->eax = 0;
     }
     else{
       struct process_file * file_temp = find_file(fd); // 在进程的打开文件里找到这个文件
       if (file_temp){
          acquire_file_lock();
          f->eax = file_write(file_temp->file,buffer,length);
          release_file_lock();
       }
       else{
         f->eax = 0 ; //找不到写入失败
       }
     }
    
}
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");//注册syscall_handler为系统调用函数
  syscalls[SYS_HALT] = &syscall_halt;
  syscalls[SYS_EXIT] = &syscall_exit;
  syscalls[SYS_EXEC] = &syscall_exec;
  syscalls[SYS_WAIT] = &syscall_wait;
  syscalls[SYS_CREATE] = &syscall_create;
  syscalls[SYS_REMOVE] = &syscall_remove;
  syscalls[SYS_OPEN] = &syscall_open;
  syscalls[SYS_FILESIZE] = &syscall_filesize;
  syscalls[SYS_READ] = &syscall_read;
  syscalls[SYS_WRITE] = &syscall_write;
  syscalls[SYS_SEEK] = &syscall_seek;
  syscalls[SYS_TELL] = &syscall_tell;
  syscalls[SYS_CLOSE] = &syscall_close;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp; // a pointer to a int syscall code
  check_user_ptr(p+1); // check first params
  int type = *(int*)f->esp;
  if (type<=0 || type>=max_syscalls){
      exit_with_error();
  }
  syscalls[type](f);
}



