/* ctool.c - A manager for all things containers */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
user_cstart(int argc, char **argv)
{
  int fd, id;

  fd = open(argv[2], O_RDWR);
  printf("fd = %d\n", fd);

  /* fork a child and exec argv[1] */
  id = fork();

  if(id == 0){
    close(0);
    close(1);
    close(2);
    dup(fd);
    dup(fd);
    dup(fd);
    cstart(fd, argv[4]);
    exec(argv[3], &argv[3]);
    exit(0);
  }

  // ctool cstart vc0 foo sh

  printf("%s started on %s\n", argv[4], argv[2]);

  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("Creating a new container process! :)\n");
  int fd;

  if(argc < 4) {
    printf("usage: ctool <subcommand> <vc> <name> <cmd> [<arg> ...]\n");
    exit(-1);
  }
  fd = open(argv[2], O_RDWR);
  printf("fd = %d\n", fd);
  printf("%s\n", argv[1]);
  printf("%d\n", strcmp(argv[1], "cstart"));

  if(strcmp(argv[1], "cstart") == 0)
    user_cstart(argc, argv);
  else
    printf("Invalid subcommand!\n Exiting...\n");

  exit(0);
}
