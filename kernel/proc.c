#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct container containers[NCONT];
uint active_cont = 0;

struct proc proc[NPROC];
struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void wakeup1(struct proc *chan);

extern char trampoline[]; // trampoline.S

void
procinit(void)
{
  struct proc *p;
  struct container *c;

  initlock(&pid_lock, "nextpid");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");

      // Allocate a page for the process's kernel stack.
      // Map it high in memory, followed by an invalid
      // guard page.
      char *pa = kalloc();
      if(pa == 0)
        panic("kalloc");
      uint64 va = KSTACK((int) (p - proc));
      kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
      p->kstack = va;
  }

  int i = 48; // Starts at '0' in ASCII
  char lock_name[] = "cont_0";
  for(c = containers; c < &containers[NCONT]; c++) {
    c->cont_id = i - 48;
    lock_name[4] = i++;
    initlock(&c->lock, lock_name);
  }

  // Assign the first container to root privileges.
  containers[0].privilege_level = 0;
  strncpy(containers[0].name, "root container", 32);
  containers[0].state = RUNNABLE;
  containers[0].rootdir = namei("/");
  containers[0].proc_limit = NPROC;
  containers[0].proc_count = 1;

  kvminithart();
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

// Return the current struct container *, or zero if none.
struct container*
mycont(void)
{
  struct proc *p = myproc();
  if(p == 0)
    return 0;
  struct container *c = &containers[p->cont_id];
  return c;
}

// Return the root container
struct container*
rootcont(void)
{
  return &containers[0];
}

int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();

  // Allocate a trapframe page.
  if((p->tf = (struct trapframe *)kalloc()) == 0){
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0) {
    kfree(p->tf);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof p->context);
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  // Set default value for traceon flag
  p->traceon = 0;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  //struct container *c;
  //printf("FREEEEEEE!!!  '%s'\n", p->name);

  if(p->tf) {
    kfree((void*)p->tf);
  }
  p->tf = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  p->cont_id = 0;
}

// Create a page table for a given process,
// with no user pages, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0) {
    printf("proc_pagetable failed to allocate!\n");
    return 0;
  }

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  mappages(pagetable, TRAMPOLINE, PGSIZE,
           (uint64)trampoline, PTE_R | PTE_X);

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  mappages(pagetable, TRAPFRAME, PGSIZE,
           (uint64)(p->tf), PTE_R | PTE_W);

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, PGSIZE, 0);
  uvmunmap(pagetable, TRAPFRAME, PGSIZE, 0);
  if(sz > 0)
    uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x05, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x05, 0x02,
  0x9d, 0x48, 0x73, 0x00, 0x00, 0x00, 0x89, 0x48,
  0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0xbf, 0xff,
  0x2f, 0x69, 0x6e, 0x69, 0x74, 0x00, 0x00, 0x01,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->tf->epc = 0;      // user program counter
  p->tf->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();
  struct container *c;

  // Check if it will be over process limit
  if((c = &containers[p->cont_id]) != 0) {
    acquire(&c->lock);
    //printf("Limit: '%d', Count: '%d'\n", c->proc_limit, c->proc_count);
    if(c->privilege_level != 0 && c->proc_limit <= c->proc_count) {
      release(&c->lock);
      printf("Process limit reached!\n");
      procdump();
      return -1;
    }
    release(&c->lock);
  }

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  np->parent = p;

  // copy saved user registers.
  *(np->tf) = *(p->tf);

  // Cause fork to return 0 in the child.
  np->tf->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  np->state = RUNNABLE;

  // Set related container id
  np->cont_id = p->cont_id;

  // Increment process counter for the container
  if(c != 0) {
    acquire(&c->lock);
    c->proc_count++;
    release(&c->lock);
  }

  release(&np->lock);

  return pid;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
cfork(void) {
  struct container *c;
  int pid = fork();

  c = &containers[proc[pid].cont_id];
  acquire(&c->lock);
  c->proc_count--;
  release(&c->lock);
  return fork();
}

