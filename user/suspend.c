#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
    printf("I am going to suspend a process YAY!\n");
    exit(0);
}