/* schedtest.c - Test the scheduling */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "kernel/fs.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  uint start_ticks = getticks();

  while(1) {
      printf("Ticks ran: '%d'\n", getticks() - start_ticks);
  }
  
  exit(0);
}
