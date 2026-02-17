#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/proc.h"
#include "headers/usermode.h"
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
uint64_t next_PID = 1;
long long int postpone_task_switches_counter=0;
uint64_t time_slice_remaining=50000000;
thread_control_block * current_task_TCB = NULL;
uint64_t last_count = 0;
thread_control_block * FIRST_THREAD = NULL;
thread_control_block * LAST_THREAD = NULL;
thread_control_block * DEAD_TASKS = NULL;
thread_control_block * sleeping_task_list = NULL;
thread_control_block * irqwaiting = NULL;
thread_control_block * procwaiting = NULL;
uint64_t TIME=0;
uint64_t CPU_idle_time = 0;
void schedule();
void switch_to_task(thread_control_block* next_thread);
void unblockP(uint64_t pid);
void block_task(uint8_t reason);
void unblock_task(thread_control_block * task);
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
enum task_states {
	STATE_RUNNING = 0,
	STATE_READY,
	STATE_WAITING,
	STATE_DEAD,
	STATE_PAUSED,
	STATE_WAITING_FOR_IRQ,
	STATE_WAITING_FOR_PROC_UPDATE,
	STATE_WAITING_FOR_LOCK,
	STATE_WAITING_FOR_POKE,
	STATE_WAITING_FOR_DEATH
};
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
	current_task_TCB->pid = next_PID++;
	current_task_TCB->pidW = 0;
	memfill(current_task_TCB->cr3, 0x1000); //yeah this is fucked up
	current_task_TCB->vaddsp = NULL;
	current_task_TCB->kernelFdLen = 0x0;
	current_task_TCB->parent = NULL;
	current_task_TCB->children = NULL;
	FIRST_THREAD=NULL;
	LAST_THREAD=NULL;
