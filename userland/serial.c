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
	outb(ourCOMbase +1, 0x00); //disable interrupts
	sBaud(ourCOMbase, 4);
	sProt(ourCOMbase, 8,0,1);
	//some weird hardcoded values
	outb(ourCOMbase + 2, 0x7); //enable and clear FIFO
	outb(ourCOMbase+4, 0x0f); //enable IRQs, set rts/dsr?
			//rts == 'request to send'
			//dtr == 'data terminal ready'
}
void main(){
	INT0x80(0xd, (uint64_t)"com", 0, 0);
	int irb = 0;
	setupSerial();
	//outb(ourCOMbase, 'a');
	while(1){
		outb(ourCOMbase +1, 1|2|4|8); //enable all interrupts
		//TODO: if we have two things happen then we get no more interrupts. to actually check if we have an interrupt, we should check against the first byte of the IRB.
		int rv = INT0x80(0xf, 0x4, 0 , 0);
		outb(ourCOMbase+1, 0); //disable interrupts (for now...)
		if(rv == -1){
			//our PL is wrong
			INT0x80(0xE,0, (uint64_t) "error pl incorrect",0);
		}
		irb = inb(ourCOMbase+2);
		if(irb&1){
			INT0x80(0xE,irb,(uint64_t) "serial interrupt triggered, but nothing to read!",0);
			//strange. let's just ignore it.
			continue;
		}
		asm volatile("xchgw %bx, %bx");
		irb = 3&(irb>>1);
		/* bits 1-2 of irb
			0 = modem status
			1 = transmitter holding register empty
				we can send 1 byte
			2 = received data available
				we can read 1 byte
			3 = receiver line status
		*/
		if(irb == 0){
			uint8_t ms = inb(ourCOMbase+6);
			/*modem status register:
				0 = delta clear to send
					cts input has changed since last read
				1 = delta data set ready
					dsr input has changed state since last read
				2 = trailing edge of ring indicator: ri input has changed from low to high state
				3 = delta data carrier detect: dcd input has changed state since read
				4 = clear to send (inverted CTS)
				5 = data set ready (inverted DSR)
				6 = ring indicator (inverted RI)
				7 = data carrier detect (inverted DCD)
		
*/
		}else if(irb == 3){
			uint8_t ls = inb(ourCOMbase+5);
			if((ls &( 1 << 7)) || (ls & (7<<1)))
				INT0x80(0xe, 0, (uint64_t) "various LSR errors", 0 );
/*line status register:
	0 = data ready: is there readable data?
	1 = overrun error: has there been data lost?
	2 = parity error: transmission error detected by parity
	3 = framing error: was a stop bit missing?
	4 = break indicator: was there a break in data input
	5 = transmitter holding register empty (can data be sent?)
	6 = transmitter empty
	7 = impending error
*/
			
		}else if(irb == 1){
//		asm volatile("xchgw %bx, %bx");
			//we can send one byte
			if(INT0x80(0x10, 0, 0, 0)){
				//READ(0, &irb, 1);
				irb = getc(stdin);
				outb(ourCOMbase, irb);
			}else{
				INT0x80(0x12, 0x1000000, 0, 0);
			}
		}else if(irb == 2){
			//grab a byte
			irb = inb(ourCOMbase);
			//WRITE(0, &irb, 1);
			putc(irb, stdout);
		}
	}
}

