/* ctool.c - A manager for all things containers */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("Creating a new container process! :)\n");
  int fd;

  if(argc < 3) {
    printf("usage: ctool cstart <name> <cmd> [<arg> ...]\n");
    exit(-1);
  }
  fd = open(argv[2], O_RDWR);
  printf("fd = %d\n", fd);

  cstart(fd, "foo");

  exit(0);
}
