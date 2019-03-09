#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <machine/trapframe.h>
#include <synch.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include "opt-A2.h"

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

//todo
#if OPT_A2
void sys__exit(int exitcode) {
    lock_acquire(lk);
    struct addrspace *as;
    struct proc *p = curproc;

    if((curproc->parent) != NULL) {//If one has a parent
      if((curproc->parent->ptr) == NULL){ //If one has a parent and dead
        //child exit and parent is dead
        //it recycle itself
        int unwanted_pid = curproc->pid;
        pid_t* tmp = kmalloc(sizeof(int));
       *tmp = unwanted_pid;
        array_add(pid_arr, tmp, NULL);
      }else if ((curproc->parent->ptr) != NULL){ //If one has a parent and alive
        //inform parent and place return value if parent is still alive
        //we use the lock of the object that we are changing it field
        //we want to modify the parent's fields
        struct array *p_children = curproc->parent->ptr->children;

        unsigned int p_c_length = array_num(p_children);
        //loop through the entire children
        // 1. find the pid
        // 2. change return value then exit
        for (unsigned int i = 0; i < p_c_length; ++i) {
          struct info *inf = array_get(p_children, i);
          if ((inf->pid == curproc->pid)&&(inf->ptr != NULL)) {
            inf->child_return = _MKWAIT_EXIT(exitcode);
            inf->ptr = NULL;
            break;
          }
        }
        //wake the parents that are still alive and
        //waiting on parent its own lock
        cv_broadcast(curproc->parent->ptr->p_cv, lk);
      }
    }

    //inform children and change ptr to NULL
    //if there is children
    unsigned int n = array_num(curproc->children);

    for (unsigned int i = 0; i < n; ++i) {
      struct info *in = array_get(curproc->children, i);
      if (in->ptr != NULL) { //if children is not dead
        KASSERT(in->ptr->parent != NULL);//if children has a parent
        in->ptr->parent->ptr = NULL; //set dead flag on its children

      }else{//children is dead and the parent recycle the children
        int unwanted_pid2 = in->pid;
        pid_t* tmp = kmalloc(sizeof(int));
       *tmp = unwanted_pid2;
        array_add(pid_arr, tmp, NULL);
      }
    }
    lock_release(lk);

    DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

    KASSERT(curproc->p_addrspace != NULL);
    as_deactivate();
    /*
     * clear p_addrspace before calling as_destroy. Otherwise if
     * as_destroy sleeps (which is quite possible) when we
     * come back we'll be calling as_activate on a
     * half-destroyed address space. This tends to be
     * messily fatal.
     */
    as = curproc_setas(NULL);
    as_destroy(as);

    /* detach this thread from its process */
    /* note: curproc cannot be used after this call */
    proc_remthread(curthread);

    /* if this is the last user process in the system, proc_destroy()
       will wake up the kernel menu thread */

    proc_destroy(p);
    thread_exit();
    /* thread_exit() does not return, so we should never get here */
    panic("return from thread_exit in sys_exit\n");
  }

  void sys__kill(int exitcode) {
    lock_acquire(lk);
    struct addrspace *as;
    struct proc *p = curproc;

    if((curproc->parent) != NULL) {//If one has a parent
      if((curproc->parent->ptr) == NULL){ //If one has a parent and dead
        //child exit and parent is dead
        //it recycle itself
        int unwanted_pid = curproc->pid;
        pid_t* tmp = kmalloc(sizeof(int));
       *tmp = unwanted_pid;
        array_add(pid_arr, tmp, NULL);
      }else if ((curproc->parent->ptr) != NULL){ //If one has a parent and alive
        //inform parent and place return value if parent is still alive
        //we use the lock of the object that we are changing it field
        //we want to modify the parent's fields
        struct array *p_children = curproc->parent->ptr->children;

        unsigned int p_c_length = array_num(p_children);
        //loop through the entire children
        // 1. find the pid
        // 2. change return value then exit
        for (unsigned int i = 0; i < p_c_length; ++i) {
          struct info *inf = array_get(p_children, i);
          if ((inf->pid == curproc->pid)&&(inf->ptr != NULL)) {
            inf->child_return = _MKWAIT_SIG(exitcode);
            inf->ptr = NULL;
            break;
          }
        }
        //wake the parents that are still alive and
        //waiting on parent its own lock
        cv_broadcast(curproc->parent->ptr->p_cv, lk);
      }
    }

    //inform children and change ptr to NULL
    //if there is children
    unsigned int n = array_num(curproc->children);

    for (unsigned int i = 0; i < n; ++i) {
      struct info *in = array_get(curproc->children, i);
      if (in->ptr != NULL) { //if children is not dead
        KASSERT(in->ptr->parent != NULL);//if children has a parent
        in->ptr->parent->ptr = NULL; //set dead flag on its children

      }else{//children is dead and the parent recycle the children
        int unwanted_pid2 = in->pid;
        pid_t* tmp = kmalloc(sizeof(int));
       *tmp = unwanted_pid2;
        array_add(pid_arr, tmp, NULL);
      }
    }
    lock_release(lk);

    DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

    KASSERT(curproc->p_addrspace != NULL);
    as_deactivate();
    /*
     * clear p_addrspace before calling as_destroy. Otherwise if
     * as_destroy sleeps (which is quite possible) when we
     * come back we'll be calling as_activate on a
     * half-destroyed address space. This tends to be
     * messily fatal.
     */
    as = curproc_setas(NULL);
    as_destroy(as);

    /* detach this thread from its process */
    /* note: curproc cannot be used after this call */
    proc_remthread(curthread);

    /* if this is the last user process in the system, proc_destroy()
       will wake up the kernel menu thread */

    proc_destroy(p);
    thread_exit();
    /* thread_exit() does not return, so we should never get here */
    panic("return from thread_exit in sys_exit\n");
  }

  int sys_waitpid (pid_t pid,
              userptr_t status,
              int options,
              pid_t *retval)
  {
    lock_acquire(lk);
    int exitstatus;
    int result;

    if (options != 0) {
      return(EINVAL);
    }

    //parent use the cv of itself
    struct cv* cv = NULL;
    unsigned int num = array_num(curproc->children);
    unsigned int i;
    struct info* ix = NULL;
    for (i = 0; i < num; ++i) {
      ix = array_get(curproc->children, i);
      if((ix->ptr == NULL)&&(ix->pid == pid)){
        //if the children is already dead
        //just get the exit code
        exitstatus = ix->child_return;
        break;
      }

      else if(ix->pid == pid){
        //otherwise the children is not dead yet
        //note that parent is acquiring the lock of itself

        cv = curproc->p_cv;
        //if the children is not dead then
          KASSERT(cv!=NULL);
          while(ix->ptr != NULL){
            //wait until child finish,
            //sleep on parent its own cv, use parent its own lock
            cv_wait(cv, lk);
          }
          exitstatus = ix->child_return;
          //todo possible error in the future seem to be removable
          struct info* infor = array_get(curproc->children, i);
          kfree(infor);
          array_remove(curproc->children, i);
        //now the child is dead,
        //find the exit code and exit
        break;
      }
    }

    lock_release(lk);

    result = copyout((void *)&exitstatus,status,sizeof(int));
    if (result) {
      return(result);
    }
    *retval = pid;
    return(0);
  }


  int sys_fork(struct trapframe *parent_tf, pid_t *retval){
    //Create process structure for child process
    struct proc * child_proc = proc_create_runprogram(curproc->p_name);
    lock_acquire(lk);//todo
    if(child_proc == NULL) panic("child_proc initialized failure.");

    //Create and copy address space
    //Attach the newly created address space to the child process structure
    int ret_copy = as_copy(curproc_getas(), &(child_proc->p_addrspace));
    if(ret_copy != 0){
      proc_destroy(child_proc);
      panic("as_copy fails");
      return ENOMEM;
    }

    //pid is generated in proc_create_runprogram


   //create trapframe
    struct trapframe* tf = kmalloc(sizeof(struct trapframe));
    if(tf==NULL) panic("trapframe kmalloc error");
    memcpy(tf, parent_tf, sizeof(struct trapframe));

    //create the parent/child relationship
    //create child and parent info objects
    struct info* prt = kmalloc(sizeof(struct info));
    struct info* child = kmalloc(sizeof(struct info));


    prt->pid = curproc->pid;
    prt->ptr = curproc;
    prt->child_return = 0;

    child->pid = child_proc->pid;
    child->ptr = child_proc;
    child->child_return = 0;

    //create children array for child process
    if(child_proc->children == NULL){
      panic("child_proc->children create failure from proc_syscalls.c");
    }

    //add child for parent process
    int ret_addchild = array_add(curproc->children, child, NULL);
    if(ret_addchild!=0){
      panic("array_add add child to parent failed");
      return ret_addchild;
    }
    //add parent for child process
    child_proc->parent = prt;
    lock_release(lk);
    //Create thread for child process
    //(need a safe way to pass the trapframe to the child thread).
    int ret_threadf = thread_fork(curproc->p_name, child_proc, (void *)enter_forked_process, tf, 0);
    if(ret_threadf != 0){
        kfree(tf);
        proc_destroy(child_proc);
        panic("thread_fork error");
        return ret_threadf;
    }else{
      *retval = child_proc->pid;
      //otherwise return value for parent is pid
    }
    //lock_release(lk);
    return 0;
  }

  int sys_getpid(pid_t *retval) {
    /* for now, this is just a stub that always returns a PID of 1 */
    /* you need to fix this to make it work properly */
    lock_acquire(lk);
    *retval = curproc->pid;
    lock_release(lk);
    return 0;
  }


  //todo
  //todo
    //todo
  //todo
    //todo
  //todo
  int sys_execv(const char *program, char **args){
        //lock_acquire(lk);
        int exception;

        //1. Count the number of arguments
        int args_num = 0;
        int index = 0;
        while(args[index]!=NULL){
                index++;
        }
        args_num = index;

        //2. copy them into the kernel
        char** args_kernel = kmalloc(sizeof(char*) * (args_num + 1));
        if(args_kernel==NULL){//mem error1
                return ENOMEM;
        }

        for(int i = 0; i<=args_num; i++){
              if(i != args_num){
                      int args_size = strlen(args[i]) + 1;
                      args_kernel[i] = kmalloc(sizeof(char) * args_size);
                       if(args_kernel[i]==NULL){//mem error2
                              for(; i>=0; i--){
                                kfree(args_kernel[i]);
                              }
                              kfree(args_kernel);
                              return ENOMEM;
                       }

                       exception = copyinstr((const_userptr_t) args[i], args_kernel[i], args_size, NULL);
                       if(exception){//mem error3
                          for(; i>=0; i--){
                                kfree(args_kernel[i]);
                              }
                              kfree(args_kernel);
                              return exception;
                       }
              }else{
                args_kernel[i] = NULL;
              }
        }

        //3. Copy the program path into the kernel
        int ppath_len = strlen(program) + 1;
        int ppath_size = sizeof(char) * ppath_len;
        char* ppath = kmalloc(ppath_size);
        if(ppath == NULL){//mem error4
            for(int i=args_num; i>=0; i--){
              kfree(args_kernel[i]);
            }
            kfree(args_kernel);
            return ENOMEM;
        }

        exception = copyinstr((const_userptr_t) program, ppath, ppath_len, NULL);
        if(exception){//mem error 5
          kfree(ppath);
          for(int i=args_num; i>=0; i--){
              kfree(args_kernel[i]);
            }
            kfree(args_kernel);
            return exception;
        }

        //follow pattern of runprogram.c
        struct addrspace* as;
        struct vnode* v;
        vaddr_t entrypoint, stackptr;
        //4. Open the program file using
        exception = vfs_open((char *) ppath, O_RDONLY, 0, &v);
	if (exception) {
	    kfree(ppath);
            for(int i=args_num; i>=0; i--){
              kfree(args_kernel[i]);
            }
            kfree(args_kernel);
            return exception;
	}

	//5. Create new address space
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		kfree(ppath);
                for(int i=args_num; i>=0; i--){
                  kfree(args_kernel[i]);
                }
                kfree(args_kernel);
                return ENOMEM;
	}

	//6. set process to the new address space, and activate it
	struct addrspace* as_old = curproc_getas();
	curproc_setas(as);
	as_activate();

	//6. Using the opened program file, load the program image using load_elf
        exception = load_elf(v, &entrypoint);
        if(exception){
                vfs_close(v);
                kfree(ppath);

                //as_deactivate();//yet implemented
                curproc_setas(as_old);
                as_destroy(as);

                for(int i=args_num; i>=0; i--){
                  kfree(args_kernel[i]);
                }
                kfree(args_kernel);
                return exception;
        }

        //7. Done with the file, program path
	vfs_close(v);
        kfree(ppath);

        //8. define stack
        exception = as_define_stack(as, &stackptr);
        if(exception){
              //as_deactivate();//yet implemented
              curproc_setas(as_old);
              as_destroy(as);

              for(int i=args_num; i>=0; i--){
                kfree(args_kernel[i]);
              }
              kfree(args_kernel);
              return exception;
        }

        //9. copy the args into the new address space
        vaddr_t ptr[args_num+1];

        for(int j = args_num-1; j>=0; j--){
          int len = strlen(args_kernel[j]) + 1;
          stackptr = stackptr - ROUNDUP(len, 8);
          exception = copyoutstr(args_kernel[j], (userptr_t) stackptr, ROUNDUP(len, 8), NULL);
          if(exception){
            //as_deactivate();//yet implemented
              curproc_setas(as_old);
              as_destroy(as);

              for(int i=args_num; i>=0; i--){
                kfree(args_kernel[i]);
              }
              kfree(args_kernel);
              return exception;
          }
          ptr[j] = stackptr;
        }
        ptr[args_num] = (vaddr_t) NULL;

        //10. copy the args's ptr into the new address space
        for(int j = args_num; j>=0; j--){
          stackptr = stackptr - ROUNDUP(sizeof(vaddr_t), 4);
          exception = copyout((void *) &ptr[j], (userptr_t) stackptr, ROUNDUP(sizeof(vaddr_t), 4));
           if(exception){
            //as_deactivate();//yet implemented
              curproc_setas(as_old);
              as_destroy(as);

              for(int i=args_num; i>=0; i--){
                kfree(args_kernel[i]);
              }
              kfree(args_kernel);
              return exception;
          }
        }

        //we force the used stack amount to be multiplier of 8
        vaddr_t args_ptr = stackptr;
        stackptr = stackptr - (8 - ((USERSTACK - stackptr) % 8));

        //11. Delete old address space and unneed ptrs
        as_destroy(as_old);

        for(int i=args_num; i>=0; i--){
          kfree(args_kernel[i]);
        }
        kfree(args_kernel);

        //lock_release(lk);
        //12 enter new process
        enter_new_process(args_num, (userptr_t) args_ptr,
                stackptr, entrypoint);

        panic("enter_new_process failed");
        return EINVAL;
  }




#else

void sys__exit(int exitcode) {
  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;
  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);
  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);
  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);
  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);

  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}
/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;
  return(0);
}
/* stub handler for waitpid() system call                */
int
sys_waitpid(pid_t pid,
            userptr_t status,
            int options,
            pid_t *retval)
{
  int exitstatus;
  int result;
  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.
     Fix this!
  */
  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}
#endif /* OPT_A2 */



