#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
    printf("I am going to suspend a process YAY!\n");
    printf("Partent PID is: '%d'\n", getpid());

    int pid;
    char *fname;
    int rv = 0;
    int fd;

    if(argc != 3)
      exit(-1);

    pid = atoi(argv[1]);
    fname = argv[2];

    fd = open(fname, O_WRONLY | O_CREATE);
    rv = suspend(pid, fd);
    close(fd);

    if(rv < 0) {
      printf("Suspend failed\n");
      unlink(fname);
    }
    else {
      printf("Suspend succeded!\n");
    }

/*
    int child = fork();
    if(child == 0) {
      printf("Child PID is: '%d'\n", getpid());
      pid = getpid();
      rv = suspend(pid, fd);
    }
    else {
      sleep(100);
    }
*/
    //ps_local();
    kill(pid);
    //ps_local();

    exit(0);
}