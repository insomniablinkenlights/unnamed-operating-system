#ifndef _L_STD_H
#define _L_STD_H
uint64_t INT0x80(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx);
void * sbrk(uint64_t size);
#define pid_t uint64_t
#define size_t uint64_t
#define NULL ((void*)0x0)
typedef struct FILE{
	uint64_t fd;
}FILE;
extern FILE * stdin;
extern FILE * stdout;
typedef struct proc{
	pid_t pid;
	FILE stdin;
	FILE stdout;
}proc;
size_t strlen(const char * a);
size_t fwrite(const void *__restrict __ptr, int __size, int __n, FILE *__restrict __s);
int putc(int __c, FILE *__stream);
int fputs(const char *__restrict __s, FILE *__restrict __stream);
void * malloc(uint64_t size);
void free(void * a);
void memcpy(void * b, void * a, uint64_t size);
void * realloc(void * a, uint64_t size);
int puts(const char *__restrict __s);
size_t fread(void *__restrict __ptr, int __size, int __n, FILE *__restrict __stream);
int getc(FILE *__stream);
char *fgets(char *__s, int __n, FILE *__stream);
proc * execv(const char *__path, char *const __argv[]);
pid_t wait(int *__stat_loc);
void bind(FILE * a, FILE * b);
void bindT(proc * a, FILE * b);
void exit(int c);
void unblock(proc * pid);
#endif
