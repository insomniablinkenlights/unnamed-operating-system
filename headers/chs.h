#ifndef chs
#define chs
uint64_t * floppy_read(uint64_t LBA, uint16_t len, uint8_t disk);
void floppy_write(uint64_t LBA, uint16_t len, void * data, uint8_t disk);
#endif
