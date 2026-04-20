#include "headers/filesystem_internal.h"
#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/chs_qfs.h"
#include "headers/string.h"
#include "headers/standard.h"
#include "headers/proc.h"
/*requirements:
	- void * read(uint64_t LBA, uint64_t disk, uint16_t len, void * m), LBA is offset to LBA_FS_BASE
	- void write(uint64_t LBA, uint64_t disk, uint16_t len, void * data), LBA offset to LBA_FS_BASE once again
*/

SEMAPHORE * disk0 = NULL;
void * readPLUS(uint64_t offset, uint64_t size, uint64_t drive, void * buffer, void * buffer2){ //destroys buffer2
	if(!disk0){
		disk0 = create_semaphore(1);
	}
	acquire_semaphore(disk0);
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
	release_semaphore(disk0);
	return buffer;
}
void writePLUS(uint64_t offset, uint64_t size, uint64_t drive, void * data, void * buffer2){ //destroys buffer2
	if(!disk0){
		disk0 = create_semaphore(1);
	}
	acquire_semaphore(disk0);
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
	release_semaphore(disk0);
}
struct vnode{
	inode a;
	uint64_t id;
	uint64_t disk;
	bool exists;
};
struct vnode * kINb = NULL;
uint64_t kINb_sz = 0;
inode * kINb_gb(uint32_t id){
	int32_t s = sizeof(struct vnode);
	int32_t in_s = sizeof(inode);
	if(id>=DIV64_32(kINb_sz<<12,s)) return NULL;
	struct vnode * M = (struct vnode*)(((char*)kINb)+(s*id));
	if(M->exists){
		inode * mb = malloc(s);
		memcpy(mb,&( M->a), in_s);
		return mb;
	}
	return NULL;
}
inode * kINb_ad(inode * a, uint64_t id){ //MP-BREAK
	uint64_t s = sizeof(struct vnode);
	uint64_t in_s = sizeof(inode);
	if(id>=DIV64_32(kINb_sz<<12,s)){
		struct vnode * kINb2 = KPALLOCS((kINb_sz<<1)+1);
		//memfill(((char*)kINb2)+(kINb_sz<<12),(kINb_sz+1)<<12);
		memfill(kINb2, (kINb_sz<<1)+1);
		if(kINb) memcpy(kINb2, kINb, kINb_sz<<12);
		kINb = kINb2;
		kINb_sz = (kINb_sz<<1)+1;
	}
	struct vnode * M = (struct vnode*)(((char*)kINb)+(s*id));
	if(!M->exists){
		M->exists	=true;
		M->id = id;
		M->disk = 0;
		memcpy(&(M->a), a,in_s);
	}
	return a;
}
void kINb_invalidate(uint64_t id){
	uint64_t s = sizeof(struct vnode);
	if(id>=DIV64_32(kINb_sz<<12,s)) return;
	struct vnode * M = (struct vnode*)(((char*)kINb)+(s*id));
	memfill(M, s);
}
inode * GrabInode(uint64_t id){
        int r = sizeof(inode);
        int k2 = id;
	inode * res = NULL;
	res = kINb_gb(id);
	//if(k2 == 3){
	//	BREAK(k2);
	//	BREAK((uint64_t)res);
	//}
	if(!res)
		res = kINb_ad((inode*)readPLUS((k2)*(r), r, 0, NULL, NULL), id);
	//if(res->chunkaddr1 != kINb_gb(id)->chunkaddr1){
	//	BREAK(0x422);
	//}
	return res;
}
void InodeRead(inode *n, uint64_t position, void * buffer, uint64_t len){
        if((position+len)>(n->chunklen<<9)){
                ERROR(ERR_FILE_PASTBOUND, position);
        }
	if(!disk0){
		disk0 = create_semaphore(1);
	}
	acquire_semaphore(disk0);
        read(n->chunkaddr1+(position>>9), 0x0, len, buffer); //TODO: drive numbers
	release_semaphore(disk0);
}
uint64_t getI(uint64_t disk){ //TODO: this breaks if called twice at once.
	if(!disk0){
		disk0 = create_semaphore(1);
	}
	acquire_semaphore(disk0);
	void * b = KPALLOC();
	for(int i = 0; ; i++){
		read(i, disk, 0x400, b);
		for(uint32_t j = 0; j<DIV64_32(0x200,sizeof(inode)); j++){
			if(((inode*)b)[j].chunklen == 0){
				P_FREE(b);
				release_semaphore(disk0);
				return j+i*DIV64_32(0x200,sizeof(inode));
			}
		}
	}
	//Unreachable.
}
uint64_t getIS(uint64_t disk, uint64_t size){
	void * b = KPALLOC();
	unsigned int lbamax = 0;
	if(!disk0){
		disk0 = create_semaphore(1);
	}
	acquire_semaphore(disk0);
	for(int i = 0; ; i++){
		read(i, disk, 0x400, b);
		for(uint32_t j = 0; j<DIV64_32(0x200,sizeof(inode)); j++){
			if(((inode*)b)[j].chunklen == 0){
				P_FREE(b);
				release_semaphore(disk0);
				return lbamax-size;
			}else{
				lbamax = MAX(lbamax, ((inode*)b)[j].chunkaddr1+((inode*)b)[j].chunklen);
			}
		}
	}
}
void writeIN(inode * a, uint64_t disk, uint64_t no){
	writePLUS(no*sizeof(inode), sizeof(inode), disk, a, NULL);
}
void addtoD(struct vnode * dir, struct vnode * toadd){ //RLC-BREAK
	uint64_t * b = read(dir->a.chunkaddr1, dir->disk, dir->a.chunklen, NULL);
	int last = 0;
	for(last = 0; b[last]; last++)
	b[last] = toadd->id;
	write(dir->a.chunkaddr1, dir->disk, dir->a.chunklen, b);
	kINb_invalidate(dir->id);
	P_FREES(b, dir->a.chunklen);
}
struct vnode * mkFile(void * data, uint64_t length, uint64_t disk, char * name, uint16_t perms, struct vnode ** links){
	struct vnode * cons = malloc(sizeof(struct vnode));
	cons->a.chunkaddr1=getIS(disk, (length>>9)+!!(length&0xFFF));
	cons->a.chunklen = (length>>9)+!!(length&0xFFF);
	cons->a.perms = perms;
	memfill(cons->a.name, 32);
	strcpy(cons->a.name, name);
	writeIN(&(cons->a),disk,getI(disk));
	write(cons->a.chunkaddr1, disk, length, data);
	for(int i = 0; links[i]; i++){
		//how do we invalidate every reference to a vnode?
		//I _think_ it's fine if, for now, we just invalidate the cached version
		addtoD(links[i], cons);
	}
	return cons;	
}
uint64_t FileReaderStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
        inode * k =*((inode**)arguments); 
        InodeRead(k, position, buffer, len);
        return position+len;
}
uint64_t FileWriterStream(void * arguments, uint64_t position, void * buffer, uint64_t len){ //RLC-BREAK
        inode * k =*((inode**)arguments); 
        if(position + len < k->chunklen<<9){
                //char * m = (char*)read(k->chunkaddr1+DIV64_32(position,512), 0x0, (len&0x1FF)?((len&(~0x1FF))+1):len, NULL);
                //memcpy(m+position, buffer, len);
                //write(k->chunkaddr1+DIV64_32(position,512), 0x0, (len&0x1FF)?((len&(~0x1FF))+1):len, m);
		writePLUS((k->chunkaddr1<<9)+position,len,0,buffer,NULL);
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
                               ERROR(ERR_FILE_DIRSTREAM, (uint64_t)filename);
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
