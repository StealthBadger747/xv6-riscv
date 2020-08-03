/* ctool.c - A manager for all things containers */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

enum command_args { CTOOL, SUBCMD, VC_FILE_NAME, NAME, CMD, ARG0, ARG1, ARG2, ARG3, ARG4 };

int
user_cstart(int argc, char **argv)
{
  int fd, id;

  fd = open(argv[VC_FILE_NAME], O_RDWR);
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
    if(cstart(fd, argv[3], 5, 6, 7) < 0)
      goto cstart_fail;

    printf("%s started on %s\n", argv[CMD], argv[VC_FILE_NAME]);
    
    if(exec(argv[CMD], &argv[CMD]) < 0)
      goto cstart_fail;

    exit(0);

  cstart_fail:
    printf("cstart() failed!\n");
    exit(-1);
  }

  // ctool cstart vc0 foo_cont sh

  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("Creating a new container process! :)\n");

  if(argc < 4) {
    printf("usage: ctool <subcommand> <vc> <name> <cmd> [<arg> ...]\n");
    exit(-1);
  }
  printf("%s\n", argv[SUBCMD]);
  printf("%d\n", strcmp(argv[SUBCMD], "cstart"));

  if(strcmp(argv[SUBCMD], "cstart") == 0)
    user_cstart(argc, argv);
  else
    printf("Invalid subcommand!\n Exiting...\n");

  exit(0);
}
