#ifndef DEVICE_H
#define DEVICE_H
#include "filesystem.h"
typedef struct __attribute__((packed)) DEVICE{
//	void  (*DRIVER_FUNCTION)(struct DEVICE *);
	uint64_t pid;
	//maybe port validation or smth
	stream * streamID;
	char * DEV_NAME;
}DEVICE;
void d0xD(char * name);
void d0xE(uint64_t errc, char * err_str);
void DEVICE_INIT_DEVICE_LIST();
void dProbe(char * name, char * argv);
stream * dOpen(char * name);
#endif
