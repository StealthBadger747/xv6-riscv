//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

//
// send one character to the uart.
//
void
consputc(int c)
{
  extern volatile int panicked; // from printf.c

  if(panicked){
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else {
    uartputc(c);
  }
}

struct console {
  struct spinlock lock;
  
  // input
#define INPUT_BUF 128
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
};

static int active = 1;

struct console consoles[MAXCONS];
struct console *consa;

//
// user write()s to the console go here.
//
int
consolewrite(struct console *cons, int user_src, uint64 src, int n)
{
  int i;

  acquire(&cons->lock);
  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    // If console is active write it to the console.
    if(cons == consa) {
      consputc(c);
    }
  }
  release(&cons->lock);

  return n;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(struct console *cons, int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons->lock);
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons->buffer.
    while(cons->r == cons->w){
      if(myproc()->killed){
        release(&cons->lock);
        return -1;
      }
      sleep(&cons->r, &cons->lock);
    }

    c = cons->buf[cons->r++ % INPUT_BUF];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons->r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons->lock);

  return target - n;
}

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons->buf,
// wake up consoleread() if a whole line has arrived.
//
void
consoleintr(int c)
{
  int doconsoleswitch = -1;
  acquire(&consa->lock);

  switch(c){
  case C('P'):  // Print process list.
    procdump();
    break;
  case C('U'):  // Kill line.
    while(consa->e != consa->w &&
          consa->buf[(consa->e-1) % INPUT_BUF] != '\n'){
      consa->e--;
      consputc(BACKSPACE);
    }
    break;
  case C('H'): // Backspace
  case '\x7f':
    if(consa->e != consa->w){
      consa->e--;
      consputc(BACKSPACE);
    }
    break;
  case 200: // F1
    doconsoleswitch = 0;
    break;
  case 201:
    doconsoleswitch = 1;
    break;
  case 202:
    doconsoleswitch = 2;
    break;
  case 203:
    doconsoleswitch = 3;
    break;
  case 204:
    doconsoleswitch = 4;
    break;
  case 205:
    doconsoleswitch = 5;
    break;
  case 206:
    doconsoleswitch = 6;
    break;
  case 207:
    doconsoleswitch = 7;
    break;
  default:
    if(c != 0 && consa->e-consa->r < INPUT_BUF){
      c = (c == '\r') ? '\n' : c;

      // echo back to the user.
      consputc(c);

      // store for consumption by consoleread().
      consa->buf[consa->e++ % INPUT_BUF] = c;

      if(c == '\n' || c == C('D') || consa->e == consa->r+INPUT_BUF){
        // wake up consoleread() if a whole line (or end-of-file)
        // has arrived.
        consa->w = consa->e;
        wakeup(&consa->r);
      }
    }
    break;
  }
  
  release(&consa->lock);

  if(doconsoleswitch > -1){
    printf("\nVirtual Console Switched to #%d\n", doconsoleswitch);
    consa = &consoles[doconsoleswitch];
    printf("\nActive console now: vc%d\n", doconsoleswitch);
  }
}

int
cread(int minor, int user_dst, uint64 dst, int n)
{
  return consoleread(&consoles[minor], user_dst, dst, n);
}

int
cwrite(int minor, int user_src, uint64 src, int n)
{
  return consolewrite(&consoles[minor], user_src, src, n);
}

void
consoleinit(void)
{
  char lock_name[] = "cons0";
  // This will break if there are more than 10 consoles
  if(MAXCONS > 10)
    printf("WARNING!!!! 'MAXCONS' is too high and will cause issues!\n");

  for(int i = 0; i < MAXCONS; i++) {
    lock_name[4] = '0' + i;
    initlock(&consoles[i].lock, lock_name);
    consoles[i].r = 0;
    consoles[i].w = 0;
    consoles[i].e = 0;
  }
/*
  initlock(&cons2.lock, "cons2");
  cons2.r = 0;
  cons2.w = 0;
  cons2.e = 0;
*/
  consa = &consoles[0];
  active = 1;

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  for(int i = 1; i <= MAXCONS; i++) {
    devsw[i].read = cread;
    devsw[i].write = cwrite;
  }

  /*devsw[CONSOLE1].read = cread1;
  devsw[CONSOLE1].write = cwrite1;

  devsw[CONSOLE2].read = cread2;
  devsw[CONSOLE2].write = cwrite2;*/
}
