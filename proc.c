#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/proc.h"
/*
 *	IMPORTANT
 *	when switching between two tasks, the tasks will ALWAYS be in kernel mode, always being interrupt handlers or whatever
 *	usermode tasks are handled by a kernelmode process which launches them, that process will have the interrupts
 *	each kernel process thus needs to have a stack of its own and a RSP0 for the TSS.
 * */
/*
 * TSS
 * tss contains rsp0 and ss0
 * when we switch to kernel mode, rsp0 is loaded from tss, data is pushed, etc
 * this happens AGAIN if a second interrupt happens while in kernel mode
 * a thread's time slice may end during a syscall, calling scheduler in the middle of the syscall, switching to user mode and calling the same syscall again from a different process
 * so interrupts will cli, change rsp to process-specific rsp, move shit off rsp0 to a new rsp, sti, goto that rsp, do shit, return
 *
 * actually that's false
 * we can simply load in our current rsp to the tss's rsp0 when we switch processes or switch to usermode
 * because tss's rsp0 is only loaded on priv change :3
 * */
long long int IRQ_disable_counter = 0;
uint64_t task_switches_postponed_flag=0;
long long int postpone_task_switches_counter=0;
void lock_scheduler(){
	#ifndef SMP
	CLI();
	IRQ_disable_counter++;
#endif
}
void unlock_scheduler(){
#ifndef SMP
	IRQ_disable_counter--;
	if(IRQ_disable_counter==0){
		STI();
	}
#endif
}
void lock_stuff(){
	#ifndef SMP
	CLI();
	IRQ_disable_counter++;
	postpone_task_switches_counter++;
#endif
}
void schedule();
thread_control_block * current_task_TCB;
uint64_t last_count;
thread_control_block * FIRST_THREAD;
thread_control_block * LAST_THREAD;
enum task_states {
	STATE_RUNNING,
	STATE_READY,
	STATE_WAITING,
	STATE_DEAD,
	STATE_PAUSED,
	STATE_WAITING_FOR_IRQ
};
uint64_t TIME=0;
uint64_t CPU_idle_time = 0;
uint64_t get_time_since_boot(){
	return TIME;
}
void unlock_stuff (){
#ifndef SMP
	if(postpone_task_switches_counter == 0){
		FAULT();
	}
	postpone_task_switches_counter--;

	if(postpone_task_switches_counter == 0){
		if(task_switches_postponed_flag != 0){
			if(current_task_TCB == NULL){
				FAULT();
			}
			task_switches_postponed_flag = 0;
			schedule();
		}
	}
	IRQ_disable_counter--;
	if(IRQ_disable_counter == 0){
		STI();
	}
#endif
}
void update_time_used(){
	uint64_t current_count = get_time_since_boot();
	uint64_t elapsed = last_count-current_count;
	last_count=current_count;
	if(current_task_TCB == NULL){
		CPU_idle_time += elapsed;
	}else{
		current_task_TCB->time_used+=elapsed;
	}
}
void initialize_multitasking(){
	initMalloc(); //shouldn't put it here but who cares anymore
	memfill((uint64_t*)(MBASE+0x1d000), sizeof(thread_control_block)); //just in case...
	((thread_control_block*)(MBASE+0x1d000))->rsp=(void*)(CBASE+0x7bff);
	((thread_control_block*)(MBASE+0x1d000))->rsp0=(void*)(CBASE+0x7bff);
	((thread_control_block*)(MBASE+0x1d000))->state=STATE_RUNNING;
	current_task_TCB=(thread_control_block*)(MBASE+0x1d000);
	current_task_TCB->time_used=0x0;
	current_task_TCB->priority=0x0;
	current_task_TCB->ttn = 0x0;
	current_task_TCB->time_used=0x0;
	current_task_TCB->irq_waiting_for = 0x0;
	current_task_TCB->PL = 0x0;
	current_task_TCB->timeout_function = NULL;
	current_task_TCB->file_descriptors = NULL;
	current_task_TCB->cr3 = KPALLOC();
	memfill(current_task_TCB->cr3, 0x1000); //yeah this is fucked up
	current_task_TCB->vaddsp = NULL;
	current_task_TCB->kernelFdLen = 0x0;
	FIRST_THREAD=NULL;
	LAST_THREAD=NULL;
//	FIRST_THREAD=current_task_TCB;
//	LAST_THREAD=current_task_TCB;
}
void switch_to_task(thread_control_block* next_thread);

