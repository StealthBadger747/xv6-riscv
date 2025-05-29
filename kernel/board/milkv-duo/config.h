#ifndef _BOARD_MILKV_DUO_CONFIG_H
#define _BOARD_MILKV_DUO_CONFIG_H

// Memory map
#define KERNBASE 0x80000000L
#ifdef CHIP_SG2002
#define PHYSTOP  (KERNBASE + 256*1024*1024)  // 256MB RAM for SG2002
#else
#define PHYSTOP  (KERNBASE + 64*1024*1024)   // 64MB RAM for CV1800B
#endif
// #define TRAMPOLINE (MAXVA - PGSIZE)
// #define TRAPFRAME (TRAMPOLINE - PGSIZE)

// UART
#define UART0 0x04140000L
#define UART0_IRQ 3

// PLIC (Platform Level Interrupt Controller)
#define PLIC_BASE 0x70000000L
#define PLIC_PRIORITY (PLIC_BASE + 0x0)
#define PLIC_PENDING (PLIC_BASE + 0x1000)
#define PLIC_MENABLE(hart) (PLIC_BASE + 0x2000 + (hart)*0x100)
#define PLIC_MTHRESHOLD(hart) (PLIC_BASE + 0x200000 + (hart)*0x1000)
#define PLIC_MCLAIM(hart) (PLIC_BASE + 0x200004 + (hart)*0x1000)
#define PLIC_MCOMPLETE(hart) (PLIC_BASE + 0x200004 + (hart)*0x1000)
#define PLIC_SENABLE(hart)    (PLIC_BASE + 0x2080 + (hart)*0x100)
#define PLIC_SPRIORITY(hart) (PLIC_BASE + 0x201000 + (hart)*0x2000)
#define PLIC_SCLAIM(hart)    (PLIC_BASE + 0x201004 + (hart)*0x2000)

// T-Head specific CSRs
#define CSR_MHCR 0x7c1
#define CSR_MCOR 0x7c2
#define CSR_MXSTATUS 0xbe9

// Cache operations
#define CACHE_INVALIDATE_ICACHE 0x11
#define CACHE_INVALIDATE_DCACHE 0x12
#define CACHE_ENABLE_ICACHE 0x1
#define CACHE_ENABLE_DCACHE 0x2

// Number of CPUs
#define NCPU 2

// CLINT (Core Local Interruptor)
#define CLINT 0x74000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// VirtIO (for compatibility; not present on all boards)
#define VIRTIO0 0x10001000L
#define VIRTIO0_IRQ 1

// PLIC base for compatibility
#define PLIC 0x70000000L

#endif // _BOARD_MILKV_DUO_CONFIG_H 