// Pass p's abandoned children to init.
// Caller must hold p->lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    // this code uses pp->parent without holding pp->lock.
    // acquiring the lock first could cause a deadlock
    // if pp or a child of pp were also in exit()
    // and about to try to lock p.
    if(pp->parent == p){
      // pp->parent can't change between the check and the acquire()
      // because only the parent changes it, and we're the parent.
      acquire(&pp->lock);
      pp->parent = initproc;
      // we should wake up init here, but that would require
      // initproc->lock, which would be a deadlock, since we hold
      // the lock on one of init's children (pp). this is why
      // exit() always wakes init (before acquiring any locks).
      release(&pp->lock);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  // we might re-parent a child to init. we can't be precise about
  // waking up init, since we can't acquire its lock once we've
  // acquired any other proc lock. so wake up init whether that's
  // necessary or not. init may miss this wakeup, but that seems
  // harmless.
  acquire(&initproc->lock);
  wakeup1(initproc);
  release(&initproc->lock);

  // grab a copy of p->parent, to ensure that we unlock the same
  // parent we locked. in case our parent gives us away to init while
  // we're waiting for the parent lock. we may then race with an
  // exiting parent, but the result will be a harmless spurious wakeup
  // to a dead or wrong process; proc structs are never re-allocated
  // as anything else.
  acquire(&p->lock);
  struct proc *original_parent = p->parent;
  release(&p->lock);
  
  // we need the parent's lock in order to wake it up from wait().
  // the parent-then-child rule says we have to lock it first.
  acquire(&original_parent->lock);

  acquire(&p->lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup1(original_parent);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&original_parent->lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  // hold p->lock for the whole time to avoid lost
  // wakeups from a child's exit().
  acquire(&p->lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      // this code uses np->parent without holding np->lock.
      // acquiring the lock first would cause a deadlock,
      // since np might be an ancestor, and we already hold p->lock.
      if(np->parent == p){
        // np->parent can't change between the check and the acquire()
        // because only the parent changes it, and we're the parent.
        acquire(&np->lock);
        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&p->lock);
            return -1;
          }
          // change container counter
          struct container *c = &containers[p->cont_id];
          acquire(&c->lock);
          if(c->proc_count > 0)
            c->proc_count--;
          release(&c->lock);
          freeproc(np);
          release(&np->lock);
          release(&p->lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&p->lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &p->lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct container *cont;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(cont = containers; cont < &containers[NCONT]; cont++) {
      acquire(&cont->lock);
      if(cont->state != RUNNABLE) {
        release(&cont->lock);
        continue;
      }
      release(&cont->lock);
      for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->cont_id == cont->cont_id && p->state == RUNNABLE && cont->state == RUNNABLE) {
          acquire(&cont->lock);
          cont->tokens++;
          release(&cont->lock);
          // Switch to chosen process.  It is the process's job
          // to release its lock and then reacquire it
          // before jumping back to us.
          p->state = RUNNING;
          c->proc = p;
          swtch(&c->scheduler, &p->context);

          // Check for suspended process
          if(p->suspended)
            exit(-1);

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
        }
        release(&p->lock);
      }
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.
  if(lk != &p->lock){  //DOC: sleeplock0
    acquire(&p->lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &p->lock){
    release(&p->lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
    }
    release(&p->lock);
  }
}

// Wake up p if it is sleeping in wait(); used by exit().
// Caller must hold p->lock.
static void
wakeup1(struct proc *p)
{
  if(!holding(&p->lock))
    panic("wakeup1");
  if(p->chan == p && p->state == SLEEPING) {
    p->state = RUNNABLE;
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;
  int this_cont_id;
  this_cont_id = myproc()->cont_id;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid && (p->cont_id == this_cont_id || this_cont_id == 0)){
      p->killed = 1;
      if(p->state == SLEEPING || p->state == SUSPENDED){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [SUSPENDED] "suspended"
  };
  struct proc *p;
  struct container *c;
  char *state;

  printf("\n-------------------  PROC DUMP  -------------------\n");
  printf("PID\tSTATE\tNAMES\tCONTAINER\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    c = &containers[p->cont_id];
    printf("%d\t%s\t%s\t'%s'\t", p->pid, state, p->name, c->name);
    printf("\n");
  }
  printf("---------------------------------------------------\n");
  printf("---------------- CONTAINER USEAGES ----------------\n");
  printf("NAMES");
  for(int i = 0; i < sizeof(containers[0].name) - 3; i++)
    printf(" ");
  printf("\tMEM(KB)\tDISK\t\tPROCS\tTOKENS\n");
  for(c = containers; c < &containers[NCONT]; c++) {
    //acquire(&c->lock);
    if(c->state == RUNNABLE || c->state == RUNNING) {
      printf("'%s'", c->name);
      for(int i = 0; i < sizeof(c->name) - strlen(c->name) + 2; i++)
        printf(" ");
      printf("%d\t%d\t\t%d\t%d\n", (c->mem_usage * PGSIZE) / 1024, c->disk_usage * 1024,
                                    c->proc_count, c->tokens);
    }
    //release(&c->lock);
  }
  printf("---------------------------------------------------\n");

}

int
numprocs(void)
{
  struct proc *p;
  int count = 0;

  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state != UNUSED) {
		  count += 1;
	  }
  }
  return count;
}

void
psget(struct p_table *pt)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [SUSPENDED] "suspended"
  };

  struct proc *p;
  pt->p_count = 0;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->state != UNUSED){
      struct p_info *entry = &pt->table[pt->p_count];
      // Copy PID
      entry->pid = p->pid;
      // Copy Memory
      entry->sz = p->sz;
      // Copy Process Name
      safestrcpy(entry->name_ps, p->name, sizeof(p->name) + 1);
      // Copy Container Name
      safestrcpy(entry->name_cont, mycont()->name, sizeof(mycont()->name) + 1);
      // Copy State
      safestrcpy(entry->state, states[p->state], sizeof(p->state) + 1);
      // Increment count
      pt->p_count++;
	  }
    release(&p->lock);
  }
}

