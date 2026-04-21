#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "../headers/filesystem_internal.h"
#include "../../headers/standard.h"
FILE * discimg;
void * read(uint64_t LBA, uint64_t disk, uint16_t len, void * m){
	if(len & 0x1ff) printf("inv r size %i\n", len);
	printf("read LBA %lu disk %lu len %i to %s\n", LBA, disk, len, m?"buffer":"new");
	if(!m) m = malloc(len);
	if(fseek(discimg, LBA*0x200+108*0x200, SEEK_SET)){
		printf("\tfseek fail\n");
	}
	fread(m, len, 1, discimg);
	return m;
}
void write(uint64_t LBA, uint64_t disk, uint16_t len, void * data){
	if(len & 0x1ff) printf("inv w size %i\n", len);
	printf("write LBA %lu disk %lu len %i from %lu\n", LBA, disk, len, (uint64_t)data);
	if(fseek(discimg, LBA*0x200+108*0x200, SEEK_SET)){
		printf("\tfseek fail\n");
	}
	if(fwrite(data, 1, len, discimg) !=  len){
		printf("\tfwrite fail\n");
	}
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
void * KPALLOCS(int64_t size){
	return malloc(0x1000*size);
}
void P_FREE(void * v_add){
	free(v_add);
}
void P_FREES(void * v_add, int64_t UNUSED(l)){
	free(v_add);
}
void ERROR(uint64_t code, uint64_t type){
	printf("error %lu, %lu\n", code, type);
}
void BREAK(uint64_t data){
	printf("BREAK: %lu\n", data);
	//if(!IsBadReadPtr((void*)data)){
	//printf("uint64_t [0]: %lu\n", *((uint64_t*)data));
	//}
}
void memfill(void * dest, uint64_t count){
	for(unsigned int i = 0; i<count; i++){
		((char*)dest)[i] = 0;
	}
}
inode * GrabInode(uint64_t id);
void printInode(inode * k){
	printf("'%s': %lu + %lu, p %i, o %i, g %i, T%lu\n", k->name, k->chunkaddr1, k->chunklen, k->perms, k->owner, k->group, k->timestamp);
}
#include <string.h>
void InodeRead(inode *n, uint64_t position, void * buffer, uint64_t len);
#include "../headers/proc.h"
void acquire_semaphore(SEMAPHORE * semaphore){
	semaphore = semaphore;
}
void release_semaphore(SEMAPHORE * semaphore){
	semaphore = semaphore;
}
SEMAPHORE * create_semaphore(int max_count){
	SEMAPHORE * a = malloc(sizeof(SEMAPHORE));
	a->max_count = max_count;
	return a;
}
struct vnode{
	inode a;
	uint64_t id;
	uint64_t disk;
	bool exists;
};
struct vnode * mkVN(inode * a, int id, int disk);
struct vnode * mkFile(void * data, uint64_t length, uint64_t disk, char * name, uint16_t perms, struct vnode ** links);
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
	char * m [] = {"/etc/keymap", "/etc/keymap", "/sbin/init", "/sbin/sh", "/sbin/echo", NULL};
	for(int i = 0; m[i]; i++){
		inode * k = getFileFromFilename(a, m[i]);
		if(!k){
			printf("%s not found!\n", m[i]);
		}
		printInode(k);
		char * M = malloc((k->chunklen<<9)+1);
		char * M2 = malloc((k->chunklen<<9)+1);
		InodeRead(k, 0, M, k->chunklen<<9);
		char * v2 = "../../sysroot";
		char * v = malloc(strlen(v2)+1+strlen(m[i]));
		strcpy(v, v2);
		strcat(v, m[i]);
		FILE * f2 = fopen(v, "rb");
		if(!f2) printf("%s DNE\n", v);
		int fl = fread(M2, k->chunklen<<9, 0x1, f2);
		for(int j = 0; j<fl; j++){
			if(M2[j] != M[j]){
				printf("mismatch in %s...\n", m[i]);
			}
		}
		fclose(f2);
		free(M);
		free(M2);
		free(k);
		free(v);
	}
	printf("done with dir checks. test MKfile:\n");
	char * data = calloc(1,0x200);
	for(int i = 0; i<0x200; i++){
		data[i]=0x34;
	}
	struct vnode ** links = calloc(2, sizeof(struct vnode));
	links [0] = mkVN(a, 0, 0);
	mkFile(data, 0x200, 0, "txt", 0x0, links);
	printf("mkf cmp\n");
	printInode(getFileFromFilename(a, "/txt"));
	free(a);
	fclose(discimg);
	return 0;
}

