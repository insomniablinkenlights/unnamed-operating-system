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
        inode * k = malloc(r);
        memcpy(k, addr, r);
        P_FREE(m);
        return k;
}

void InodeRead(inode *n, uint64_t position, void * buffer, uint64_t len){
        if((position+len)>(n->chunklen<<9)){
                ERROR(ERR_FILE_PASTBOUND, position);
        }
        read(n->chunkaddr1+(position>>9), 0x0, len, buffer); //TODO: drive numbers
}

uint64_t FileReaderStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
        inode * k =((void**)arguments)[0]; 
        InodeRead(k, position, buffer, len);
        return position+len;
}
uint64_t FileWriterStream(void * arguments, uint64_t position, void * buffer, uint64_t len){
        inode * k =((void**)arguments)[0]; 
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
	switch(rw){
		case 0x0:
                	return FileReaderStream(arguments, position, buffer, len);
		case 0x1:
                	return FileWriterStream(arguments, position, buffer, len);
		case 0x2:
			free(arguments);        
			return 0x0;
		case 0x3:
			inode * k = ((inode**)arguments)[0];
			uint64_t len = k->chunklen<<9;
			return len;
		default:
			ERROR(ERR_FILESTREAM_RW, rw);
			return 0x0;
	}
}
inode * getFileFromFilename(inode * basedir, const char * filename){ //honestly let's just not touch this LOL
        //   filename:    /DIR/DIR/DIR.../file or /file or /DIR/ or /DIR/DIR/
        //   split filename into directories ... filename
        // TODO: fix malloc and free here
        char ** directories = NULL;
        char * name = NULL;
        int slashcount = 0;
        int i = 0;
        int e = 0;
        int j = 0;
        int k = 0;
        int l = 0;
        char * FN2 = NULL;
        FN2++;
        if(FN2 != NULL+1){ //genuinely why the FUCK does this work
                //ERROR(ERR_OPEN_TWO, (uint64_t)FN2);
                return NULL;
        }
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
                //right here we enter an infinite loop...
                directories = malloc(slashcount*sizeof(char*));
                k = i;
                l = i;
                j=0; //we do this earlier, but I assume that I'll break it and forget to fix this here because I don't read my own comments. If it still manages to break in the future I am very sorry but it is all your fault.
                while(j<slashcount){ //TODO: probably doesn't work at all; supposed to split the string
                        k = i;
                        l = i;

                        while(filename[i] != '/'){
                                i++;
                                if(filename[i] == '\0'){
                //                      ERROR(ERR_FILE_OPEN_SLASHCOUNT, 0x0);
                                        return NULL;
                                }
                        }
                        directories[j] = malloc(i-k+1);
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
        //i is now the first character in the actual string
        name = malloc(e-i+1); //I think malloc is broken... 3:
        memcpy(name, filename+i, e-i+1); //TODO: fencepost?
        name[e-i] = 0x0; //for some reason it's not doing it
        if(strcmp(name, filename+i)){
                //ERROR(ERR_SPLIT_FAIL, (uint64_t)name);
                return NULL;
        }
        /*if(slashcount != 0x0){
                if(strcmp(directories[0], "dev" +CBASE) == 0){ //we have a device in /dev
                                                                //TODO: this breaks isolation
                        return openDEV(name);
                }
        }*/
//      name ++; //if this fixes it i will die
        // scan through all files in basedir, find one with correct name, repeat

        
        inode * curdir = malloc(sizeof(inode));
        memcpy(curdir, basedir, sizeof(inode));
        j=0;
        k = 0;

        int p = 1;
        void * m = KPALLOC();
//      void * m2 = KPALLOC();
        inode * n = NULL;
        while(j<slashcount){ //loop through all directories in path
                l = 0;
                InodeRead(curdir, 0x0, m, 0x200); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
                p=1;
                while(((uint64_t*)m)[l] != 0x0){
                        //loop through inodes indicated by directory
                        //get directory indicated by this inode, check name equality
                        if(n!=NULL){
                //              free(n); n=NULL;
                        }
                        n=GrabInode(((uint64_t*)m)[l]); //grab l'th inode in directory
                        p=strcmp(n->name, directories[j]);
                        if(p==0){
                                break; //it's this one
                        }
                        l++;
                }
                if(p!=0){
                //      ERROR(ERR_IN_DIR_DNE, (uint64_t)filename);
                        return NULL;
                }
        //      free(curdir); 
                                        //IM no longer freeing shit to stop this fuck ass double free
                curdir = n;
                j++;
        }

        if(strcmp(name, filename+i)){
                //ERROR(ERR_SPLIT_FAIL, (uint64_t)name);
                return NULL;
        }
        //finally, find the entry in our CURDIR which contains the thingy
        k = 0;
        l = 0;
        InodeRead(curdir, 0x0, m, 0x200); //TODO: to save myself time, I here assume a maximum length of directories. This obviously doesn't work, so I will have to reimplement this section of code later.
                                           //
//      while(((uint64_t*)m)[k++]!=0x0); //I'm not even going to bother parsing the name of curdir because we already did it.
        p=1;
        while(((uint64_t*)m)[l] != 0x0){ //m is a LIST of inodes.
                //get file indicated by this inode, check name equality
        //      if(n!=NULL)free(n); //okay so our new fucking problem is that N is 0x7 ???????????????? why the FUCK would it be 0x7..... im gonna kms im gonna km fucking s rn

                n=GrabInode(((uint64_t*)m)[l]); //should give 0x1
                                                  //this FINALLY WORKS !!! but next up, "TXT" still doesn't match bc the slash wasn't separated properly TODO
        //      InodeRead(n, 0x0, m2, 0x1000);
                p=strcmp(n->name, name);
                if(p==0){
                        break;
                }
                l++;
        }
        if(p!=0){
        //      ERROR(ERR_IN_FILE_DNE,(uint64_t) name);
                return NULL;
        }
        
//      BREAK(0x444);
//      BREAK((uint64_t)n);
        /*stream * k2 = malloc(sizeof(stream));
        k2->arguments = malloc(sizeof(uint64_t));
        ((uint64_t*)k2->arguments)[0]=((uint64_t*)m)[l];
        k2->position = 0x0;
        k2->function = FileStream;
        k2->flags = flags;
*/
        P_FREE(m);
//      P_FREE(m2);
/*      for(;slashcount>1;slashcount--){
                free(directories[slashcount-1]);
        }
        if(slashcount>0)free(directories);
        free(name);
        if(curdir != basedir)free(curdir);
        if(n!=NULL)free(n);*/
        return n;
}
