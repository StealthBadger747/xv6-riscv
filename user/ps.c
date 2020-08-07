#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Add p_info struct for storing the information of a process in the p_table struct
struct p_info {
  int pid;
  uint64 sz;
  char state[16];                  // The state of the process
  char name_ps[16];                // The name of process
  char name_cont[32];              // The name of container
};

// Add p_table struct for the ps command to use
struct p_table {
  struct p_info table[64];    // A table of p_info, aka active processes
  int p_count;                // Keeps track of the number of entries in table
};

int
main(int argc, char *argv[])
{
  printf("PID\tMEM\tSTATE\tNAME\n");
  struct p_table pt;
  psget(&pt);
  for(int i = 0; i < pt.p_count; i++){
    //printf("%d\t%dK\t%s\t%s\n", pt.table[i].pid, pt.table[i].sz / 1024, pt.table[i].state, pt.table[i].name);
  }
    
  exit(0);
}