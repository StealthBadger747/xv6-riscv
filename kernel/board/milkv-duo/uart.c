#include "../../types.h"
#include "../../param.h"
#include "../../memlayout.h"
#include "../../riscv.h"
#include "../../spinlock.h"
#include "../../proc.h"
#include "../../defs.h"
#include "config.h"

// UART registers
#define UART_RBR 0x00  // Receive Buffer Register
#define UART_THR 0x00  // Transmit Holding Register
#define UART_IER 0x01  // Interrupt Enable Register
#define UART_IIR 0x02  // Interrupt Identification Register
#define UART_FCR 0x02  // FIFO Control Register
#define UART_LCR 0x03  // Line Control Register
#define UART_MCR 0x04  // Modem Control Register
#define UART_LSR 0x05  // Line Status Register
#define UART_MSR 0x06  // Modem Status Register
#define UART_SCR 0x07  // Scratch Register
#define UART_DLL 0x00  // Divisor Latch (LSB)
#define UART_DLM 0x01  // Divisor Latch (MSB)

// LSR bits
#define LSR_RX_READY (1 << 0)
#define LSR_TX_IDLE  (1 << 5)

static struct spinlock uart_tx_lock;
static struct spinlock uart_rx_lock;

// Initialize UART
void
uartinit(void)
{
  // Disable interrupts
  *(uint8*)(UART0 + UART_IER) = 0x00;

  // Set baud rate to 115200
  *(uint8*)(UART0 + UART_LCR) = 0x80;  // Enable DLAB
  *(uint8*)(UART0 + UART_DLL) = 0x01;  // 115200 baud
  *(uint8*)(UART0 + UART_DLM) = 0x00;
  *(uint8*)(UART0 + UART_LCR) = 0x03;  // 8N1

  // Enable FIFO
  *(uint8*)(UART0 + UART_FCR) = 0x01;

  // Enable interrupts
  *(uint8*)(UART0 + UART_IER) = 0x01;  // Enable receive interrupts

  initlock(&uart_tx_lock, "uart_tx");
  initlock(&uart_rx_lock, "uart_rx");
}

// Send a byte
void
uartputc(int c)
{
  acquire(&uart_tx_lock);
  while((*(uint8*)(UART0 + UART_LSR) & LSR_TX_IDLE) == 0)
    ;
  *(uint8*)(UART0 + UART_THR) = c;
  release(&uart_tx_lock);
}

// Read a byte
int
uartgetc(void)
{
  if((*(uint8*)(UART0 + UART_LSR) & LSR_RX_READY) == 0)
    return -1;
  return *(uint8*)(UART0 + UART_RBR);
}

// UART interrupt handler
void
uartintr(void)
{
  while(1){
    int c = uartgetc();
    if(c == -1)
      break;
    consoleintr(c);
  }
} 