uint64_t time_slice_remaining=50000000;
void switch_to_task_wrapper(thread_control_block * task){
	if(postpone_task_switches_counter != 0){
		task_switches_postponed_flag = 1;
		return;
	}
	if(current_task_TCB == NULL){
		time_slice_remaining = 0;
	}else if(FIRST_THREAD==NULL && current_task_TCB->state != STATE_RUNNING){ //if we have only blocked tasks
		time_slice_remaining = 0;
	}else{
		time_slice_remaining = 50000000;
	}
	update_time_used();
	//switch fd?
	switch_to_task(task);
}
thread_control_block * create_kernel_task(void startingRIP(void)){
	thread_control_block * M = malloc(sizeof(thread_control_block));
	void * M2 = KPALLOC();
	//lowkey i think this is the error
	M->rsp0=M2+0xFF8;
	M->rsp=M2+0x1000-5*8; //near ret is used in stt, we pop four quads (rbp -> rdi -> rsi -> rbx) and then pop rip
	M->state = STATE_READY;
	M->next=0x0;
	M->priority = 0x0; 
	M->ttn = 0x0;
       	M->time_used = 0x0;
       	M->PL =0x0; 
	M->timeout_function = NULL;
       	M->irq_waiting_for=0x0;
       	M->file_descriptors=NULL; 
	M->cr3=KPALLOC(); //each task has its own cr3 where the top half is the same as in our mbase and the bottom half is usermode
	memfill(M->cr3, 0x1000);
				//so each time we update kernel memory we need to update both actual cr3 and 0x10000? 
				//nah, when we switch out we'll just change the 10k
	M->vaddsp=NULL;
	M->kernelFdLen=0x0;
	*(uint64_t*)(((char*)M2+0xFF8)) = (uint64_t)startingRIP; //this should be the last one we pop!
	if(FIRST_THREAD != NULL){
		LAST_THREAD->next = M;
		LAST_THREAD= M;
	}else{
		FIRST_THREAD=M;
		LAST_THREAD=M;
	}

	M->time_used=0x0;
	return M;
}
void schedule(){
	//what really is a scheduler
	//okay jokes aside, we need malloc first because these tcbs are tiny
	//malloc done !
	thread_control_block * task;
	if(postpone_task_switches_counter !=0){
		task_switches_postponed_flag =1;
		return;
	}
	//caller is responsible for locking scheduler
//	if(current_task_TCB == NULL){
//		asm volatile("xchgw %bx, %bx; call FAULT");
//	}
	if(FIRST_THREAD != NULL){
		task = FIRST_THREAD;
		FIRST_THREAD=task->next;
		switch_to_task_wrapper(task);
	}else if(current_task_TCB->state == STATE_RUNNING){
	
	}
	else{
		//when we unblock the task we co-opted, we're screwed...
		task=current_task_TCB;
		current_task_TCB=NULL;
		time_slice_remaining=0x0;
	//	uint64_t idle_start_time = get_time_since_boot();
		do{
			asm volatile("sti; hlt; cli;");
		//	STI();
		//	HLT();
		//	CLI();
		}while(FIRST_THREAD==NULL);
		current_task_TCB=task;
		task = FIRST_THREAD;
		FIRST_THREAD = task->next;
		if(task != current_task_TCB){
			switch_to_task_wrapper(task);
		}
	}
}
void block_task (uint8_t reason){
	lock_scheduler();
	current_task_TCB->state = reason;

	schedule();
	unlock_scheduler();
}
void unblock_task(thread_control_block * task){
	lock_scheduler();
	//switching should be postponed anyway !!!
	

	//if task switching is postponed, we try to switch to a task not on the ready list and thus remove it from both lists...
	task->state=STATE_READY;
	if(current_task_TCB == NULL){
		//prevents trying to switch to a task that never had its rsp pushed, we need to return to schedule and let them handle that
		if(FIRST_THREAD == NULL){
			LAST_THREAD=task;
		task->next=NULL;
		FIRST_THREAD=LAST_THREAD;

		}
		else{
			//we are in an idle state but there is a task that needs to be unblocked already... 
		LAST_THREAD->next=task;
		LAST_THREAD=task;
		//	asm volatile("xchgw %bx, %bx; mov $1, %cl");
		//	FAULT(); //??
		}
	}
	else if(FIRST_THREAD == NULL){
		LAST_THREAD=task;
		task->next=NULL;
		FIRST_THREAD=LAST_THREAD;
		switch_to_task_wrapper(task);
	}else{
		LAST_THREAD->next=task;
		LAST_THREAD=task;
	}
	unlock_scheduler();
}

