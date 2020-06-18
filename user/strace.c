#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
    printf("Welcome to strace!\n");

    if(argc < 2) {
        printf("No program specified!\n");
        exit(1);
    }

    int pid = fork();

    // Child
    if(pid == 0) {
        traceon();
        exit(0);
    }
    // Parent
    else {
        wait(&pid);
    }

    printf("Done!\n");

    exit(0);
}