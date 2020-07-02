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