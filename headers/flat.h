#ifndef flat
#define flat
#include "stdint.h"
typedef struct __attribute__((packed)){
	uint64_t TEXT_START;
	uint64_t TEXT_END;
	uint64_t TEXT_VADD;
	uint64_t DATA_START;
	uint64_t DATA_END;
	uint64_t DATA_VADD;
	uint64_t BSS_SIZE;
	uint64_t BSS_VADD;
}PFH;
void * PF(void * fi, uint64_t filesize);
#endif
