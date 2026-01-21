#include "stdint.h"
#ifndef proc
#define proc
typedef struct __attribute__((packed)) thread_control_block {
	void * rsp; //+0
	void * rsp0; //+8
	struct thread_control_block * next; //+16
	uint8_t state; //+24
	uint8_t priority; //+25
	uint64_t ttn; //+26
	uint64_t time_used; //+34
	uint8_t irq_waiting_for; //+42
	uint8_t PL; //+43
	void * timeout_function; //+44
	void * file_descriptors;// +52
	void * cr3;//+60
	void * vaddsp;//+64
	uint64_t kernelFdLen; //+68
} thread_control_block;
extern thread_control_block * current_task_TCB;
thread_control_block * create_kernel_task(void startingRIP(void));
#endif
