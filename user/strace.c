#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    printf("Welcome to strace!\n");

    if(argc < 2){
        printf("No program specified!\n");
        exit(1);
    }

    int pid = fork();

    // Child
    if(pid == 0){
        traceon();
        exec(argv[1], &argv[1]);
        exit(0);
    }
    else if(pid < 0){
        printf("strace failed!\n");
    }
    // Parent
    else {
        wait(&pid);
    }

    printf("Done!\n");

    exit(0);
}