#include "headers/standard.h"
#include "headers/device.h"
#include "headers/proc.h"
#include "headers/stdint.h"
#include "headers/usermode.h"
#include "headers/addresses.h"
#include "headers/terminal.h"
#include "headers/string.h"
/*to add a usermode driver, we'll open a .flat file and set it to have better perms
  which will then have access to inb, outb, wait_for_irq
  we will then create a struct * DEVICE which will have the function point to its function, the name of the device, and the stream id of its stdio
  to get those it will need to have a main function which sets that in a syscall, and the syscall will then store that and call its driver function with that data
  drivers will also have IOPL 3*/
DEVICE * DEVICE_LIST;
int LAST_DEVICE = -1;
void dProbe(char * name, char * argv){
	if(LAST_DEVICE == -1){
		LAST_DEVICE = 0;
		DEVICE_LIST = KPALLOC(); //TODO: make this and kernelfd dynamic
	}
	uint64_t new_PID = ExecFile(name, argv);
	thread_control_block * new_task = find_child_by_pid(new_PID);
	new_task->PL = 0x2;
	unblock_task(new_task);
	block_task(STATE_WAITING_FOR_POKE);
}
void d0xD(char * name){
	//streamID is currently just the int of the fd
	//dev->streamID = &(((stream*)(current_task_TCB->file_descriptors))[(uint64_t)(dev->streamID)]); //TODO: need to bounds-check
	//dev->pid = current_task_TCB->pid;
	//memcpy(&(DEVICE_LIST[LAST_DEVICE]), dev, sizeof(DEVICE));
	DEVICE * K = &(DEVICE_LIST[LAST_DEVICE]);
	K->streamID = &(((stream*)current_task_TCB->file_descriptors)[0]);
	if(K->streamID->function == NULL){
		//we NEED to create a FD
		inSO(K->streamID, NULL, NULL);
	}
	K->pid = current_task_TCB->pid;
	K->DEV_NAME = malloc(strlen(name)+1);
	memcpy(K->DEV_NAME, name, strlen(name)+1);
	LAST_DEVICE ++; //TODO: need to bounds-check
	unblock_task(current_task_TCB->parent);
	//now we call the driver function with this as the argument
	//switchToUserModeProc(dev->DRIVER_FUNCTION, 0x0); //TODO: with this as the argument...
}
void d0xE(uint64_t errc, char * err_str){
	write_to_screen(err_str, strlen(err_str));	
	BREAK(errc);
	FAULT();
}
stream * dOpen(char * name){
	for(int i = 0; i<LAST_DEVICE; i++){
		if(strcmp(name, DEVICE_LIST[i].DEV_NAME) == 0){
			stream * k2 = malloc(sizeof(stream));
			k2->arguments = malloc(sizeof(stdIO));
			stdIO * m = k2->arguments;
			m->pairIn = (stdIO*)((DEVICE_LIST[i].streamID)->arguments);
			m->pairOut = (stdIO*)((DEVICE_LIST[i].streamID)->arguments);
			m->bufferOut=malloc(sizeof(stdFIFOBUF));
			m->bufferOutEnd = m->bufferOut;
			m->bufferOut->datac = 0;
			m->bufferOut->next = NULL;
			m->bufferSema = create_semaphore(1);
			m->type = 0;
			m->waiter = malloc(sizeof(thread_control_block*));
			*(m->waiter) = NULL;
			k2->position = 0x0;
			//k2->function = StdIOStream;
			k2->function = DEVICE_LIST[i].streamID->function; //quick hack! _should_ be fineeee
			k2->flags = 0x0;
			return k2;
		}
	}
	ERROR(ERR_DEV_NF, (uint64_t)name);
	return NULL;
}
