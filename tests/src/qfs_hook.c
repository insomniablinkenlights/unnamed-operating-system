#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "../headers/filesystem_internal.h"
FILE * discimg;
void * read(uint64_t LBA, uint64_t disk, uint16_t len, void * m){
	printf("read LBA %lu disk %lu len %i to %s\n", LBA, disk, len, m?"buffer":"new");
	if(!m) m = malloc(len);
	fseek(discimg, LBA*0x200+108*0x200, SEEK_SET);
	fread(m, len, 1, discimg);
	return m;
}
void write(uint64_t LBA, uint64_t disk, uint16_t len, void * data){
	printf("write %lu %lu %i %lu\n", LBA, disk, len, (uint64_t)data);
	fseek(discimg, LBA*0x200+108*0x200, SEEK_SET);
	fwrite(data, len, 1, discimg);
}
uint32_t DIV64_32(uint64_t dd, uint32_t ds){
	return dd / ds;
}
uint32_t MOD64_32(uint64_t dd, uint32_t ds){
	return dd % ds;
}
void * KPALLOC(){
	return malloc(0x1000);
}
void P_FREE(void * v_add){
	free(v_add);
}
void ERROR(uint64_t code, uint64_t type){
	printf("error %lu, %lu\n", code, type);
}
inode * GrabInode(uint64_t id);
void printInode(inode * k){
	printf("'%s': %lu + %lu, p %i, o %i, g %i, T%lu\n", k->name, k->chunkaddr1, k->chunklen, k->perms, k->owner, k->group, k->timestamp);
}
int main(int argc, char ** argv){
	if(argc!=2){
		printf("[QFS_TEST] need image!\n");
		exit(1);
	}else{
		discimg = fopen(argv[1], "rb+");
		if(!discimg){
			printf("[QFS_TEST] image DNE!\n");
			exit(1);
		}
	}
	inode * a = GrabInode(0);
	inode * k = getFileFromFilename(a, "/etc/keymap");
	printInode(k);
	free(a);
	free(k);
	fclose(discimg);
	return 0;
}

