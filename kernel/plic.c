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
#ifndef QEMU
  // cvitek quirk: the PLIC has a global enable register that must
  // be set to 1 before any interrupt routing works.
  *(uint32*)PLIC_CTRL = 1;
#endif

  // set desired IRQ priorities non-zero (otherwise disabled).
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;
#ifdef QEMU
  *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
#endif
}

void
plicinithart(void)
{
  int hart = cpuid();

  // set the uart's enable bit for this hart's S-mode.
  // (word 0 on qemu, word 1 on the Duo where UART0_IRQ is 44)
  *(uint32*)(PLIC_SENABLE(hart) + (UART0_IRQ/32)*4) = (1 << (UART0_IRQ%32));
#ifdef QEMU
  *(uint32*)(PLIC_SENABLE(hart) + (VIRTIO0_IRQ/32)*4) |= (1 << (VIRTIO0_IRQ%32));
#endif

  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_SPRIORITY(hart) = 0;
}

// return a bitmap of which IRQs are waiting
// to be served.
uint64
plic_pending(void)
{
  uint64 mask;

  //mask = *(uint32*)(PLIC + 0x1000);
  //mask |= (uint64)*(uint32*)(PLIC + 0x1004) << 32;
  mask = *(uint64*)PLIC_PENDING;

  return mask;
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
  int hart = cpuid();
  //int irq = *(uint32*)(PLIC + 0x201004);
  int irq = *(uint32*)PLIC_SCLAIM(hart);
  return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
  int hart = cpuid();
  //*(uint32*)(PLIC + 0x201004) = irq;
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}
