/* ctool.c - A manager for all things containers */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/fs.h"
#include "user/user.h"

enum ctool_args { CTOOL, SUBCMD };
enum start_args { BL0, BL1, VC_FILE_NAME, ROOT_DIR, NAME, MAX_PROC, MAX_MEM, 
                  MAX_DISK, CMD, ARG0, ARG1, ARG2, ARG3, ARG4 };
enum creat_args { BL2, BL3, CONT_DIR, PROG_0, PROG_1, PROG_2, PROG_3, PROG_4 };

int cp(char* source, char* destination)
{
  printf("Source: '%s'\n", source);
  printf("Destination: '%s'\n", destination);
  int src_file, dst_file, n;
  char buf[512];

  if((src_file = open(source, O_RDONLY)) < 0) {
    printf("Can't open the source file!\n");
    return -1;
  }

  if((dst_file = open(destination, O_RDWR|O_CREATE)) < 0) {
    printf("Can't open or create the destination file!\n");
    return -1;
  }
  
  while((n = read(src_file, buf, sizeof(buf))) > 0) {
    write(dst_file, buf, n);
  }

  close(dst_file);
  close(src_file);

  return 0;
}

int
create(int argc, char **argv)
{
  /*  ctool create cont sh echo mkdir rm ls cat  */
  /*  ctool start vc1 /cont foo_cont sh          */

  if(argc < PROG_0) {
    printf("Incomplete argument set.\n");
    printf("Usage: ctool create <container directory e.g. '/c1/root/'> <programs> <...>\n");
    return -1;
  }

  // Not capable of creating new nested directories. Only one directory at a time.
  mkdir(argv[CONT_DIR]);

  for(int i = PROG_0; i < argc; i++) {
    // Build Source Path
    char src_path[DIRSIZ + 6] = { 0 };
    src_path[0] = '/';
    src_path[1] = '\0';
    strcpy(src_path + 1, argv[i]); //strncpy(src_path + 1, argv[i], DIRSIZ + 1);

    // Build destination path
    char dst_path[MAXPATH] = { 0 };
    dst_path[0] = '/';
    dst_path[1] = '\0';
    strcpy(dst_path + 1, argv[CONT_DIR]);  //strncpy(dst_path, argv[i], MAXPATH - DIRSIZ);
    uint base_len = strlen(dst_path); // strnlen(dst_path, MAXPATH - DIRSIZ);
    if(dst_path[base_len - 1] != '/') {
      dst_path[base_len++] = '/';
      dst_path[base_len] = '\0';
    }
    strcpy((dst_path + base_len), argv[i]); // strncpy((dst_path + base_len), argv[i], DIRSIZ);

    if(cp(src_path, dst_path) < 0) {
      printf("error, exiting!\n");
      return -1;
    }
  }

  return 0;
}

int
start(int argc, char **argv)
{
  int fd, id;
  int maxproc, maxmem, maxdisk;

  fd = open(argv[VC_FILE_NAME], O_RDWR);
  printf("fd = %d\n", fd);
  close(fd);

  maxproc = atoi(argv[MAX_PROC]);
  maxmem = atoi(argv[MAX_MEM]);
  maxdisk = atoi(argv[MAX_DISK]);

  printf("'%d' '%d' '%d'\n", maxproc, maxmem, maxdisk);

  if(maxproc < 1 || maxdisk < 10) {
    printf("Invalid values!\n");
    return -1;
  }

  if(maxmem < 50) {
    printf("Memory needs to be greater than 50 for the container to work!\n");
    return -1;
  }

  /* fork a child and exec argv[1] */
  id = fork();

  if(id == 0) {
    close(0);
    close(1);
    close(2);
    dup(fd);
    dup(fd);
    dup(fd);
    if(cstart(fd, argv[NAME], argv[ROOT_DIR], maxproc, maxmem, maxdisk) < 0) {
      printf("FAILLLLL!\n");
      goto cstart_fail;
    }

    printf("%s started on %s\n", argv[CMD], argv[VC_FILE_NAME]);
    
    printf("Exec() status: '%d'\n", exec(argv[CMD], &argv[CMD]));

    exit(0);

cstart_fail:
    printf("start() failed!\n");
    exit(-1);
  }

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
  printf("subcommand: '%s'\n", argv[SUBCMD]);
  //printf("%d\n", strcmp(argv[SUBCMD], "start"));

  if(strcmp(argv[SUBCMD], "start") == 0)
    start(argc, argv);
  else if(strcmp(argv[SUBCMD], "create") == 0)
    create(argc, argv);
  else
    printf("Invalid subcommand!\n Exiting...\n");

  exit(0);
}
