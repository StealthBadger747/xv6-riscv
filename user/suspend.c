#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Add p_info struct for storing the information of a process in the p_table struct
struct p_info {
  int pid;
  uint64 sz;
  char state[16];
  char name[16];
};

// Add p_table struct for the ps command to use
struct p_table {
  struct p_info table[64];    // A table of p_info, aka active processes
  int p_count;                // Keeps track of the number of entries in table
};

void ps_local() {
    printf("PID\tMEM\tSTATE\tNAME\n");
    struct p_table pt;
    psget(&pt);
    for(int i = 0; i < pt.p_count; i++){
        printf("%d\t%dK\t%s\t%s\n", pt.table[i].pid, pt.table[i].sz / 1024, pt.table[i].state, pt.table[i].name);
    }
}
    

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

    ps_local();

    return rv;
}