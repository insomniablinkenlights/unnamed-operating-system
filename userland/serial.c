#include "stdlib.h"
#include <stdint.h>
int COMtoIO(int no){
	switch(no){
		case 1: return 0x3f8;
		case 2: return 0x2f8;
		case 3: return 0x3e8;
		case 4: return 0x2e8;
		case 5: return 0x5f8;
		case 6: return 0x4f8;
		case 7: return 0x5e8;
		case 8: return 0x4e8;
		default: return 0x3f8;
	}
}
int ourCOMbase = 0;
int checkCOM(int port){
	outb(port + 7, 0x3e); 
	int chk = inb(port + 7);
	if(chk == 0x3e) return 1;
	return 0;
}
int findCOM(){
	for (int i = 1; i<8; i++){
		if(checkCOM(COMtoIO(i))){
			return i;
		}
	}
	return 0;
}
void sBaud(int cb, int k){
	int b = inb(cb+3);
	b |= 0x80;
	outb(cb+3, b);
	outb(cb, k&0xFF);
	outb(cb+1, (k&0xFF00)>>8);
	b ^= 0x80;
	outb(cb+3, b);
}
void sProt(int cb, int bits, int parity, int stop){
	int b = inb(cb+3) & 0x80;
	b = b | (bits -5) | ((stop-1) <<2) | (parity<<3);
	outb(cb+3, b);
}
void setupSerial(){
	ourCOMbase = COMtoIO(findCOM());
	outb(ourCOMbase +1, 0x00);
	sBaud(ourCOMbase, 4);
	sProt(ourCOMbase, 8,0,1);
	//some weird hardcoded values
	outb(ourCOMbase + 2, 0x7); //enable and clear FIFO
	outb(ourCOMbase+4, 0x0f); //enable IRQs, set rts/dsr?
			//rts == 'request to send'
			//dtr == 'data terminal ready'
}
void main(){
	asm volatile("xchgw %bx, %bx");
	INT0x80(0xd, (uint64_t)"com", 0, 0);
	int irb = 0;
	setupSerial();
	while(1){
		//wait_for_irq(0x4);
		INT0x80(0xf, 0x4, 0 , 0);
		irb = 3&(inb(ourCOMbase+2)>>1);
		/* bits 1-2 of irb
			0 = modem status
			1 = transmitter holding register empty
				we can send 1 byte
			2 = received data available
				we can read 1 byte
			3 = receiver line status
		*/
		if(irb == 0 || irb == 3){
			//ERROR(ERR_COM_UNIMPLEMENTED, irb);
			INT0x80(0,(uint64_t) "error com unimplemented :3",0, 0);
		}
		if(irb == 1){
			//we can send one byte
			if(INT0x80(0x10, 0, 0, 0)){
				//READ(0, &irb, 1);
				irb = getc(stdin);
				outb(ourCOMbase, irb);
			}
		}else if(irb == 2){
			//grab a byte
			irb = inb(ourCOMbase);
			//WRITE(0, &irb, 1);
			putc(irb, stdout);
		}
	}
}

