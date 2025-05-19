#include "../../types.h"
#include "../../param.h"
#include "../../memlayout.h"
#include "../../riscv.h"
#include "../../spinlock.h"
#include "../../proc.h"
#include "../../defs.h"
#include "config.h"

// Enable I-cache
void
enable_icache(void)
{
  // Invalidate I-cache
  asm volatile("csrc %0, %1" : : "i"(CSR_MCOR), "i"(CACHE_INVALIDATE_ICACHE));
  asm volatile("csrs %0, %1" : : "i"(CSR_MCOR), "i"(CACHE_INVALIDATE_ICACHE));
  
  // Enable I-cache
  asm volatile("csrs %0, %1" : : "i"(CSR_MHCR), "i"(CACHE_ENABLE_ICACHE));
}

// Enable D-cache
void
enable_dcache(void)
{
  // Invalidate D-cache
  asm volatile("csrc %0, %1" : : "i"(CSR_MCOR), "i"(CACHE_INVALIDATE_DCACHE));
  asm volatile("csrs %0, %1" : : "i"(CSR_MCOR), "i"(CACHE_INVALIDATE_DCACHE));
  
  // Enable D-cache
  asm volatile("csrs %0, %1" : : "i"(CSR_MHCR), "i"(CACHE_ENABLE_DCACHE));
}

// Disable I-cache
void
disable_icache(void)
{
  asm volatile("csrc %0, %1" : : "i"(CSR_MHCR), "i"(CACHE_ENABLE_ICACHE));
}

// Disable D-cache
void
disable_dcache(void)
{
  asm volatile("csrc %0, %1" : : "i"(CSR_MHCR), "i"(CACHE_ENABLE_DCACHE));
}

// Invalidate I-cache
void
invalidate_icache(void)
{
  asm volatile("th.icache.iall");
  asm volatile("th.sync.i");
}

// Invalidate D-cache range
void
invalidate_dcache_range(uint64 start, uint64 size)
{
  uint64 end = start + size;
  start = start & ~(64-1);  // Align to cache line size
  
  for(uint64 addr = start; addr < end; addr += 64) {
    // dcache.cva rd - T-Head custom instruction
    asm volatile(".word 0x0c000073" : : "r"(addr));
  }
  asm volatile("th.sync.i");
}

// Flush D-cache range
void
flush_dcache_range(uint64 start, uint64 size)
{
  uint64 end = start + size;
  start = start & ~(64-1);  // Align to cache line size
  
  for(uint64 addr = start; addr < end; addr += 64) {
    // dcache.cva rd - T-Head custom instruction
    asm volatile(".word 0x0c000073" : : "r"(addr));
  }
  asm volatile("th.sync.i");
} 
