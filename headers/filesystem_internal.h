#ifndef FS_INTERNAL
#define FS_INTERNAL
#include "filesystem.h"
#include "stdint.h"
typedef struct __attribute__((packed)) inode {
	uint64_t chunkaddr1; //chunksize 4kb
	uint64_t chunklen;
	uint16_t perms;
	uint8_t owner;
	uint8_t group;
	uint64_t timestamp; //nanoseconds after jan 1 1970
	char name[32];
}inode;
inode * getFileFromFilename(inode * basedir, const char * filename);
#endif
