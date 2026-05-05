#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  int p1 = getpid_plus();

  printf("getpid=%d getpid_plus=%d (expect plus=%d)\n", pid, p1, pid + 1);
  exit(0);
}
