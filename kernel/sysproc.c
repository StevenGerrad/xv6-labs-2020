#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  backtrace();  // lab traps

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 
sys_sigalarm(void)
{
  int ticks;
  uint64 func;
  if(argint(0, &ticks) < 0 || argaddr(1, &func) < 0)
    return -1;

  struct proc *p = myproc();
  // if(p->alarm_hascalled == 1) return -1;
  p->alarm_ticks = ticks;
  p->alarm_func = (handler)func;

  // 时钟什么的根本就不懂啊——首先要重新过一遍LEC 6
  // 注意提示中的一点——本来就有每隔1s的时钟中断
  p->alarm_last = 0;
  // if(ticks != 0)
  //   p->alarm_hascalled = 1;
  // XXX: p->alarm_hascalled 在这里不管

  return 0;
}

// 处理alarm后恢复内核进程
uint64 
sys_sigreturn(void)
{
  // 将 trapframe 恢复到时钟中断之前的状态，恢复原本正在执行的程序流
  // TODO: 为什么在这里恢复trapframe就能恢复了？
  struct proc *p = myproc();
  *p->trapframe = *p->alarm_trapframe;
  p->alarm_hascalled = 0;
  return 0;
}