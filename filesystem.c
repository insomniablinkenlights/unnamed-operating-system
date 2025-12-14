#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/chs.h"
uint64_t * read(uint64_t LBA, uint64_t disk, uint16_t len){
	//TODO: drive numbers, len > 0x1000
	return floppy_read(LBA, len);
}
void write(uint64_t LBA, uint64_t disk, uint16_t len, void * data){
	//TODO: drive numbers, len> 0x1000
	floppy_write(LBA, len, data);
}
#define LBA_FS_BASE 64
typedef struct inode {
	uint64_t chunkaddr1; //chunksize 4kb
	uint64_t chunklen;
	uint16_t perms;
	uint8_t owner;
	uint8_t group;
	uint64_t timestamp; //nanoseconds after jan 1 1970
	char name[32];
}inode; //filename is at the beginning of file and terminated with \0
  //directories have perm bit 0x8 =1, and are lists of 64-bit inode numbers terminated with 0x0
void format(){
	void * m = KPALLOC();
	memfill(m, 0x1000);
(	(inode*)m)->chunkaddr1=1;
(	(inode*)m)->chunklen=1;
(	(inode*)m)->perms=0x8;
(	(inode*)m)->owner=0x0;
(	(inode*)m)->group=0x0;
(	(inode*)m)->timestamp=0x0;
	m+=sizeof(inode);
(	(inode*)m)->chunkaddr1=2;
(	(inode*)m)->chunklen=1;
(	(inode*)m)->perms=0x0;
(	(inode*)m)->owner=0x0;
(	(inode*)m)->group=0x0;
(	(inode*)m)->timestamp=0x0;
	memcpy( ((inode*)m)->name, "TXT\0", 4);
	m-=sizeof(inode);
	write(LBA_FS_BASE, 0x0, 0x1000, m);
	memfill(m, 0x1000);
*(	(uint64_t *) m) = 0x1;
*(	(uint64_t *) m+1 )= 0x0;
	write(LBA_FS_BASE+1, 0x0, 0x1000, m);
	memfill(m, 0x1000);
	memcpy((char*) m, "hello\0", 5); //TODO: double check memcpy works
	write(LBA_FS_BASE+2, 0x0, 0x1000, m);
	P_FREE(m);
}
//and now a function that reads the file into a buffer
//we'll implement IO as streams like in unix
typedef struct stream{
	uint64_t (* function)(void *, uint64_t, void *, uint64_t, uint8_t);
	void * arguments;
	uint64_t position;
	uint64_t flags;
}stream;
//which gives you a uint64_t function(void * arguments, uint64_t position, void * buffer, uint64_t len)
//every process has a fd table, where every fd maps to a stream for reading and/or a stream for writing
//for now we'll just have ONE FD for every kernel task
stream * kernelFd;
uint64_t kernelFdLen;
inode * root;
uint64_t kernelFdLastClosed = 0x0;
inode * GrabInode(uint64_t id);
void InitKernelFd(){
	kernelFd=KPALLOC();
	kernelFdLen=0x1000/sizeof(stream);
	root = GrabInode(0x0);
}
//the streams (for the case of files) map to ids of inodes
//so to read or write takes a fd which grabs the correct function from the table
//and open uses openfilename and pushes it
//TODO: and for output?
//TODO: rework kpalloc to have  second version which gives you continuous memory slabs
//TODO: KPFREE
inode * GrabInode(uint64_t id){
	void * m = read(LBA_FS_BASE+ ((sizeof(inode)*id)/0x1000), 0x0, 0x1000); //TODO: drive numbers
	if(((inode*)(m+sizeof(inode)*id))->chunkaddr1==0x0){
		ERROR(ERR_INODE_NE, id);
	}
	inode * k = malloc(sizeof(inode));
	memcpy(k, ((inode*)(m+sizeof(inode)*id)), sizeof(inode));
	P_FREE(m);
	return k;
}
void InodeRead(inode *n, uint64_t position, void * buffer, uint64_t len){
	if((position+len)>0x1000*n->chunklen){
		ERROR(ERR_FILE_PASTBOUND, position);
	}
	if(len>0x1000){
		ERROR(ERR_READ_SIZE_TEMP, len);
	}
	void * m = read(LBA_FS_BASE+n->chunkaddr1+(position/0x1000), 0x0, 0x1000); //TODO: drive numbers
	memcpy(buffer, m, len);
	P_FREE(m);
}
uint64_t FileReaderStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
	//arguments[0] = inode #
	inode * k =GrabInode(((uint64_t*)arguments)[0]); 
	InodeRead(k, position, buffer, len);
	free(k);
	return position+len;
}
uint64_t FileWriterStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
	inode * k =GrabInode(((uint64_t*)arguments)[0]); 
	InodeRead(k, position, buffer, len);
	free(k);
	return position+len;
}
uint64_t FileStream(void * arguments, uint64_t position, void * buffer, uint64_t len, uint8_t rw){
	if(rw == 0x0){
		return FileReaderStream(arguments, position, buffer, len);
	}else if(rw == 0x1){
		return FileWriterStream(arguments, position, buffer, len);
	}else if(rw == 0x2){
		//cleanup
		free(arguments);	
		return 0x0;
	}
}
stream * OpenFilename(inode * basedir, char * filename, uint64_t flags);
void READ(uint64_t fd, void * buffer, uint64_t len){
	if(fd>0x1000/sizeof(stream)){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].position = (kernelFd[fd].function)(kernelFd[fd].arguments, kernelFd[fd].position, buffer, len, 0x0);
}
void WRITE(uint64_t fd, void * buffer, uint64_t len){
	if(fd>0x1000/sizeof(stream)){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].position = (kernelFd[fd].function)(kernelFd[fd].arguments, kernelFd[fd].position, buffer, len, 0x1);
}
void SEEK(uint64_t fd, uint64_t pos){
	if(fd>kernelFdLen){
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].position = pos;
}
uint64_t OPEN(char * filename, uint64_t flags){
	stream * m = OpenFilename(root, filename, flags);
	while(kernelFdLastClosed<kernelFdLen && kernelFd[kernelFdLastClosed].function != NULL){
		kernelFdLastClosed++;
	}
	if(kernelFdLastClosed == kernelFdLen){
		kernelFdLastClosed = 0x0;
		while(kernelFdLastClosed<kernelFdLen && kernelFd[kernelFdLastClosed].function != NULL){
			kernelFdLastClosed++;
		}
		if(kernelFdLastClosed == kernelFdLen){
			ERROR(ERR_FD_TOOHIGH, kernelFdLastClosed);
		}
	}
	memcpy(kernelFd+kernelFdLastClosed, m, sizeof(stream));
	free(m);
	return kernelFdLastClosed;

}
void CLOSE(uint64_t fd){
	if(fd>kernelFdLen){ //TODO: fencepost
		ERROR(ERR_FD_TOOHIGH, fd);
	}
	if(kernelFd[fd].function == NULL){
		ERROR(ERR_FD_DNE, fd);
	}
	kernelFd[fd].function(kernelFd[fd].arguments, kernelFd[fd].position, NULL, 0x0, 0x2); //free it all up
	kernelFd[fd].function = NULL;
	kernelFdLastClosed = fd;
}
stream * OpenFilename(inode * basedir, char * filename, uint64_t flags){
	//   filename:    /DIR/DIR/DIR.../file or /file or /DIR/ or /DIR/DIR/
	//   split filename into directories ... filename
	char ** directories;
	char * name;
	int slashcount = 0;
	int i = 0;
	int e = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	while(filename[i] != 0x0){
		if(filename[i] == '/'){
			slashcount++;
			while(filename[i] == '/')i++;
			if(filename[i] == 0x0) ERROR(ERR_FILE_DIRSTREAM, (uint64_t)filename);
		}else{
			i++;
		}
		e = i; //TODO: fencepost error?
	}
	slashcount --; //root
	i = 1; //TODO: breaks if starts with two slashes
	if(slashcount != 0x0){
		directories = malloc(slashcount*sizeof(char*));
		k = i;
		l = i;
		j=0; //we do this earlier, but I assume that I'll break it and forget to fix this here because I don't read my own comments. If it still manages to break in the future I am very sorry but it is all your fault.
		while(j<slashcount){ //TODO: probably doesn't work at all; supposed to split the string
			k = i;
			l = i;
			while(filename[i] != '/'){
				i++;
			}
			directories[j] = malloc(i-k);
			while(l<i){
				directories[j][l-k] = filename[l];
			}
			i++;
			j++;
		}


	}
	name = malloc(e-i);
	memcpy(name, filename+i, e-i); //TODO: fencepost?

	// scan through all files in basedir, find one with correct name, repeat
	
	inode * curdir = basedir;
	j=0;
	k = 0;
	int o = 0;
	int p = 0;
	void * m = KPALLOC();
//	void * m2 = KPALLOC();
	inode * n;
	while(j<slashcount){
		k = 0;
		l = 0;
		InodeRead(curdir, 0x0, m, 0x1000); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
	//	while(((uint64_t*)m)[k++]!=0x0); //I'm not even going to bother parsing the name of curdir because we already did it.
		while(((uint64_t*)m)[k+l++] != 0x0){
			//get directory indicated by this inode, check name equality
			if(n!=NULL){
				free(n); n=NULL;
			}
			n=GrabInode(((uint64_t*)m)[k+l]);
		//	InodeRead(n, 0x0, m2, 0x1000);
			p=0;
			o=0;
			while(1){
				if(n->name[o] == 0x0 || directories[j][o] == 0x0){
					p=1;
					break;
				}
				if(n->name[o] != directories[j][o]){
					p=0; 
					break;
				}
				o++;
			}
			if(p){
				break;
			}
		}
		if(!p){
			ERROR(ERR_IN_DIR_DNE, (uint64_t)filename);
		}
		if(curdir != basedir) free(curdir);
		curdir = n;
		j++;
	}
	//finally, find the entry in our CURDIR which contains the thingy
	k = 0;
	l = 0;
	InodeRead(curdir, 0x0, m, 0x1000); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
//	while(((uint64_t*)m)[k++]!=0x0); //I'm not even going to bother parsing the name of curdir because we already did it.
	while(((uint64_t*)m)[k+l++] != 0x0){
		//get file indicated by this inode, check name equality
		if(n!=NULL)free(n);
		n=GrabInode(((uint64_t*)m)[k+l]);
	//	InodeRead(n, 0x0, m2, 0x1000);
		p=0;
		o=0;
		while(1){
			if(n->name[o] == 0x0 || name[o] == 0x0){
				p=1;
				break;
			}
			if(n->name[o] != name[o]){
				p=0; 
				break;
			}
			o++;
		}
		if(p){
			break;
		}
	}
	if(!p){
		ERROR(ERR_IN_FILE_DNE,(uint64_t) filename);
	}
	
	stream * k2 = malloc(sizeof(stream));
	k2->arguments = malloc(sizeof(uint64_t));
	((uint64_t*)k2->arguments)[0]=((uint64_t*)m)[k+l];
	k2->position = 0x0;
	k2->function = FileStream;
	k2->flags = flags;

	P_FREE(m);
//	P_FREE(m2);
	for(;slashcount>1;slashcount--){
		free(directories[slashcount-1]);
	}
	if(slashcount>0)free(directories);
	free(name);
	if(curdir != basedir)free(curdir);
	if(n!=NULL)free(n);
	return k2;
}
void start_init_task(){
	InitKernelFd();
	uint64_t id = OPEN("/TXT", 0x0);
	READ(id, UPALLOC(0x0), 0x1000);
	asm volatile("xchgw %bx, %bx");	
}
