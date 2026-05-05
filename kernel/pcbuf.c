// Shared bounded buffer for producer/consumer demo (spinlock + sleep/wakeup).

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define PCBUFSZ 16

static struct spinlock pclock;
static int pcbuf[PCBUFSZ];
static int wi, ri, count;

void
pcinit(void)
{
  initlock(&pclock, "pcbuf");
  wi = ri = count = 0;
}

uint64
sys_pcput(void)
{
  int v;

  argint(0, &v);
  acquire(&pclock);
  while(count == PCBUFSZ)
    sleep(&count, &pclock);
  pcbuf[wi] = v;
  wi = (wi + 1) % PCBUFSZ;
  count++;
  wakeup(&count);
  release(&pclock);
  return 0;
}

uint64
sys_pcget(void)
{
  int v;

  acquire(&pclock);
  while(count == 0)
    sleep(&count, &pclock);
  v = pcbuf[ri];
  ri = (ri + 1) % PCBUFSZ;
  count--;
  wakeup(&count);
  release(&pclock);
  return (uint64)(uint)v;
}
