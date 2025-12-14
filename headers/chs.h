#ifndef chs
#define chs
uint64_t * floppy_read(uint64_t LBA, uint16_t len);
void floppy_write(uint64_t LBA, uint16_t len, void * data);
#endif
