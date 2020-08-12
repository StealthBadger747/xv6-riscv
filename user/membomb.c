/* membomb.c - allocate memory, 4KB at a time until malloc fails. */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int totalpages = 0;
  char *p;

  printf("membomb: started\n");
  while(1) {
    p = (char *) malloc(4096);
    if (p == 0) {
      printf("membomb: malloc() failed, exiting\n");
      cinfo();
      exit(0);
    }
    totalpages++;

    printf("membomb: total memory allocated: %d KB\n", totalpages);
  }

  exit(0);
}
