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
    int rv;

    pid = atoi(argv[1]);

    int child = fork();
    if(child == 0) {
      printf("Child PID is: '%d'\n", getpid());
      char *sus = "SUSPENDEDSUSPENDEDSUSPENDED";
      suspend(getpid());
    }
    else {
      sleep(100);
    }

    ps_local();

    exit(0);
}