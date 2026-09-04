// Physical memory layout
//
// Two targets, selected by -DQEMU (see Makefile):
//
//  QEMU: qemu-system-riscv64 -machine virt (original MIT xv6 layout,
//        M-mode entry at 0x80000000, virtio disk, CLINT timer).
//
//  Duo:  Milk-V Duo (CV1800B, 64 MB) and Duo 256M (SG2002, 256 MB).
//        Same peripheral map on both; booted via U-Boot `go` under
//        OpenSBI, landing at 0x80200000 in S-mode:
//
//        04140000 -- UART0 (DW APB UART, 32-bit registers)
//        70000000 -- PLIC
//        74000000 -- CLINT (timer stays in M-mode; we use SBI)
//        80200000 -- load address
//        81000000 -- PHYSTOP: kernel page allocator stops here (~14 MB)
//        81000000 -- RAMDISK: formatted and populated by ramdiskinit()
//                    (above PHYSTOP so kinit() never frees it)
//
//        PHYSTOP/RAMDISK are identical on both Duo variants; the extra
//        RAM of the 256M simply goes unused.

#ifdef QEMU
// qemu -machine virt, based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0
// 10001000 -- virtio disk
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here

// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L
#define UART0_IRQ 10

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

// local interrupt controller, which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// qemu puts programmable interrupt controller here.
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024)

#else
// Milk-V Duo / Duo 256M peripherals.
#define UART0 0x04140000L
#define UART0_IRQ 44

// local interrupt controller (timer stays in M-mode; armed via SBI).
#define CLINT 0x74000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // cycles since boot.

// platform-level interrupt controller.
#define PLIC 0x70000000L
#define PLIC_CTRL (PLIC + 0x1FFFFC) // cvitek: write 1 to enable the PLIC
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80200000 to PHYSTOP.
// Identical on Duo and Duo 256M.
#define KERNBASE 0x80200000L
#define PHYSTOP 0x81000000L
#define RAMDISK (PHYSTOP)
#define RAMDISK_MAX (FSSIZE*BSIZE)

#endif // QEMU

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->tf, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)
