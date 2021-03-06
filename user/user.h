struct stat;
struct rtcdate;
struct p_table;
struct c_info;
struct mem_info;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int numprocs(void);
void traceon(void);
void psget(void); //struct p_table*
int suspend(int, int);
int resume(char *);
int cstart(int, char *, char *, int, int, int);
int cpause(char *);
int cresume(char *);
int cstop(char *);
int cinfo(void); // struct c_info *
int mypriv(void);
int freemem(struct mem_info *);
int getticks(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
