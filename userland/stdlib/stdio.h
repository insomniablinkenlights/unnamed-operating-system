#ifndef _STDIO_H
#define _STDIO_H 1
#include "stdint.h"
typedef struct fpos_t {
	
}fpos_t;
typedef struct FILE{
	uint64_t fd;
	fpos_t POS;
}FILE;
extern FILE *stdin;
extern FILE * stdout;
extern FILE *fopen (const char *__filename, const char *__modes);
extern int getc (FILE *__stream);
extern int getchar (void);
extern int putc (int __c, FILE *__stream);
extern int putchar (int __c);
//no stderr for now...

#endif
