/* vctest.c - test virtual consoles */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int fd, id;

  if (argc < 3) {
    printf("usage: vctest <vc> <name> <cmd> [<arg> ...]\n");
    exit(-1);
  }

  fd = open(argv[1], O_RDWR);
  printf("fd = %d\n", fd);

  /* fork a child and exec argv[1] */
  id = fork();

  if (id == 0){
    close(0);
    close(1);
    close(2);
    dup(fd);
    dup(fd);
    dup(fd);
    exec(argv[2], &argv[2]);
    printf("COMMAND FAILED?: %s\n", argv[2]);
    exit(0);
  }

  printf("%s started on %s\n", argv[2], argv[1]);

  exit(0);
}
