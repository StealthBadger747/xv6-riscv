#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  // Had trouble getting the information back to userspace.
  // It was working before I had containers, Alex and I can't figure out what went wrong.
  psget();
  exit(0);
}