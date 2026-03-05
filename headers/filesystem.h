#ifndef filesystem
#define filesystem
#include "proc.h"
#include "stdint.h"
void CloseAllFds();
uint64_t OPEN(char * filename, uint64_t flags);
void READ(uint64_t fd, void * buffer, uint64_t len);
void CLOSE(uint64_t fd);
void SEEK(uint64_t fd, uint64_t pos, uint64_t WHENCE);
void WRITE(uint64_t fd, void * buffer, uint64_t len);
uint64_t TELL(uint64_t fd);
void InitKernelFd();
void PS2_DRIVER_BIND_STDIO();
uint64_t OpenStdIn();
typedef struct stream {
	uint64_t (*function)(void *, uint64_t, void *, uint64_t, uint8_t);
	void * arguments;
	uint64_t position;
	uint64_t flags;
}stream;
void BIND_T_STDIO(thread_control_block * A, uint64_t FD0, thread_control_block * B, uint64_t FD1); 
void BIND_HANDLES(uint64_t FD0, uint64_t FD1);
void BINDR(thread_control_block * b);
#endif
