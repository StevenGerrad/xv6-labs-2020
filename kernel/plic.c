#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void
plicinit(void)
{
  // set desired IRQ priorities non-zero (otherwise disabled).
  // 强制转换为32位整数指针，因为PLIC寄存器是32位的。将1写入寄存器UART0_IRQ
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;   // 使能了UART的中断（PLIC是路由中断的，这个程序中PLIC可以接受这些中断）
  *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1; // 设置PLIC接收来自IO磁盘的中断
}

// plicinit是由0号CPU运行，之后，每个CPU的核都需要调用plicinithart函数表明对于哪些外设中断感兴趣
void
plicinithart(void)
{
  int hart = cpuid();
  
  // set uart's enable bit for this hart's S-mode. 
  *(uint32*)PLIC_SENABLE(hart)= (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);  // 每个CPU的核都表明自己对来自于UART和VIRTIO的中断感兴趣

  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_SPRIORITY(hart) = 0;   // 忽略中断的优先级，先将优先级设置为0
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
  int hart = cpuid();
  int irq = *(uint32*)PLIC_SCLAIM(hart);
  return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
  int hart = cpuid();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}
