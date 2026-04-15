#include <stdlib.h>
#include "../../../../headers/error_types.h"
#ifndef H_ADDRESSES_HOOK
#define H_ADDRESSES_HOOK
void * KPALLOC();
void * KPALLOCS(int64_t size);
void P_FREE(void * v_add);
void ERROR(uint64_t code, uint64_t type);
uint32_t DIV64_32(uint64_t dd, uint32_t ds);
uint32_t MOD64_32(uint64_t dd, uint32_t ds);
#endif