int
ksuspend(int pid, struct file *file)
{
  printf("ksuspend('%d', '...') called\n", pid);

  int found = 0;
  struct proc *p_iter;
  struct proc *p;
  for(p_iter = proc; p_iter < &proc[NPROC]; p_iter++){
    acquire(&p_iter->lock);
    if(p_iter->pid == pid){
      p_iter->state = SUSPENDED;
      p = p_iter;
      found = 1;
    }
    release(&p_iter->lock);
  }

  if(!found){
    printf("NOT FOUND!!");
    return -1;
  }
  else {
    // Populate struct with information needed to resume
    struct suspended_hdr suspension;
    suspension.mem_sz = p->sz;
    suspension.code_sz = p->sz - (2 * PGSIZE);
    suspension.stack_sz = PGSIZE;
    suspension.traceon = p->traceon;
    strncpy(suspension.name, p->name, 16);
    
    // Store current page table to swap out to new one to write stuff
    pagetable_t old_table = myproc()->pagetable;  // The current table
    myproc()->pagetable = p->pagetable;           // The pagetable of the suspended program

    // Write from kernel
    kernelfilewrite(file, (uint64) &suspension, sizeof(suspension));
    kernelfilewrite(file, (uint64) p->tf, sizeof(struct trapframe));

    // Write normal
    filewrite(file, (uint64) 0, suspension.code_sz);
    filewrite(file, (uint64) (suspension.code_sz + PGSIZE), PGSIZE);

    // Restore pagetable
    myproc()->pagetable = old_table;
  }

  return 0;
}

