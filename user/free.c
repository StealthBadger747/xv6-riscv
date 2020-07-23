/* free.c - show free memory */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int
main(int argc, char *argv[])
{

  printf("Inifinite free memory :)\n");

  exit(0);
}