thread_control_block * sleeping_task_list;
void nano_sleep_until(uint64_t t){
	lock_stuff();
	if(t<get_time_since_boot()){
	//	asm volatile("xchgw %bx, %bx");
		unlock_scheduler();
		return;
	}
	current_task_TCB->ttn=t;
	current_task_TCB->next = sleeping_task_list;
	sleeping_task_list = current_task_TCB;
	//do other stuff
//	asm volatile("xchgw %bx, %bx");
	unlock_stuff();
//	asm volatile("xchgw %bx, %bx");
	block_task (STATE_WAITING);
}
void nano_sleep(uint64_t t){
	nano_sleep_until(TIME+t);
}
void sleep(uint64_t t){
	t=t*1000000000; //TODO: broken for no fucking reason
	nano_sleep(t);
}
thread_control_block * irqwaiting;
void wait_for_irq(uint8_t irq, uint64_t timeout, void timeout_function(void)){
	lock_stuff();
	current_task_TCB->irq_waiting_for = irq;
	current_task_TCB->next=irqwaiting;
	if(timeout == 0x0){
		current_task_TCB->ttn=0x0;
	}else{
		current_task_TCB->ttn = TIME + timeout;
	}
	current_task_TCB->timeout_function = timeout_function;
	irqwaiting=current_task_TCB;
	unlock_stuff();
	block_task(STATE_WAITING_FOR_IRQ); //for some reason we never switch back!
}
void unblockirq(uint8_t irq){
	thread_control_block * next_task;
	thread_control_block * this_task;
	lock_stuff();
	next_task = irqwaiting;
	irqwaiting = NULL;
	while(next_task != NULL){
	this_task = next_task;
	next_task = this_task->next;
	if(this_task -> irq_waiting_for == irq){
		unblock_task(this_task);
	}else{
		this_task->next = irqwaiting;
		irqwaiting= this_task;
	}
	}
	unlock_stuff();
}
void PIT_IRQ_handler(){
	thread_control_block * next_task;
	thread_control_block * this_task;
	lock_stuff();
	TIME += 10000000; //100hz, slightly off, will fix later
	next_task = sleeping_task_list;
	sleeping_task_list = NULL;
	//for each task, wake up or put back on list
	while(next_task != NULL){
		this_task = next_task;
		next_task = this_task -> next;
		if(this_task == next_task){
			ERROR(ERR_TT_NT, 0x0);
		}
		if( this_task -> state == STATE_WAITING_FOR_IRQ){
			ERROR(ERR_PIT_WFI_MISP, (uint64_t)this_task);
		}
		if( this_task -> ttn <= TIME){
			unblock_task(this_task);
		}else{
			this_task->next = sleeping_task_list;
			sleeping_task_list = this_task;
		}
	}
	next_task = irqwaiting;
	irqwaiting=NULL;
	while(next_task != NULL){
		this_task = next_task;
		next_task = this_task ->next;
		if(this_task -> ttn <= TIME && this_task->ttn != 0x0){
			if(this_task -> timeout_function == NULL){
				ERROR(ERR_TFNTTNNZ, (uint64_t)this_task);
			}
			*((void**)(this_task ->rsp+48)) = this_task -> timeout_function; //TODO: not sure if this works!
			unblock_task(this_task);
		}else{
			this_task -> next = irqwaiting;
			irqwaiting=this_task;
		}
	}
	if(time_slice_remaining != 0){
		if(time_slice_remaining <= 10000000){
			schedule();
		}else{
			time_slice_remaining -= 10000000;
		}
	}
	unlock_stuff(); //after unblocking task it is postponed until HERE, we then call schedule where it errors out
}

