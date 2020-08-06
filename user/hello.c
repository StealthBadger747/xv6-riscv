#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("Hello xv6!\n");

  int i = 0;
  char *label = 0;

  if(argc == 2)
    label = argv[1];
  
  mknod("../../console33333", 1, 10);
  mknod("../console5555", 1, 11);
  mkdir("../monstermash");
  mkdir("../../funkymash");

  while(1 == 2){
    if(label)
      printf("[%s] i = %d\n", label, i);
    else
      printf("i = %d\n", i);
  
    sleep(20);
    i++;
  }

  exit(0);
}