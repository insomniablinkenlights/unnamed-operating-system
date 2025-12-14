#include "headers/stdint.h"
#include "headers/addresses.h"
void lba_2_chs(uint32_t lba, uint8_t* cyl, uint8_t* head, uint8_t* sector){
	if(lba != 0x0)
		asm volatile("xchgw %bx, %bx");
	*cyl = lba / (36);
	*head = ((lba %(2*18))/18);
	*sector = ((lba % (2*18))%18 +1);
}
/* i think we should uuse ISA DMA (!= PCI DMA) instead of PIO
 * - set up DMA channel 2 (total transfer bytecount -1, target buffer physical address, transfer direction)
 * - init/reset controller if needed
 * - select drive if needed
 * - set up floppy controller for DMA using specify command
 * - seek to correct cylinder
 * - issue a sense interrupt command (?)
 * - issue standard read/write commands
 * - controller sends IRQ6 when transfer complete
 *
 * */

void wffi_err(){
	ERROR(ERR_WFIRQ, 0x06);
}
uint8_t fDMA_uninit=0x1;
void initialise_floppy_DMA(){
	uint16_t transfer_address_start = 0x1000;
	uint16_t transfer_address_end = 0x1fff;
	outb(0x0a, 0x06); //mask 2, 0
	outb(0x0c, 0xFF); //reset master flipflop
	outb(0x04, transfer_address_start & 0xff); 
	outb(0x04, (transfer_address_start>>8)&0xff); 
	outb(0x0c, 0xff); //reset master flipflop again
	outb(0x05, (transfer_address_end-transfer_address_start)&0xff);
	outb(0x05, ((transfer_address_end-transfer_address_start)>>8)&0xff);
	outb(0x81, 0x0); //external page register to 0
	outb(0x0a, 0x02); //unmask dma channel 2
}
void prepare_for_floppy_DMA_write(){
	outb(0x0a, 0x06); //mask 2,0
	outb(0x0b, 0b01011010); //single transfer, address increment, autoinit, write, channel 2
	outb(0x0a, 0x02); //unmask 2
}
void prepare_for_floppy_DMA_read(){
	outb(0x0a, 0x06);
	outb(0x0b, 0b01010110); //single transfer, autoinit, read, channel 2
	outb(0x0a, 0x02);
}
enum FloppyRegs{
	STATUS_REGISTER_A = 0x3f0, //ro
	STATUS_REGISTER_B = 0x3f1, //ro
	DIGITAL_OUTPUT_REGISTER = 0x3f2, //rw
	TAPE_DRIVE_REGISTER = 0x3f3, //rw
	MAIN_STATUS_REGISTER = 0x3f4, //ro
	DATARATE_SELECT_REGISTER = 0x3f4, //wo
	DATA_FIFO = 0x3f5, //rw
	DIGITAL_INPUT_REGISTER = 0x3f7, //ro
	CONFIGURATION_CONTROL_REGISTER = 0x3f7 //wo
};
enum FloppyCommands {
READ_TRACK = 2, //generates irq6
SPECIFY = 3,
SENSE_DRIVE_STATUS=4,
WRITE_DATA=5,
READ_DATA=6,
RECALIBRATE=7,
SENSE_INTERRUPT=8, //ack irq6, get last cmd status
WRITE_DELETED_DATA=9,
	READ_ID=10, //generates irq6
	READ_DELETED_DATA=12,
	FORMAT_TRACK=13,
	DUMPREG=14,
	SEEK=15, //both heads to cylinder x
	VERSION=16,
	SCAN_EQUAL=17,
	PERPENDICULAR_MODE=18,
	CONFIGURE=19,
	LOCK=20, //protect params from reset
	VERIFY=22,
	SCAN_LOW_OR_EQUAL=25,
	SCAN_HIGH_OR_EQUAL=29

};
void FLOPPY_SEND_COMMAND(uint8_t cmd, uint8_t * param1, uint8_t *param1l, uint8_t * out1, uint8_t * out1l){
	uint16_t timeout=1000;
	uint8_t msr = inb(MAIN_STATUS_REGISTER);
	
	while((msr &0xc0) != 0x80){
		msr=inb(MAIN_STATUS_REGISTER);
//	floppy_ctrlrs();
		timeout --;
		nano_sleep(10000000);
		if(timeout==0)
			ERROR(ERR_FSCUNIMP, cmd);
	//	goto(RSFSC);		
	}
	outb(DATA_FIFO, cmd);
	if(param1 != NULL){
	while(param1 != param1l){
		timeout=1000;
		while((inb(MAIN_STATUS_REGISTER)&0xc0)!=0x80){
			timeout --; if(timeout==0)ERROR(ERR_FSC, cmd);
		}
		outb(DATA_FIFO, param1[0]);
		param1++;
	}
	}
	//skip to result phrase because using dma
	//somehow see if the command has a result phase
	//i dont think we use any that don't so it's probably fine
	//while(inb(MAIN_STATUS_REGISTER)&0x10){}
	
	if((cmd&(~0x40)) == READ_DATA || (cmd&(~0x40)) == WRITE_DATA){  wait_for_irq(0x06, 10000000, &wffi_err);
//	outb(DATA_FIFO, SENSE_INTERRUPT);
	}
	else if(cmd == RECALIBRATE){ wait_for_irq(0x06, 3000000000, &wffi_err);}
	//else{
	timeout=1000;
	if(out1l == out1){
		while((inb(MAIN_STATUS_REGISTER)&0x80)!=0x80){
			timeout --; if(timeout==0)ERROR(ERR_FSC4, cmd);
		}
		return;
	}
	while((inb(MAIN_STATUS_REGISTER)&0xc0)!=0xc0){ //TODO: c0 if have result phase, otherwise 80...
		nano_sleep(10000000);
		timeout --; if(timeout==0)ERROR(ERR_FSC2, cmd);
	}
	
	//timeout=1000;
	while((inb(MAIN_STATUS_REGISTER)&(0x50))==(0x50) && out1 < out1l){
		*(out1++) = inb(DATA_FIFO);
		while((inb(MAIN_STATUS_REGISTER)&0x80)!=0x80)
		{
		
		timeout --; if(timeout==0)ERROR(ERR_FSC3, cmd);
		}
	}
}
void floppy_ctrlrs(){
	uint8_t dor = inb(DIGITAL_OUTPUT_REGISTER);
	outb(DIGITAL_OUTPUT_REGISTER, 0x0);
	nano_sleep(4000);
	outb(DIGITAL_OUTPUT_REGISTER, dor);
	wait_for_irq(0x06, 10000000, &wffi_err);
	uint8_t  *aa=malloc(2);
	aa[0] = 8<<4|0;
	aa[1] = 5<<1;
	FLOPPY_SEND_COMMAND(SPECIFY, aa, aa+2, NULL, NULL);
	free(aa);
	outb(DIGITAL_OUTPUT_REGISTER, dor);
}
void floppy_init(){
	uint8_t  *aa = malloc(3);
	FLOPPY_SEND_COMMAND(VERSION, NULL, NULL, aa, aa+1); //god, i really fucking hope this works
	if(aa[0] != 0x90) ERROR(ERR_FVNS, 0x0);
	aa[0] = 0x0;
	aa[1] = 1<<6|1<<4|7;
	aa[2] = 0x0;
	FLOPPY_SEND_COMMAND(CONFIGURE, aa, aa+3, NULL, NULL); //im fucking this all up...
	FLOPPY_SEND_COMMAND(LOCK, NULL, NULL, aa, aa+1);
	floppy_ctrlrs();
	aa[0] = 0x0;
	FLOPPY_SEND_COMMAND(RECALIBRATE, aa, aa+1, NULL, NULL);
//	wait_for_irq(0x06, 3000000000, &FAULT);
	FLOPPY_SEND_COMMAND(SENSE_INTERRUPT, NULL, NULL, aa, aa+2); 
	free(aa);
}
int floppy_in_use = 0;
uint64_t * floppy_read(uint64_t LBA, uint16_t len){
	//if(floppy_in_use ==1)
	//	FAULT();
		//TODO: this breaks for UNGODLY reasons that will PROBABLY screw up something else later
	if(len != 0x1000)
		ERROR(ERR_FRLX, len);
	if(fDMA_uninit){
		initialise_floppy_DMA();
		floppy_init();
		fDMA_uninit=0x0;
		floppy_in_use=0;
	}
	if(floppy_in_use == 1) ERROR(ERR_FDC_INUSE, LBA);
	floppy_in_use=1;
	prepare_for_floppy_DMA_read();
	uint8_t *aa=malloc(8);
	asm volatile("xchgw %bx, %bx");
	lba_2_chs(LBA, aa+1, aa+2, aa+3);
	aa[0]=aa[2]<<2; //TODO: or with drive number
	aa[4]=2;
	aa[5]=*(aa+3)-*(aa+3)%18+18;
	aa[6]=0x1b;
	aa[7]=0xff;
	FLOPPY_SEND_COMMAND(READ_DATA|0x40, aa, aa+8, aa, aa+7);
	FLOPPY_SEND_COMMAND(SENSE_INTERRUPT, NULL, NULL, aa, aa+2); 
	free(aa);
	uint64_t * m = KPALLOC();
	memcpy(m, (void*)CBASE+0x1000, 0x1000);
	floppy_in_use = 0;
	return m;
}

void floppy_write(uint64_t LBA, uint16_t len, void * data){ //kilobyte aligned
	if(len != 0x1000)
		ERROR(ERR_FRLX, len);
	if(fDMA_uninit){
		initialise_floppy_DMA();
		floppy_init();
		fDMA_uninit=0x0;
		floppy_in_use=0;
	}
	if(floppy_in_use == 1) ERROR(ERR_FDC_INUSE, LBA);
	floppy_in_use=1;
	prepare_for_floppy_DMA_write();
	uint8_t *aa=malloc(8);
	lba_2_chs(LBA, aa+1, aa+2, aa+3);
	aa[0]=aa[2]<<2; //TODO: or with drive number
	aa[4]=2;
	aa[5]=*(aa+3)-*(aa+3)%18+18;
	aa[6]=0x1b;
	aa[7]=0xff;
	memcpy((void*)CBASE+0x1000, data, 0x1000);
	FLOPPY_SEND_COMMAND(WRITE_DATA|0x40, aa, aa+8, aa, aa+7);
	FLOPPY_SEND_COMMAND(SENSE_INTERRUPT, NULL, NULL, aa, aa+2); 
	free(aa);
	floppy_in_use = 0;
}
