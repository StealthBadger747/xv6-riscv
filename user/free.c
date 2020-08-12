/* free.c - show free memory */

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
  struct mem_info mem;
  if(freemem(&mem) < 0) {
    printf("Memory checking failed!\n");
    exit(-1);
  }
  printf("Used memory:  '%d' Pages\n", mem.mem_usage);
  printf("Free memory:  '%d' Pages\n", mem.mem_limit);
  
  exit(0);
}
