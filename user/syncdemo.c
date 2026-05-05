#include "kernel/types.h"
#include "user/user.h"

#define N 12

// 第二层任务 3：两进程竞争控制台输出——无同步 vs 管道“令牌”同步（用户态无法直接用 spinlock，用管道串行化临界区作对比）。

void
nosync(void)
{
  int pid, i;

  printf("\n--- 无同步：父子各打印一行短消息 %d 次（输出会交错）---\n", N);
  pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    return;
  }
  for(i = 0; i < N; i++){
    if(pid == 0)
      printf("C");
    else
      printf("P");
  }
  if(pid != 0){
    wait(0);
    printf("\n");
  } else {
    exit(0);
  }
}

void
withpipe(void)
{
  int pid, i;
  int p2c[2], c2p[2];
  char tok = 'x';

  printf("\n--- 有同步：双管道握手，每次仅一方打印整行---\n");
  if(pipe(p2c) < 0 || pipe(c2p) < 0){
    printf("pipe failed\n");
    return;
  }
  pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    return;
  }
  if(pid == 0){
    close(p2c[1]);
    close(c2p[0]);
    for(i = 0; i < N; i++){
      if(read(p2c[0], &tok, 1) != 1)
        break;
      printf("child line %d\n", i);
      if(write(c2p[1], &tok, 1) != 1)
        break;
    }
    close(p2c[0]);
    close(c2p[1]);
    exit(0);
  }
  close(p2c[0]);
  close(c2p[1]);
  for(i = 0; i < N; i++){
    if(write(p2c[1], &tok, 1) != 1)
      break;
    printf("parent line %d\n", i);
    if(read(c2p[0], &tok, 1) != 1)
      break;
  }
  close(p2c[1]);
  close(c2p[0]);
  wait(0);
}

int
main(int argc, char *argv[])
{
  nosync();
  withpipe();
  exit(0);
}
