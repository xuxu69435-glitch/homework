#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid, i, v;

  pid = fork();
  if(pid < 0){
    printf("pcdemo: fork failed\n");
    exit(1);
  }

  if(pid == 0){
    for(i = 0; i < 8; i++){
      v = pcget();
      printf("consumer: got %d\n", v);
    }
    exit(0);
  }

  for(i = 1; i <= 8; i++){
    if(pcput(i) < 0)
      printf("producer: pcput failed\n");
  }

  wait(0);
  printf("pcdemo: done\n");
  exit(0);
}
