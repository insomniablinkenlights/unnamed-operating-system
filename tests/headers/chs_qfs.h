#ifndef H_CHS_QFS_HOOK
#define H_CHS_QFS_HOOK
#include <stdint.h>
void * read(uint64_t LBA, uint64_t disk, uint16_t len, void * m);
void * write(uint64_t LBA, uint64_t disk, uint16_t len, void * data);
#endif