//	FIRST_THREAD=current_task_TCB;
//	LAST_THREAD=current_task_TCB;
}
void switch_to_task_wrapper(thread_control_block * task){
	if(postpone_task_switches_counter != 0){
		task_switches_postponed_flag = 1;
		return;
	}
	if(current_task_TCB == NULL){
		time_slice_remaining = 0;
	}else{
		if(current_task_TCB->state == STATE_READY){
			ERROR(ERR_SHOULD_NOT_BE_READY2, (uint64_t)current_task_TCB);
		}
		if(current_task_TCB->state == STATE_DEAD){
			//add to the reaper list
			current_task_TCB->next = DEAD_TASKS;
			DEAD_TASKS=current_task_TCB;
		}
		if(FIRST_THREAD==NULL && current_task_TCB->state != STATE_RUNNING){ //if we have only blocked tasks
			time_slice_remaining = 0;
		}else{
			time_slice_remaining = 50000000;
		}
	}
	update_time_used();
	//free dead task's stuff IN stt
	//switch fd?
	loadRSP0((uint64_t)(task->rsp0));
	switch_to_task(task);
}
void waitForChildToDie(){
	block_task(STATE_WAITING_FOR_DEATH); //if we have no children this will break our mind
}
void appendChild(thread_control_block * A, thread_control_block * B){
	TCB_CH * k = A->children;
	A->children = malloc(sizeof(TCB_CH));
	A->children->next = k;
	A->children->ch = B;
	if(B->parent == NULL){
		B->parent = A;
	}
}
void murderChild(thread_control_block * missing_eight_year_old_white_girl){ //Used when the daughter has died.
	thread_control_block * single_mother_of_3 = missing_eight_year_old_white_girl->parent;
	TCB_CH * victims = single_mother_of_3->children;
	TCB_CH * prev = NULL;
	while(victims != NULL && victims->ch != missing_eight_year_old_white_girl){
		prev = victims;
		victims = victims->next;
	}
	if(victims == NULL){
		//The girl claims a parent which has disowned her!
		ERROR(ERR_AMBER_ALERT, (uint64_t)missing_eight_year_old_white_girl);
	}
	if(prev == NULL){
		single_mother_of_3->children = victims->next;
	}else{
		prev->next = victims->next;
	}
	//Alert the mother that her daughter has died.
	if(single_mother_of_3->state != STATE_WAITING_FOR_DEATH){ //Psychopathic mother.
	}else{
		unblock_task(single_mother_of_3);
	}
}
thread_control_block * create_kernel_task(void startingRIP(void)){
	//deprecating it
	return ckprocA((void (*) (void *))startingRIP, NULL);
}
thread_control_block * ckprocA(void startingRIP(void * arguments), void * arguments){
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
	M->pid = next_PID++;
	M->pidW = 0;
	M->children = NULL;
	M->parent = NULL;
	appendChild(current_task_TCB, M);
	*(uint64_t*)(((char*)M2+0xFF8)) = (uint64_t)startingRIP; //this should be the last one we pop!
	*(uint64_t*)(((char*)M2+0xFE0)) = (uint64_t)arguments; //this should be the second one we pop!
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
void reap(){
	thread_control_block * task = DEAD_TASKS;
	while(task != NULL){
		BREAK((uint64_t)task);
		//free the stack somehow? doesn't work for the first task though
		if(task->parent) murderChild(task);
		if(task->file_descriptors) P_FREE(task->file_descriptors);
		if(task->cr3) P_FREE(task->cr3);
		//vaddsp is unused
		thread_control_block * next = task->next;
		//free(task); //this doesn't work for the first task!
		task = next;
	}
	DEAD_TASKS = NULL;
}
void schedule();
void proc_relent(){
	if(FIRST_THREAD == NULL){ //the other thread IS waiting for an IRQ, but when we receive an IRQ we don't switch to it?
		BREAK(0x9);
	}else{
		BREAK(0x10);
	}
	schedule(); //eventually it should like uhh do more
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
	reap();
	if(current_task_TCB->state == STATE_READY){
		ERROR(ERR_SHOULD_NOT_BE_READY3, (uint64_t) current_task_TCB);
	}
	if(FIRST_THREAD != NULL){
		task = FIRST_THREAD;
		FIRST_THREAD=task->next;
		switch_to_task_wrapper(task);
	}else if(current_task_TCB->state == STATE_RUNNING){
		//do nothing, just continue...
	}
	else{
		//when we unblock the task we co-opted, we're screwed...
		task=current_task_TCB;
		if(task->state == STATE_READY){
			ERROR(ERR_SHOULD_NOT_BE_READY, (uint64_t) task); //because we would have been on first thread
		}
		current_task_TCB=NULL;
		time_slice_remaining=0x0;
	//	uint64_t idle_start_time = get_time_since_boot();
		do{
			asm volatile("sti; hlt; cli;");
		//	STI();
		//	HLT();
		//	CLI();
		}while(FIRST_THREAD==NULL);
		current_task_TCB=task; //if task is modified then that's not good...
	//	if(task->state == STATE_READY){
	//		ERROR(ERR_SHOULD_NOT_BE_READY, (uint64_t) task); //something changed!
	//	}
		task = FIRST_THREAD;
		FIRST_THREAD = task->next;
		if(task != current_task_TCB){
			switch_to_task_wrapper(task);
		}else{
			current_task_TCB->state = STATE_RUNNING; //this fixes it! ^w^
								  //this in fact does NOT fix it
								  //I think we're unblocking it and setting the state to STATE_READY somewhere, and then it's the first and only thread on the list because the other one leaves it?
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
	
	if(task->state == STATE_RUNNING){
		ERROR(ERR_TASK_SHOULD_BE_RUNNING, (uint64_t)task);
	}
	if(task->state == STATE_READY){
		ERROR(ERR_TASK_BADUNBLOCK, (uint64_t)task);
	}
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
	else if(FIRST_THREAD == NULL){ //we have a task running but we have an empty ready list, so we put the task on the ready list and then switch to it
		LAST_THREAD=task;
		task->next=NULL;
		FIRST_THREAD=LAST_THREAD;
	//	switch_to_task_wrapper(task); //sets state to running...
					      //but the problem is... this lets us go back but it doesn't go the other way!
	}else{
		LAST_THREAD->next=task;
		LAST_THREAD=task;
	}
	unlock_scheduler();
}

void nano_sleep_until(uint64_t t){
	lock_stuff();
	if(t<get_time_since_boot()){
	//	asm volatile("xchgw %bx, %bx");
		unlock_stuff();
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
	uint8_t haveweunblocked = 0;
	while(next_task != NULL){
		this_task = next_task;
		next_task = this_task->next;
		if(this_task -> irq_waiting_for == irq){
			unblock_task(this_task);
			haveweunblocked=1;
		}else{
			this_task->next = irqwaiting;
			irqwaiting= this_task;
		}
	}
	if(!haveweunblocked){
		ERROR(ERR_HAVENT_UNBLOCKED, irq);
	}
	unlock_stuff();
}
thread_control_block * find_task_by_pid(uint64_t pid){ //doesn't work for tasks waiting for semaphores -- TODO
	//check the list of running tasks, then the list of sleeping tasks, then the list of blocked tasks waiting for procs, then the list of blocked tasks waiting for irqs, then return NULL
	//this should not be exposed to users
	if(pid == 0) return current_task_TCB;
	thread_control_block * task = NULL;
	task = FIRST_THREAD;
	pid |= ((uint64_t)1<<63);
	while(task != NULL){
		if((task->pid | ((uint64_t)1<<63)) == pid) return task;
		task = task->next;
	}
	task = sleeping_task_list;
	while(task != NULL){
		if((task->pid | ((uint64_t)1<<63)) == pid) return task;
		task = task->next;
	}
	task = procwaiting;
	while(task != NULL){
		if((task->pid | ((uint64_t)1<<63)) == pid) return task;
		task = task->next;
	}
	task = irqwaiting;
	while(task != NULL){
		if((task->pid | ((uint64_t)1<<63)) == pid) return task;
		task = task->next;
	}
	return NULL;
}
/*this might end up useless
 * I think that the way to implement semaphores is a linked list of processes attached to a semaphore rather than a centralised "waiting for semaphore" list, so a linked list of semaphores and each semaphore has a value and a first and last process
 * proberen() decrements semaphore value, if it's negative has the process sleep and go on the queue
 * verhogen() increments semaphore value, if it's negative tells the scheduler to wake up one process from the queue*/
SEMAPHORE * create_semaphore(int max_count){
	SEMAPHORE * semaphore = malloc(sizeof(SEMAPHORE)); //damn it, it's undergoing semantic satiation now
	semaphore->max_count = max_count;
	semaphore->current_count = 0;
	semaphore->first_waiting = NULL;
	semaphore->last_waiting = NULL;
	return semaphore;
}
void acquire_semaphore(SEMAPHORE * semaphore){
	lock_stuff();
	if(semaphore->current_count < semaphore->max_count){
		//we can acquire it
		semaphore->current_count ++;
	}else{
		//we have to wait
		current_task_TCB->next = NULL;
		if(semaphore->first_waiting == NULL){
			semaphore->first_waiting = current_task_TCB;
		}else{
			semaphore->last_waiting->next = current_task_TCB;
		}
		semaphore->last_waiting = current_task_TCB;
		block_task(STATE_WAITING_FOR_LOCK);
	}
	unlock_stuff();
}
void release_semaphore(SEMAPHORE * semaphore){
	lock_stuff();
	if(semaphore->first_waiting != NULL){
		//we need to wake up the first task waiting for the semaphore
		//semaphore->current count remains the same because we're replacing this task with another ^.^
		thread_control_block *task = semaphore-> first_waiting;
		semaphore->first_waiting = task->next;
		unblock_task(task);
	}else{
		//no tasks waiting
		semaphore->current_count --;
	}
	unlock_stuff();
}
void wfPokeT(thread_control_block ** p){ //lock_stuff() locks both task switching and irqs; lock_scheduler locks only task switching
	lock_stuff();
	if(!p || *p){
		ERROR(ERR_POKE_NULLP, (uint64_t)p);
	}
	*p = current_task_TCB;
	unlock_stuff();
	block_task(STATE_WAITING_FOR_POKE); //now all of our tasks are blocked and none are sleeping. the fucker decides to fuck shit up bc of ts
}
void PokeT(thread_control_block **p){
	lock_stuff();
	if(!p){
		ERROR(ERR_POKE_NULLP, (uint64_t)p);
	}
//	BREAK((uint64_t)FIRST_THREAD);
//	BREAK((uint64_t)current_task_TCB);
	if(*p != NULL){
		unblock_task(*p);
		*p = NULL;
	}else{
		BREAK((uint64_t)p);
	}
//	BREAK((uint64_t)FIRST_THREAD);
//	BREAK((uint64_t)current_task_TCB);
	unlock_stuff();
}
int wait_for_procupdate(uint64_t pid, uint64_t timeout){
	if(find_task_by_pid(pid) == NULL){
		return -2; //we're waiting for a task that doesn't exist
	}
	lock_stuff();
	current_task_TCB->pidW = pid;
	current_task_TCB->next = procwaiting;
	if(timeout == 0x0){
		current_task_TCB->ttn = 0x0;
	}else{
		current_task_TCB->ttn = TIME + timeout;
	}
	procwaiting = current_task_TCB;
	unlock_stuff();
	block_task(STATE_WAITING_FOR_PROC_UPDATE);
	if(current_task_TCB->irq_waiting_for == 0x1){
		current_task_TCB->irq_waiting_for = 0x0;
		return -1;
	}
	return 0;
}
void unblockP(uint64_t pid){
	thread_control_block * task = procwaiting;
	thread_control_block * next = NULL;
	uint64_t K = pid ^ ((uint64_t)1<<63);
	if(pid & (uint64_t)1<<63){
		lock_stuff();
		while(task != NULL){
			if(task->pidW == K){
				unblock_task(task);
			}else{
				task->next = procwaiting;
				procwaiting = task;
			}
			task = task->next;
		}
		unlock_stuff();
	}
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
		if( this_task -> state != STATE_WAITING){
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
void PROC_EXIT(){
	//block scheduling
	lock_scheduler();
	//remove ourselves from all the lists
	//free everything in the scheduler instead
	current_task_TCB->state = STATE_DEAD;
	//unblock scheduling
	unlock_scheduler();
	schedule();
}
