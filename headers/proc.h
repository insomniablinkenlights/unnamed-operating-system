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
	void * vaddsp;//+68
	uint64_t kernelFdLen; //+76
	uint64_t pid; //+84
		      //pid's last bit will be set to 1 if we have a task to unblock
	uint64_t pidW; //+92
} thread_control_block;
typedef struct SEMAPHORE{
	int64_t max_count;
	int64_t current_count;
	thread_control_block * first_waiting;
	thread_control_block * last_waiting;
}SEMAPHORE;
void acquire_semaphore(SEMAPHORE * semaphore);
void release_semaphore(SEMAPHORE * semaphore);
SEMAPHORE * create_semaphore(int max_count);
extern thread_control_block * current_task_TCB;
thread_control_block * create_kernel_task(void startingRIP(void));
thread_control_block * ckprocA(void startingRIP(void * arguments), void * arguments);
void PROC_EXIT();
#endif
