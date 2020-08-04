// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context scheduler;   // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// the sscratch register points here.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

enum procstate { UNUSED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, SUSPENDED };

// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  struct proc *parent;         // Parent process
  void *chan;                  // If non-zero, sleeping on chan
  int suspended;               // Tells us whether it is suspended or not
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Bottom of kernel stack for this process
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // Page table
  struct trapframe *tf;        // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

  // Add a flag for strace functionality
  int traceon;                 // Flag for strace fucntionality, default is 0

  // Container specific values
  int cont_id;                 // The ID of the corresponding container
};

enum contstate { EMPTY, CREATED, USED };

// Struct for each container
struct container {
  int privilege_level;        // 0 signifies root privileges
  int cont_id;                // The ID for the container aka the index
  char name[32];              // The name of container
  int proc_limit;             // -1 for uninitialized and 0 for infinite
  int mem_limit;              // -1 for uninitialized and 0 for infinite
  int disk_limit;             // -1 for uninitialized and 0 for infinite
  int proc_count;             // uninitialized is at 0
  int mem_usage;              // uninitialized is at 0
  int disk_usage;             // uninitialized is at 0
  struct inode *rootdir;
  char rootdir_str[MAXPATH];
  int tokens;

  // Specifies whether it is in use or not
  int cont_state;                 // Value based on enum 'contstate'.

  // Lock for container modifications
  struct spinlock lock;
};

// Add p_info struct for storing the information of a process in the p_table struct
struct p_info {
  int pid;
  uint64 sz;
  char state[16];
  char name[16];
};

// Add p_table struct for the ps command to use
struct p_table {
  struct p_info table[NPROC];    // A table of p_info, aka active processes
  int p_count;                // Keeps track of the number of entries in table
};

// Header for resuming / suspending processes
struct suspended_hdr {
  uint64 mem_sz;               // Size of program in memory
  uint64 code_sz;              // Size of the code in memory
  uint64 stack_sz;             // Size of the stack
  int traceon;                 // Was tracing on?
  char name[16];               // Name of process
};