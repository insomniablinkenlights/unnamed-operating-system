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
inode * GrabInode(uint64_t id){ //this code displeases me.
        int r = sizeof(inode);
        int k2 = id;
        void * m = read((r*k2)>>9, 0x0, 1024, NULL); //TODO: drive numbers
        inode * addr = (inode*)(((char*)m)+((r*k2)&511)); //TERRIBLE hack. WHY does imulq not work?
        if((void*)addr == m && id != 0){
                ERROR(ERR_INODE_NE2, k2*r);
        }
        if(addr->chunklen == 0x0){
        	ERROR(ERR_INODE_NE, id);
        }
        inode * k =(inode*) (malloc(r));
        memcpy(k, addr, r);
        P_FREE(m); //addr should be freed?
        return k;
}

void InodeRead(inode *n, uint64_t position, void * buffer, uint64_t len){
        if((position+len)>(n->chunklen<<9)){
                ERROR(ERR_FILE_PASTBOUND, position);
        }
        read(n->chunkaddr1+(position>>9), 0x0, len, buffer); //TODO: drive numbers
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
