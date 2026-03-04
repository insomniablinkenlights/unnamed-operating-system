#include "stdint.h"
#ifndef HEADER_BRK
#define HEADER_BRK
typedef struct __attribute__((packed)) prog_mem{
	uint64_t start;
	uint64_t end;
}prog_mem;
#endif
