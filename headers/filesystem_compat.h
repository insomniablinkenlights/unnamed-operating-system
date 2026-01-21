/*this is NOT for internal use!*/
#ifndef filesystem_compat
#define filesystem_compat
#define LBA_FS_BASE 72
typedef struct __attribute__((packed))inode{
	uint64_t chunkaddr1;
	uint64_t chunklen;
	uint16_t perms;
	uint8_t owner;
	uint8_t group;
	uint64_t timestamp;
	char name [32];
} inode;
#endif
