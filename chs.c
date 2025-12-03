#include "headers/stdint.h"
#include "headers/addresses.h"
void lba_2_chs(uint32_t lba, uint8_t* cyl, uint8_t* head, uint8_t* sector){
	*cyl = lba / (2*18);
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
uint8_t fDMA_uninit=0x1;
void initialise_floppy_DMA(){
	uint16_t transfer_address_start = 0x1000;
	uint16_t transfer_address_end = 0x33ff;
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
}
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

}
void FLOPPY_SEND_COMMAND(uint8_t cmd, uint8_t * param1, uint8_t param1l, uint8_t * out1, uint8_t * out1l){
	uint16_t timeout=1000;
RSFSC: uint8_t msr = inb(MAIN_STATUS_REGISTER);
	if((msr &0xc0) != 0x80){
		FLOPPY_CONTROLLER_RESET();
		goto(RSFSC);		
	}
	outb(DATA_FIFO, cmd);
	if(param1 != NULL){
	while(param1++ != param1l){
		timeout=1000;
		while((inb(MAIN_STATUS_REGISTER)&0xc0)!=0x80){
			timeout --; if(timeout==0)FAULT();
		}
		outb(DATA_FIFO, param1);
	}
	}
	//skip to result phrase because using dma
	//somehow see if the command has a result phase
	//i dont think we use any that don't so it's probably fine
	//while(inb(MAIN_STATUS_REGISTER)&0x10){}
	
	if(cmd == READ_DATA || cmd == WRITE_DATA)  wait_for_irq(0x06, 10000000, &FAULT);
	if(cmd == RECALIBRATE) wait_for_irq(0x06, 3000000000, &FAULT);
	timeout=1000;
	while((inb(MAIN_STATUS_REGISTER)&0xc0)!=0x80){
	
		timeout --; if(timeout==0)FAULT();
	}
	timeout=1000;
	while((inb(MAIN_STATUS_REGISTER)&(0x50|0x80))==(0x50|0x80) && out1 < out1l){
		*(out1++) = inb(DATA_FIFO);
		while((inb(MAIN_STATUS_REGISTER)&0x80)!=0x80)
		{
		
		timeout --; if(timeout==0)FAULT();
		}
	}
}
void floppy_ctrlrs(){
	uint8_t dor = inb(DIGITAL_OUTPUT_REGISTER);
	outb(DIGITAL_OUTPUT_REGISTER, 0x0);
	nano_sleep(4000);
	outb(DIGITAL_OUTPUT_REGISTER, dor);
	wait_for_irq(0x06, 10000000, &FAULT);
	uint8_t [2] aa;
	aa[0] = 8<<4|0;
	aa[1] = 5<<1;
	FLOPPY_SEND_COMMAND(SPECIFY, aa, aa+2, NULL, NULL);
	outb(DIGITAL_OUTPUT_REGISTER, dor);
}
void floppy_init(){
	uint8_t [3] aa;
	FLOPPY_SEND_COMMAND(VERSION, NULL, NULL, aa, aa+1); //god, i really fucking hope this works
	if(aa != 0x90) FAULT();
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
}
int floppy_in_use = 0;
uint64_t * floppy_read(uint64_t LBA, uint16_t len){
	if(floppy_in_use ==1)
		FAULT();
	floppy_in_use=1;
	if(len != 0x1000)
		FAULT();
	if(FDMA_uninit){
		initialise_floppy_DMA();
		floppy_init();
		FDMA_uninit=0x0;
	}
	prepare_for_floppy_DMA_read();
	uint8_t [8] aa;
	lba_2_chs(LBA, aa[1], aa[2], aa[3]);
	aa[0]=aa[2]<<2; //TODO: or with drive number
	aa[4]=2;
	aa[5]=18;
	aa[6]=0x1b;
	aa[7]=0xff;
	FLOPPY_SEND_COMMAND(READ|0x40, aa, aa+8, aa, aa+7);
	uint64_t * m = KPALLOC();
	memcpy(m, CBASE+0x1000, 0x1000);
	floppy_in_use = 0;

}

