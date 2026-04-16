#include "headers/filesystem_internal.h"
#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/chs_qfs.h"
#include "headers/string.h"
#include "headers/standard.h"
/*requirements:
	- void * read(uint64_t LBA, uint64_t disk, uint16_t len, void * m), LBA is offset to LBA_FS_BASE
	- void write(uint64_t LBA, uint64_t disk, uint16_t len, void * data), LBA offset to LBA_FS_BASE once again
*/

//TODO: semaphores for r+ and w+
void * readPLUS(uint64_t offset, uint64_t size, uint64_t drive, void * buffer, void * buffer2){ //destroys buffer2
	int lba1 = offset>>9;
	int lba2 = (offset+size)>>9;
	int o_prov = 0;
	if(!buffer2){
		buffer2 = KPALLOCS(((lba2-lba1+1)>>3)+((((lba2-lba1+1))&0x7)?1:0));
		o_prov=1;
	}
	read(lba1, drive, (1+(lba2-lba1))<<9, buffer2);
	if(!buffer) buffer = malloc(size);
	memcpy(buffer, ((char*)buffer2)+offset, size);
	if(o_prov) P_FREES(buffer2,((lba2-lba1+1)>>3)+((((lba2-lba1+1))&0x7)?1:0));
	return buffer;
}
void writePLUS(uint64_t offset, uint64_t size, uint64_t drive, void * data, void * buffer2){ //destroys buffer2
	int lba1 = offset>>9;
	int lba2 = (offset+size)>>9;
	int o_prov = 0;
	if(!buffer2){
		buffer2 = KPALLOCS(((lba2-lba1+1)>>3)+((((lba2-lba1+1))&0x7)?1:0));
		o_prov=1;
	}
	read(lba1, drive, (1+(lba2-lba1))<<9, buffer2);
	memcpy(((char*)buffer2)+offset, data, size);
	write(lba1, drive, (1+(lba2-lba1))<9, buffer2);
	if(o_prov) P_FREES(buffer2,((lba2-lba1+1)>>3)+((((lba2-lba1+1))&0x7)?1:0));
}
struct vnode{
	inode a;
	uint64_t id;
	bool exists;
};
struct vnode * kINb = NULL;
uint64_t kINb_sz = 0;
inode * kINb_gb(uint64_t id){
	if(id>=DIV64_32(kINb_sz*0x1000,sizeof(struct vnode))) return NULL;
	if(kINb[id].exists){
		inode * mb = malloc(sizeof(inode));
		memcpy(mb,&( kINb[id].a), sizeof(inode));
		return mb;
	}
	return NULL;
}
inode * kINb_ad(inode * a, uint64_t id){ //MP-BREAK
	if(id>=DIV64_32(kINb_sz*0x1000,sizeof(struct vnode))){
		struct vnode * kINb2 = KPALLOCS(kINb_sz*2+1);
		memfill(((char*)kINb2)+(kINb_sz<<12),(kINb_sz+1)<<12);
		memcpy(kINb2, kINb, kINb_sz<<12);
		kINb = kINb2;
		kINb_sz = kINb_sz*2+1;
	}
	if(!kINb[id].exists){
		kINb[id].exists	=1;
		kINb[id].id = id;
		memcpy(&(kINb[id].a), a,sizeof(inode));
	}
	return a;
}
void kINb_invalidate(uint64_t id){
	if(id>=DIV64_32(kINb_sz*0x1000,sizeof(struct vnode))) return;
	memfill(&(kINb[id]), sizeof(struct vnode));
}
inode * GrabInode(uint64_t id){
        int r = sizeof(inode);
        int k2 = id;
	inode * res = NULL;
	res = kINb_gb(id);
	if(!res)
		res = kINb_ad((inode*)readPLUS((k2)*(r), r, 0, NULL, NULL), id);
	return res;
}
void InodeRead(inode *n, uint64_t position, void * buffer, uint64_t len){
        if((position+len)>(n->chunklen<<9)){
                ERROR(ERR_FILE_PASTBOUND, position);
        }
        read(n->chunkaddr1+(position>>9), 0x0, len, buffer); //TODO: drive numbers
}
uint64_t getI(uint64_t disk){ //TODO: this breaks if called twice at once.
	void * b = KPALLOC();
	for(int i = 0; ; i++){
		read(i, disk, 0x400, b);
		for(uint32_t j = 0; j<DIV64_32(0x200,sizeof(inode)); j++){
			if(((inode*)b)[j].chunklen == 0){
				P_FREE(b);
				return j+i*DIV64_32(0x200,sizeof(inode));
			}
		}
	}
	//Unreachable.
}
uint64_t getIS(uint64_t disk, uint64_t size){
	void * b = KPALLOC();
	unsigned int lbamax = 0;
	for(int i = 0; ; i++){
		read(i, disk, 0x400, b);
		for(uint32_t j = 0; j<DIV64_32(0x200,sizeof(inode)); j++){
			if(((inode*)b)[j].chunklen == 0){
				P_FREE(b);
				return lbamax-size;
			}else{
				lbamax = MAX(lbamax, ((inode*)b)[j].chunkaddr1+((inode*)b)[j].chunklen);
			}
		}
	}
}
void writeIN(inode * a, uint64_t disk, uint64_t no){ //TODO: ineffective and reads the whole disk!
	void * b =read(no*DIV64_32(0x200, sizeof(inode)), disk, 0x400, NULL);
	memcpy(&(((inode*)b)[no]), a, sizeof(inode));
	write(no*DIV64_32(0x200, sizeof(inode)), disk, 0x400,  b);
	P_FREE(b);
}
inode * mkFile(void * data, uint64_t length, uint64_t disk, char * name, uint16_t perms){
	inode * cons = malloc(sizeof(inode));
	cons->chunkaddr1=getIS(disk, (length>>9)+!!(length&0xFFF));
	cons->chunklen = (length>>9)+!!(length&0xFFF);
	cons->perms = perms;
	memfill(cons->name, 32);
	strcpy(cons->name, name);
	writeIN(cons,disk,getI(disk));
	write(cons->chunkaddr1, disk, length, data);
	return cons;	
}
uint64_t FileReaderStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
        inode * k =*((inode**)arguments); 
        InodeRead(k, position, buffer, len);
        return position+len;
}
uint64_t FileWriterStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
        inode * k =*((inode**)arguments); 
        if(position + len < k->chunklen*0x200){
                char * m = (char*)read(k->chunkaddr1+DIV64_32(position,512), 0x0, (len&0x1FF)?((len&(~0x1FF))+1):len, NULL);
                memcpy(m+position, buffer, len);
                write(k->chunkaddr1+DIV64_32(position,512), 0x0, (len&0x1FF)?((len&(~0x1FF))+1):len, m);
        }else{
                //we have to reallocate the inode, so like about 50% of the contents of insertFileSystem
                ERROR(ERR_FWS_UNIMP, 0);
        }
        return position+len;
}
uint64_t FileStream(void * arguments, uint64_t position, void * buffer, uint64_t len, uint8_t rw){
	inode * k;
	switch(rw){
		case 0x0:
                	return FileReaderStream(arguments, position, buffer, len);
		case 0x1:
                	return FileWriterStream(arguments, position, buffer, len);
		case 0x2:
			free(arguments);        
			return 0x0;
		case 0x3:
			k = *((inode**)arguments);
			len = k->chunklen<<9;
			return len;
		default:
			ERROR(ERR_FILESTREAM_RW, rw);
			return 0x0;
	}
}
inode * getFileFromFilename(inode * basedir, const char * filename){
        //   filename:    /DIR/DIR/DIR.../file or /file or /DIR/ or /DIR/DIR/
        //   split filename into directories ... filename
        char ** directories = NULL;
        char * name = NULL;
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
                        if(filename[i] == 0x0){
                        //       ERROR(ERR_FILE_DIRSTREAM, (uint64_t)filename);
                                return NULL;
                        }
                }else{
                        i++;
                }
                e = i; //TODO: fencepost error?
                       //filename[e] = 0x0
        }
        slashcount --; //root
        if(slashcount <0){
                slashcount=0;
        }
        i = 1; //TODO: breaks if starts with two slashes
        if(slashcount != 0x0){
                directories = (char**)malloc(slashcount*sizeof(char*));
                k = i;
                l = i;
                j=0; 
                while(j<slashcount){
                        k = i;
                        l = i;

                        while(filename[i] != '/'){
                                i++;
                                if(filename[i] == '\0'){
                //                      ERROR(ERR_FILE_OPEN_SLASHCOUNT, 0x0);
                                        return NULL;
                                }
                        }
                        directories[j] = (char*)malloc(i-k+1);
                        while(l<i){
                                if(l-k < 0){
                //                      ERROR(ERR_FILE_OPEN_WHY, 0x0);
                                        return NULL;
                                }
                                directories[j][l-k] = filename[l];
                                l++;
                        }
                        directories[j][i-k] = '\0';
                        i++; 
                        j++;
                }
        }
        i = e;
        while(filename[i-1] != '/'){
                i--;
        }
        name = (char*)malloc(e-i+1);
        memcpy(name, filename+i, e-i+1); 
        name[e-i] = 0x0; 
        
        inode * curdir = (inode*)malloc(sizeof(inode));
        memcpy(curdir, basedir, sizeof(inode));
        j=0;
        k = 0;

        int p = 1;
        void * m = KPALLOC();
        inode * n = NULL;
        while(j<slashcount){
                l = 0;
                InodeRead(curdir, 0x0, m, 0x200); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
                p=1;
                while(((uint64_t*)m)[l] != 0x0){
                        if(n!=NULL){
                              free(n);
			      n=NULL;
                        }
                        n=GrabInode(((uint64_t*)m)[l]);
                        p=strcmp(n->name, directories[j]);
                        if(p==0){
                                break;
                        }
                        l++;
                }
                if(p!=0){
               //       ERROR(ERR_IN_DIR_DNE, (uint64_t)filename);
                        return NULL;
                }
             	free(curdir); 
                curdir = n;
                j++;
        }

        if(strcmp(name, filename+i)){
                //ERROR(ERR_SPLIT_FAIL, (uint64_t)name);
                return NULL;
        }
        k = 0;
        l = 0;
        InodeRead(curdir, 0x0, m, 0x200); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
        p=1;
        while(((uint64_t*)m)[l] != 0x0){
                n=GrabInode(((uint64_t*)m)[l]);
                p=strcmp(n->name, name);
                if(p==0){
                        break;
                }
                l++;
		free(n);
        }
        if(p!=0){
        //      ERROR(ERR_IN_FILE_DNE,(uint64_t) name);
                return NULL;
        }
        
        P_FREE(m);
	for(l = 0; l<slashcount; l++){
		free(directories[l]);
	}
        if(slashcount>0)free(directories);
        free(name);
        if(curdir != basedir)free(curdir);
        return n;
}
