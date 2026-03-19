#ifndef usermode
#define usermode
void run_EXE(char * name);
void * VERIFY_USER(void * rdx);
uint8_t VERIFY_FLAGS(uint64_t rdx);
void UM_CLEANUP();
void loadRSP0(uint64_t rsp);
uint64_t ExecFile(char * A, char * argv);
void switchToUserModeProc(void * UP, int rdi);
#endif
