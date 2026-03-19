#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../headers/filesystem_compat.h"
FILE * f = NULL;
int errc=0;
uint64_t flll;
FILE * f2o(const char * a, char * b){
	FILE * c = fopen(a, b);
	if(!c){
		errc+=1;
		printf("[insertFileSystem] missing '%s'\n", a);
	}
	fseek(c, 0, SEEK_SET);
	return c;
}
inode constructInode(uint64_t chunkaddr1, uint64_t chunklen, uint16_t perms, uint8_t owner, uint8_t group, uint64_t timestamp, const char * name){
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
void wdiskr(uint64_t lba, uint64_t len, char * mrg){
	if(0x200*lba+0x200*len>flll){
		printf("[insertFileSystem] LBA > fileSIZE!\n");
		errc++;
	}
#ifdef IFS_DBG
	printf("write of size %lu chx to LBA %lu\n", len, lba);
#endif
	fseek(f, LBA_FS_BASE*0x200+0x200*lba, SEEK_SET);
	fwrite(mrg, 0x200, len, f);
}
int get_fl(const char * a){
	FILE * m = f2o(a, "rb");
	fseek(m, 0, SEEK_END);
	int b = ftell(m);
	fclose(m);
#ifdef IFS_DBG
	printf("%s len is %i\n", a, b);
#endif
	return b;
}
typedef struct dirORfile{
	struct dirORfile ** entries;
	uint64_t entriesLength;
	bool file;
	char * filecontentsifso;
	uint64_t filelengthifso;
	uint16_t perms;
	uint8_t group;
	uint8_t owner;
	uint64_t timestamp;
	char * name;
}dirORfile;
int numinodes = 0;
dirORfile * readFile(const char * name){
	numinodes++;
	dirORfile * L = calloc(1, sizeof(dirORfile));
	L->file = 1;
	struct stat * us = calloc(1,sizeof(struct stat));
#ifdef IFS_DBG
	printf("stat '%s'\n", name);
#endif
	stat(name, us);
	L->perms = us->st_mode;
	L->group = us->st_gid;
	L->owner = us->st_uid;
	L->timestamp= us->st_mtime;
	L->name = (char*)strrchr(name, '/')+1;
#ifdef IFS_DBG
	printf("L->name '%s'\n",L->name );
#endif
	char * n2 = calloc(strlen(L->name)+1,1);
	strcpy(n2, L->name);
	L->name = n2;
	int LEN = get_fl(name);
	FILE * m = f2o(name, "rb");
	L->filelengthifso=LEN;
	L->filecontentsifso=calloc(LEN+1,sizeof(char));
	fread(L->filecontentsifso, 0x1, LEN, m);
	fclose(m);
	free(us);
	return L;
}
dirORfile * readDir(const char * name){
	numinodes++;
	dirORfile * L = calloc(1, sizeof(dirORfile));
	L->entries = calloc(1, sizeof(dirORfile*));
	L->entriesLength = 1;
	DIR * m = opendir(name);
	if(!m){
		printf("[insertFileSystem] could not open %s!\n", name);
		errc++;
	}
	struct dirent* cont;
	struct stat * us = calloc(1, sizeof(struct stat));
	stat(name, us);
	L->file = 0;
	L->perms = us->st_mode;
	L->group = us->st_gid;
	L->owner = us->st_uid;
	L->timestamp = us->st_mtime;
	L->name = (char*)strrchr(name, '/')+1;
	if(strcmp(name, "./sysroot")==0){
#ifdef IFS_DBG
		printf("[insertFileSystem] renaming sysroot...\n");
#endif
		L->name = "";
	}
	//in some cases L->name has a length of 0!
	char * cna = calloc(strlen(L->name)+1, 1);
	memcpy(cna, L->name, strlen(L->name)+1);
	L->name = cna;
	cont = readdir(m);
	while(cont != NULL){
		if(strcmp(".", cont->d_name)==0 || strcmp("..", cont->d_name)==0){
			cont = readdir(m);
			continue;
		}
		char * K = calloc(strlen(name)+4+strlen(cont->d_name),1);
		if(strcmp("", name) == 0){
			strcpy(K, ".");
		}else{
			strcpy(K, name);
		}
		strcat(K, "/");
		strcat(K, cont ->d_name);
		stat(K, us);
		if(us == NULL){
			printf("[insertFileSystem] could not find %s\n", K);
			errc++;
		}
		if(S_ISREG(us->st_mode)){
			L->entries[L->entriesLength-1] = readFile(K);
		}else{
			L->entries[L->entriesLength-1] = readDir(K);
		}
		free(K);
		L->entries = realloc(L->entries, sizeof(dirORfile*)*(L->entriesLength+1));
		L->entriesLength++;
		cont = readdir(m);
	}
	closedir(m);
	free(us);
	return L;
}
int FIRST_UNU = 0;
int getdiskregion(int size){
	FIRST_UNU+=size;
	return FIRST_UNU-size;
}
int getsizeofDOF(const dirORfile * a){
	if(a->file){
	 	uint64_t b = ((a->filelengthifso)>>9) + ((a->filelengthifso&0x1ff)?1:0);
		return b;
	}
	else return (((a->entriesLength-1)*sizeof(inode))>>9) + ((((a->entriesLength-1)*sizeof(inode))&0x1ff)?1:0);
}
int lastfucker = 0; //inode index
uint64_t cDF(dirORfile *a, inode * inbase){
	int b = getsizeofDOF(a);
	char * m = calloc(0x200, b);
	int st = getdiskregion(b);
#ifdef IFS_DBG
	printf("cdf %s...\n", a->name);
#endif
	int ourindex = lastfucker++;
	uint64_t i = 0;
	if(a->file){
		if(a->filelengthifso > (uint64_t)b*0x200){
			printf("[insertFileSystem] %lu>%i\n",a->filelengthifso/0x200, b);
			errc++;
		}
		memcpy(m, a->filecontentsifso, a->filelengthifso);
	}else{
		for(i = 0; i<a->entriesLength-1; i++){
#ifdef IFS_DBG
			printf("cdf L %lu: %s btw last i# %i and last disk is %i and file len %i\n", i, a->entries[i]->name, lastfucker, FIRST_UNU, b);
#endif
			((uint64_t*)m)[i] = cDF(a->entries[i], inbase);
		}
	}
	wdiskr(st, b, m); //lba (*0x200), len (*0x200), mrg (char *)
	char * m2 = calloc(0x200, b);
	fseek(f, 0x200*LBA_FS_BASE+0x200*st, SEEK_SET);
	fread(m2, b, 0x200, f); 
	int hashadmismatch = 0;
	for(i = 0; i<(uint64_t)b<<6; i++){
		if(((uint64_t*)m2)[i] != ((uint64_t*)m)[i]){
			//printf("%lu != %lu, index %lu in %s\n", ((uint64_t*)m2)[i], ((uint64_t*)m)[i], i, a->name);
			hashadmismatch=1;
		}
	}
	if(hashadmismatch){
		printf("[insertFileSystem] mismatch in %s\n", a->name);
		errc++;
	}
	free(m2);
	free(m);
	inbase[ourindex] = constructInode(st, b, a->file?0x1:0x8, 0x0, 0x0, 0x0, a->name);
	if(!a->file){
		free(a->entries);
	}else{
		free(a->filecontentsifso);
	}
	
	free(a->name);
	free(a);
	return ourindex;
}
void shitfuckpiss2000(){
	dirORfile * root = readDir("./sysroot");
	int lnb = (((sizeof(inode)) * numinodes)>>9) + ((((sizeof(inode))*numinodes)&0x1ff)?1:0);
	if(lnb == 0){
		printf("0 inodes !\n");
		errc++;
	}
	FIRST_UNU += lnb;
	char * inba = calloc(lnb, 0x200);
#ifdef IFS_DBG
	printf("[insertFileSystem] time to cdf\n");
#endif
	cDF(root, (inode*)inba);
#ifdef IFS_DBG
	inode * k = (inode*)inba;
	for(int i = 0; i<lastfucker;i++){
		printf("%i: %lu, %lu, %i, %i, %i, %lu, \"%s\"\n", i, k[i].chunkaddr1, k[i].chunklen, k[i].perms, k[i].owner, k[i].group, k[i].timestamp, k[i].name);
		if(k[i].perms&0x8){
			char * m = calloc(k[i].chunklen, 0x200);
			fseek(f, 0x200*LBA_FS_BASE+0x200*k[i].chunkaddr1, SEEK_SET);
			fread(m, k[i].chunklen, 0x200, f); 
			for(uint64_t i2 = 0; i2<(k[i].chunklen<<6); i2++){
				printf("\t%lu\n", ((uint64_t*)m)[i2]);
			}
			free(m);
		}
	}
#endif
	wdiskr(0, lnb, inba);
	free(inba);
	printf("[insertFileSystem] done!\n");
}
void constructKeymap(){
	FILE*f2=f2o("./sysroot/etc/keymap","wb");
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
	free(K);
}
int main(){
	f = fopen("../floppya.img", "rb+");
	if(!f){
		return -1;
	}
	fseek(f, 0, SEEK_END);
	flll = ftell(f);
	fseek(f, 0, SEEK_SET);

	constructKeymap();
	shitfuckpiss2000();

	fclose(f);
	if(errc != 0){
		printf("%i ERRORS", errc);
		return -1;
	}
	return 0;
}
