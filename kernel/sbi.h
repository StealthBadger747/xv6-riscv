// SBI (Supervisor Binary Interface) calls, used to ask OpenSBI
// (running in M-mode) for services we cannot do from S-mode,
// such as programming the next timer interrupt.

#ifndef SBI_H
#define SBI_H

#define SBI_EXT_ID_TIME 0x54494D45

void sbi_set_timer(uint64 stime_value);

#endif
