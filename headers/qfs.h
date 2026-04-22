#ifndef H_QFS
#define H_QFS
inode * GrabInode(uint64_t id); //required for compatibility with the insecure interface in filesystem.c
uint64_t FileStream(void * arguments, uint64_t position, void * buffer, uint64_t len, uint8_t rw); //required for OPEN()
#endif
