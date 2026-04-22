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
struct streamDescriptor {
	stream * (*opener) (char * name);
	char * arguments;
};
union File_u {
	inode * in;
	struct streamDescriptor * sd;
};
struct File{
	union File_u content;
	uint8_t type;
};
#endif
