#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
  printf("I am going to resume a process YAY!\n");

  char *fname;
  int rv = 0;

  if(argc != 2){
    printf("FAILED! Please enter a file name\n");
    exit(1);
  }

  fname = argv[1];

  rv = resume(fname);

  if(rv < 0){
    printf("Resume failed\n");
  }
  else {
    printf("Resume succeded!\n");
  }

  exit(0);
}