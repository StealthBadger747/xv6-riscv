// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "sh", 0 };

void
create_vcs(void)
{
  int i, fd;
  char *dname = "vc0";

  for(i = 1; i < MAXCONS; i++) {
    dname[2] = '0' + i;
    if((fd = open(dname, O_RDWR)) < 0){
      mknod(dname, 1 + i, 1 + i);
    } else {
      close(fd);
    }
  }
}

int
main(void)
{
  int pid, wpid, i;

  // if(open("console", O_RDWR) < 0){
  //   mknod("console", 1, 0);
  //   open("console", O_RDWR);
  // }
  if(open("vc0", O_RDWR) < 0){
    mknod("vc0", 1, 0);
    open("vc0", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  printf("CREATING VCs\n");
  create_vcs();

  char *dname = "vc0";

  for(;;){
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      for(i = 1; i < MAXCONS; i++) {
        printf("DNAME: %s\n", dname);
        dname[2] = '0' + i;
        int fd = open(dname, O_RDWR);
        printf("FD: %d\n", fd);
        dup(0);  // stdout
        dup(0);  // stderr
        pid = fork();
        if(pid < 0){
          printf("init: fork failed\n");
          exit(1);
        }
        if(pid == 0){
          exec("sh", argv);
          printf("init: exec sh failed\n");
          exit(1);
        }
      }
    }
    //printf("SOME FORK HAPPENDE HERE!, pid: %d\n", pid);
    while((wpid=wait(0)) >= 0 && wpid != pid){
      printf("zombie!\n");
    }
    printf("AFTER WHILE!\n");
  }

  // while((wpid=wait(0)) >= 0 && wpid != pid){
  //   printf("zombie!\n");
  // }
}
