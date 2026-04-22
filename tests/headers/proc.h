#ifndef H_PROC
#define H_PROC
typedef struct SEMAPHORE{
	int64_t max_count;
	int64_t current_count;
	void * first_waiting;	
	void * last_waiting;
} SEMAPHORE;
void acquire_semaphore(SEMAPHORE * semaphore);
void release_semaphore(SEMAPHORE * semaphore);
SEMAPHORE * create_semaphore(int max_count);
#endif
