#ifndef filesystem
#define filesystem
void CloseAllFds();
uint64_t OPEN(char * filename, uint64_t flags);
void READ(uint64_t fd, void * buffer, uint64_t len);
void CLOSE(uint64_t fd);
void SEEK(uint64_t fd, uint64_t pos);
void WRITE(uint64_t fd, void * buffer, uint64_t len);
void InitKernelFd();
#endif