// Attempts to find a free container and return the index.
// Returns -1 if it can't find anything.
int
alloc_cont(void)
{
  struct container *c = &containers[1];
  int index = 1;
  for(; c <= &containers[NCONT]; c++) {
    acquire(&c->lock);
    if(c->state == EMPTY || c->state == CREATED) {
      // Reserve the container.
      c->state = RUNNABLE;
      release(&c->lock);
      return index;
    }
    release(&c->lock);
    index++;
  }
  return -1;
}

int
cstart(int vc_fd, char *name, char *root_path, int maxproc, int maxmem, int maxdisk)
{
  struct container *cont;
  struct container *p_cont;
  struct proc *p;
  struct inode *ip;
  int cont_index;
  char temp_path[MAXPATH];

  if(root_path[0] != '/') {
    temp_path[0] = '/';
    safestrcpy(temp_path + 1, root_path, MAXPATH);
  } else {
    safestrcpy(temp_path, root_path, MAXPATH);
  }
  int len = strlen(temp_path);
  if(temp_path[len - 1] != '/') {
    temp_path[len] = '/';
    temp_path[len + 1] = '\0';
  }

  ip = namei(temp_path);
  if(ip == 0) {
    printf("Invalid root path!\n");
    return -1;
  }
  
  cont_index = alloc_cont();
  if(cont_index < 1)
    return cont_index;

  p = myproc();

  p->cont_id = cont_index;

  // Set the name of the container and set the process counter to 1.
  cont = mycont();
  acquire(&cont->lock);
  strncpy(cont->name, name, sizeof(cont->name));
  cont->proc_count = 1;
  cont->state = RUNNABLE;

  // Cleanup process counter from parent as well as subracting 1 page of memory
  // and adding that memory usage to here.
  p_cont = &containers[p->parent->cont_id];
  acquire(&p_cont->lock);
  p_cont->proc_count--;
  p_cont->mem_usage -= 9;
  release(&p_cont->lock);

  cont->mem_usage += 9;
  cont->proc_limit = maxproc;
  cont->mem_limit = maxmem;
  cont->disk_limit = maxdisk;
  cont->privilege_level = 1;
  safestrcpy(cont->rootdir_str, temp_path, MAXPATH);
  
  release(&cont->lock);

  begin_op();
  cont->rootdir = idup(ip);
  iput(p->cwd);
  end_op();
  p->cwd = ip;

  return 0;
}

struct container *
find_container(char *cname)
{
  struct container *c;
  for(c = &containers[0]; c < &containers[NCONT]; c++) {
    acquire(&c->lock);
    if(strncmp(c->name, cname, sizeof(c->name)) == 0) {
      release(&c->lock);
      return c;
    }
    release(&c->lock);
  }

  return 0;
}

int
cpause(char *cname)
{
  struct container *c = find_container(cname);
  if(c == 0)
    return -1;

  acquire(&c->lock);
  c->state = SUSPENDED;
  release(&c->lock);

  return 0;
}

int
cresume(char *cname)
{
  struct container *c = find_container(cname);
  if(c == 0 || c->state == RUNNABLE || c->state == RUNNABLE)
    return -1;

  acquire(&c->lock);
  c->state = RUNNABLE;
  release(&c->lock);

  return 0;
}

int
cstop(char *cname)
{
  struct proc *p = &proc[0];
  struct container *c = find_container(cname);
  if(c == 0)
    return -1;

  for(; p < &proc[NPROC]; p++) {
    if(p->cont_id == c->cont_id) {
      kill(p->pid);
      yield();
    }
  }

  acquire(&c->lock);

  *c->name = '\0';
  c->proc_count = 0;
  c->state = EMPTY;
  c->mem_usage = 0;
  c->proc_limit = 0;
  c->mem_limit = 0;
  c->disk_limit = 0;
  *c->rootdir_str = '\0';
  c->rootdir = 0;
  
  release(&c->lock);

  return 0;
}