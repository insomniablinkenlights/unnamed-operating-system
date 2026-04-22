#ifndef usermode
#define usermode
#include "stdint.h"
void run_EXE(char * name);
void * VERIFY_USER(void * rdx);
uint64_t VERIFY_FLAGS(uint64_t rdx);
void UM_CLEANUP();
void loadRSP0(uint64_t rsp);
uint64_t ExecFile(char * A, char * argv, struct perm_desc * perms);
void switchToUserModeProc(void * UP, int rdi);
#endif
