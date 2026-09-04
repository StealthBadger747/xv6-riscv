#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;
extern char _bss_start[], _bss_end[];

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
#ifndef QEMU
  // U-Boot loads the raw binary and does not initialize ELF NOBITS
  // sections. Clear BSS before consulting any kernel globals.
  memset(_bss_start, 0, _bss_end - _bss_start);
#endif
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector
 #ifndef QEMU
    // Do not enable S-mode interrupts in start(): a pending OpenSBI timer
    // could otherwise vector through U-Boot before stvec is installed.
    // Select interrupt sources once; intr_on()/intr_off() only control
    // the global SSTATUS.SIE gate.
    w_sie(SIE_SEIE | SIE_STIE);
 #endif
    plicinit();      // set up interrupt controller
    plicinithart();  // ask PLIC for device interrupts
    binit();         // buffer cache
    iinit();         // inode cache
    fileinit();      // file table
#ifdef QEMU
    virtio_disk_init(); // emulated hard disk
#else
    ramdiskinit();   // format and populate the volatile RAM disk
#endif
    userinit();      // first user process
#ifndef QEMU
    // Trap handling is installed. Arm the first 10 ms supervisor timer
    // deadline; devintr() re-arms each subsequent tick.
    sbi_set_timer(r_time() + 250000);
#endif
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler();        
}
