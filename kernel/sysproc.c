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
sys_cstart(void)
{
  int fd, maxproc, maxmem, maxdisk;
  char name[32];
  char root_path[MAXPATH];

  if(argint(0, &fd) < 0)
    return -1;
  if(argstr(1, name, 32) < 0)
    return -1;
  if(argstr(2, root_path, 32) < 0)
    return -1;
  if(argint(3, &maxproc) < 0)
    return -1;
  if(argint(4, &maxmem) < 0)
    return -1;
  if(argint(5, &maxdisk) < 0)
    return -1;

  if(maxproc < 1 || maxdisk < 10) {
    printf("Invalid values!\n");
    return -1;
  }

  if(maxmem < 50) {
    printf("Memory needs to be greater than 50 for the container to work!\n");
    return -1;
  }

  return cstart(fd, name, root_path, maxproc, maxmem, maxdisk);
}