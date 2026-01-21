#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/filesystem_compat.h"
FILE * f = NULL;
int errc=0;
FILE * f2o(char * a, char * b){
	FILE * c = fopen(a, b);
	if(!c){
		errc+=1;
		printf("[insertFileSystem] missing '%s'\n", a);
	}
	return c;
}
inode constructInode(uint64_t chunkaddr1, uint64_t chunklen, uint16_t perms, uint8_t owner, uint8_t group, uint64_t timestamp, char * name){
	inode m;
	m.chunkaddr1=chunkaddr1;
	m.chunklen=chunklen;
	m.perms=perms;
	m.owner=owner;
	m.group=group;
	m.timestamp=timestamp;
	strncpy(m.name, name, 32);
	return m;
}
void write(void * m, uint64_t LBA){
	fseek(f, LBA*0x200, SEEK_SET);
	fwrite(m, 0x1, 0x200, f);
}
void fuckstuffup(){
	//TODO: dirent
	char * m = calloc(0x200, 0x1);
	inode * k = ((inode*)m);
	k[0] = constructInode(1, 1, 8, 0, 0, 0, "");
	k[1] = constructInode(2, 1, 8, 0, 0, 0, "sbin");
	k[2] = constructInode(3, 1, 0x1, 0x0, 0x0, 0x0, "init");
	k[3] = constructInode(4, 1, 0x1, 0x0, 0x0, 0x0, "sh");
	k[4] = constructInode(5, 2, 0x1, 0x0, 0x0, 0x0, "txt");
	k[5] = constructInode(7, 1, 0x8, 0x0, 0x0, 0x0, "etc");
	k[6] = constructInode(8, 1, 0x1, 0x0, 0x0, 0x0, "keymap");
	write(m, LBA_FS_BASE);
	memset(m, 0x0, 0x200);
	uint64_t * k2 = (uint64_t*)m;
	k2[0] = 0x1;
	k2[1] = 0x5;
	k2[2] = 0x0;
	write(k2, LBA_FS_BASE+1);
	memset(m, 0x0, 0x200);
	k2[0] = 0x2;
	k2[1] = 0x3;
	k2[2] = 0x4;
	k2[3] = 0x0;
	write(k2, LBA_FS_BASE+2);
	memset(m, 0x0, 0x200);
	k2[0] = 0x6;
	k2[1] = 0x0;
	write(k2, LBA_FS_BASE+7);


	memset(m, 0x0, 0x200);
	FILE * f2 = f2o("./init.bin", "rb");
	fread(m, 1, 0x200, f2);
	fclose(f2);
	write(m, LBA_FS_BASE+3);
	memset(m, 0x0, 0x200);
	f2 = f2o("./sh.bin", "rb");
	fread(m, 1, 0x200, f2);
	fclose(f2);
	write(m, LBA_FS_BASE+4);
	memset(m, 0x0, 0x200);
	f2 = f2o("../userland/txt", "rb");
	fread(m, 1, 0x400, f2);
	fclose(f2);
	write(m, LBA_FS_BASE+5);
	write(m+0x200, LBA_FS_BASE+6);
	memset(m, 0x0, 0x200);
	f2 = f2o("./keymap", "rb");
	fread(m, 1, 0x200, f2);
	fclose(f2);
	write(m, LBA_FS_BASE+8);
}
void constructKeymap(){
	FILE*f2=f2o("./keymap","wb");
	fseek(f2, 0, SEEK_SET);
	char qwerty[] = {
		0x0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8','9','0','-','=', 0x08 /*0x0e, backspace*/, '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0x0 /*lctrl, should be handled by drivers*/, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0x0 /*lshft, should be handled by drivers*/, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',','.','/',0x0/*rshft, should be handled by drivers*/,'*', 0x0 /*lalt*/, ' ', 0x0 /*caps*/, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'}; //everything after 0x53 is zero...
	char qwertySHIFT[] = {
		0x0, 0x1B, '!', '@', '#', '$', '%', '^', '&', '*','(',')','_','+', 0x08 /*0x0e, backspace*/, '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0x0 /*lctrl, should be handled by drivers*/, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0x0 /*lshft, should be handled by drivers*/, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<','>','?',0x0/*rshft, should be handled by drivers*/,'*', 0x0 /*lalt*/, ' ', 0x0 /*caps*/, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'}; //everything after 0x53 is zero...
	char * K = malloc(0x200);
	memset(K, 0x0, 0x200);
	memcpy(K, qwerty, 0x53);
	memcpy(K+0x80, qwertySHIFT, 0x53);
	memcpy(K+0x100, qwerty, 0x53);
	memcpy(K+0x180, qwertySHIFT, 0x53);
	fwrite(K, 0x1, 0x200, f2);
	fclose(f2);
}
int main(){
	f = fopen("../floppya.img", "rb+");
	if(!f){
		return -1;
	}
	fseek(f, 0, SEEK_END);
	uint64_t fl = ftell(f);
	fseek(f, 0, SEEK_SET);

	constructKeymap();
	fuckstuffup();	

	fclose(f);
	return 0;
}
