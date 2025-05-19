#include "asm.h"
#include "board/milkv-duo/config.h"

.section .text
.global _entry
_entry:
        # set up a stack for C.
        # stack0 is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = stack0 + (hartid * 4096)
        la sp, stack0
        li a0, 1024*4
        csrr a1, mhartid
        addi a1, a1, 1
        mul a0, a0, a1
        add sp, sp, a0

        # Set up mxstatus for T-Head
        li t0, 0xc0638000
        csrw CSR_MXSTATUS, t0

        # Enable PLIC
        li t0, PLIC_BASE
        addi t0, t0, 0x1FFFFC
        li t1, 1
        sw t1, 0(t0)

        # Enable I-cache
        li t0, CACHE_INVALIDATE_ICACHE
        csrc CSR_MCOR, t0
        csrs CSR_MCOR, t0
        li t0, CACHE_ENABLE_ICACHE
        csrs CSR_MHCR, t0

        # Enable D-cache
        li t0, CACHE_INVALIDATE_DCACHE
        csrc CSR_MCOR, t0
        csrs CSR_MCOR, t0
        li t0, CACHE_ENABLE_DCACHE
        csrs CSR_MHCR, t0

        # jump to start() in start.c
        call start

spin:
        j spin 
