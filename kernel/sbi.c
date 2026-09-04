#include "types.h"
#include "riscv.h"
#include "sbi.h"

// Legacy SBI v0.2 TIME extension: set the next S-mode timer
// interrupt for stime_value (mtime ticks, 25 MHz on the Duo).
void
sbi_set_timer(uint64 stime_value)
{
  register uint64 a0 asm("a0") = stime_value;
  register uint64 a6 asm("a6") = 0; // fid
  register uint64 a7 asm("a7") = SBI_EXT_ID_TIME;
  asm volatile("ecall"
               : "+r"(a0)
               : "r"(a6), "r"(a7)
               : "memory");
}
