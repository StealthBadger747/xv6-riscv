//
// low-level driver routines for 16550a UART.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
// QEMU's 16550 registers are byte-spaced.  SG2002 UART registers
// are four bytes apart, with the implemented fields in the low byte.
#ifdef QEMU
#define REG_SHIFT 0
#else
#define REG_SHIFT 2
#endif
#define Reg(reg) ((volatile unsigned char *)(UART0 + ((reg) << REG_SHIFT)))

// the UART control registers.
// some have different meanings for
// read vs write.
// http://byterunner.com/16550.html
#define RHR 0 // receive holding register (for input bytes)
#define THR 0 // transmit holding register (for output bytes)
#define IER 1 // interrupt enable register
#define FCR 2 // FIFO control register
#define ISR 2 // interrupt status register
#define LCR 3 // line control register
#define LSR 5 // line status register
#define USR 31 // DW APB UART status register (offset 0x7c)
#define DLL 0 // divisor latch low (when LCR bit 7 set)
#define DLH 1 // divisor latch high (when LCR bit 7 set)
#define LCR_BAUD_LATCH (1<<7)
#define LCR_EIGHT_BITS (3<<0)

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

void
uartinit(void)
{
  // disable interrupts.
  WriteReg(IER, 0x00);

#ifdef QEMU
  // special mode to set baud rate, 8 bits, no parity.
  WriteReg(LCR, LCR_BAUD_LATCH | LCR_EIGHT_BITS);

  // LSB for baud rate of 38.4K.
  WriteReg(DLL, 0x03);

  // MSB for baud rate of 38.4K.
  WriteReg(DLH, 0x00);

  // leave set-baud mode.
  WriteReg(LCR, LCR_EIGHT_BITS);

  // reset and enable FIFOs.
  WriteReg(FCR, 0x07);
#else
  // The Duo enters through U-Boot, which has already configured UART0
  // for the live 115200/8-N-1 console. Preserve that known-good state.
  // In particular, do not rewrite LCR during the handoff: SG2002's DW
  // UART can reject that write and latch a busy-detect interrupt while
  // the inherited transmitter/receiver is active.
  (void)ReadReg(USR);
#endif

  // enable receive interrupts.
  WriteReg(IER, 0x01);
}

// write one output character to the UART.
void
uartputc(int c)
{
  // wait for Transmit Holding Empty to be set in LSR.
  while((ReadReg(LSR) & (1 << 5)) == 0)
    ;
  WriteReg(THR, c);
}

// read one input character from the UART.
// return -1 if none is waiting.
int
uartgetc(void)
{
  /*printf("---------------------------------------------\n");
  printf("LSR: '%d', '%d'\n", ReadReg(LSR) & 0x01, ReadReg(LSR) & 0x01);
  printf("RHR: '%d', '%d', '%d', '%d', '%d'\n", ReadReg(RHR), ReadReg(RHR), ReadReg(RHR), ReadReg(RHR), ReadReg(RHR));
  printf("---------------------------------------------\n"); */
  if(ReadReg(LSR) & 0x01){
    // input data is ready.
    return ReadReg(RHR);
  } else {
    return -1;
  }
}

// trap.c calls here when the uart interrupts.
void
uartintr(void)
{
#ifndef QEMU
  // DW APB UART interrupt ID 0x7 is busy detect. Reading USR is the
  // documented acknowledgement; draining RHR does not clear it.
  if((ReadReg(ISR) & 0xf) == 0x7)
    (void)ReadReg(USR);
#endif

  while(1){
    int c = uartgetc();
    if(c == 27) {
      c = uartgetc();
      if(c == 79)
        consoleintr(uartgetc() - 80 + 200);
      else if(c == 91 && uartgetc() == 49) {
        c = uartgetc();
        if(c == 53)
          consoleintr(204), uartgetc();
        else if(c == 55)
          consoleintr(205), uartgetc();
        else if(c == 56)
          consoleintr(206), uartgetc();
        else if(c == 57)
          consoleintr(207), uartgetc();
      }
    }
    else if(c == -1)
      break;
    else
        consoleintr(c);
  }
}
