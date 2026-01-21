#ifndef usermode
#define usermode
void run_EXE(char * name);
void * VERIFY_USER(void * rdx);
uint8_t VERIFY_FLAGS(uint64_t rdx);
#endif
