#include <stdlib.h>
#include "../../../../headers/error_types.h"
#include "standard.h"
#ifndef H_ADDRESSES_HOOK
#define H_ADDRESSES_HOOK
void * KPALLOC();
void * KPALLOCS(int64_t size);
void P_FREE(void * v_add);
void P_FREES(void * v_add, int64_t UNUSED(l));
void ERROR(uint64_t code, uint64_t type);
void BREAK(uint64_t data);
uint32_t DIV64_32(uint64_t dd, uint32_t ds);
uint32_t MOD64_32(uint64_t dd, uint32_t ds);
void memfill(void * dest, uint64_t count);
#endif
