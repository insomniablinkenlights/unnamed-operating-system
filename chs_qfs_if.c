#define LBA_FS_BASE 108
#include "headers/addresses.h"
#include "headers/stdint.h"
#include "headers/standard.h"
#include "headers/chs.h"
#include "headers/chs_qfs.h"
void * read(uint64_t LBA, uint64_t disk, uint16_t len, void * m){
        //TODO: drive numbers, len > 0x1000, queue?
        if(disk != 0x0){
                ERROR(ERR_READ_DISKINV, disk);
        }
        if(len&511){
                ERROR(ERR_READ_SIZE, len);
        }
        if(m == NULL){
                 m = KPALLOCS((len>>12) + 1);
        }
        for(uint32_t i = 0; i<len>>9; i++){
                floppy_read(LBA_FS_BASE+LBA+i, 512, disk, (char*)m+(i<<9));
        }
        return m;

}
void write(uint64_t LBA, uint64_t disk, uint16_t len, void * data){
        //TODO: drive numbers, len> 0x1000
        if(disk != 0x0){
                ERROR(ERR_WRITE_DISKINV, disk);
        }
        if(len&511){
                ERROR(ERR_WRITE_SIZE, len);
        }
        for(uint32_t i = 0; i<len>>9; i++){
                floppy_write(LBA_FS_BASE+LBA+i, len, (char*)data+(i<<9), disk);
        }
}
