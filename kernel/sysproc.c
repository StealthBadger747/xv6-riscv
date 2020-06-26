#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  struct proc *curr_p = myproc();
  if(curr_p->traceon == 1)
    printf("[%d] sys_sleep(%d)\n", curr_p->pid, n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);

  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_numprocs(void)
{
  return numprocs();
}

uint64
sys_traceon(void)
{
  myproc()->traceon = 1;
  return 0;
}

uint64
sys_psget(void)
{
  uint64 ps_info_addr;
  if(argaddr(0, &ps_info_addr) < 0)
    return -1;
  
  struct p_table pt;
  psget(&pt);

  struct proc *p = myproc();
  acquire(&p->lock);
  int status = copyout(p->pagetable, ps_info_addr, (char *) &pt, sizeof(pt) + 1);
  release(&p->lock);
  return status;
}

uint64
sys_suspend(void)
{
  int pid;
  char fname[64];
  int rv;

  if(argint(0, &pid) < 0)
    return -1;

  if(argstr(1, fname, 64) < 0)
    return -1;

  rv = ksuspend(pid, fname)
  //suspend(pid);
  return rv;
}

uint64
sys_resume(void)
{
  int pid;
  if(argint(0, &pid) < 0)
    return -1;
  resume(pid);
  return 0;
}