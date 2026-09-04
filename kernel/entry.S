#ifdef QEMU
	# qemu -kernel starts at 0x1000. the instructions
        # there seem to be provided by qemu, as if it
        # were a ROM. the code at 0x1000 jumps to
        # 0x80000000, the _start function here,
        # in machine mode. each CPU starts here.
#else
	# OpenSBI (via U-Boot `go`) jumps here in supervisor mode with
	# a0 = hartid and a1 = device tree pointer. The Duo runs a single
	# C906 hart; mhartid must not be read (it traps in S-mode).
#endif
.section .data
.globl stack0
.section .text
.globl start
.section .text
.globl _entry
_entry:
	# set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + ((hartid + 1) * 4096)
        la sp, stack0
        li a2, 1024*4
#ifdef QEMU
        csrr a1, mhartid
        addi a1, a1, 1
#else
        addi a1, a0, 1
#endif
        mul a2, a2, a1
        add sp, sp, a2
	# jump to start() in start.c
        call start
junk:
        j junk
