#include "headers/standard.h"
#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/filesystem.h"
#include "headers/ps2.h"
#include "headers/terminal.h"
void SEND_8042_CMD(uint8_t byte){
	uint16_t timer = 0x1000;
	while(timer>0){
		if(!(inb(0x64)&0x2)) break;
		timer--;
		if(timer == 0x0){
			ERROR(ERR_PS2_CMD_TIMEOUT, byte);
		}
		nano_sleep(0x1000000);
	}
	outb(0x64, byte);
}
void SEND_8042_DATA(uint8_t byte){
	uint16_t timer = 0x1000;
	while(timer>0){
		if(!(inb(0x64)&0x2)) break;
		timer--;
		if(timer == 0x0){
			ERROR(ERR_PS2_DATA_TIMEOUT, byte);
		}
		nano_sleep(0x1000000);
	}
	outb(0x60, byte);
}
uint8_t READ_8042(){
	uint16_t timer = 0x1000;
	while(timer>0){
		if((inb(0x64)&0x1)) break;
		timer--;
		if(timer == 0x0){
			ERROR(ERR_PS2_READ_TIMEOUT, 0x0);
		}
		nano_sleep(0x1000000);
	}
	return inb(0x60);
}
uint8_t PS2_IS_DUAL_CHANNEL = 0;
uint8_t PS2_SUPPORTED_CHANNELS = 0;
uint8_t PS2_KB_CHANNEL = 0;
uint8_t READ_KB_DATA(){
	PIC_PS2();
	wait_for_irq(0x1, 0x0, NULL);
	//TODO: fix wfirq
	uint8_t j = inb(0x60);
	PIC_sendEOI(0x1);
	PIC_PS2off(); //we'll drop some keys, but until we can make a better way this is what we get.
	return j;
}
uint8_t READ_KB_DATA2(){
	wait_for_irq(12, 0x0, NULL);
	//TODO: fix wfirq
	uint8_t j = inb(0x60);
	PIC_sendEOI(12);
	return j;
}
void PS2_INIT(){
	//TODO: initialise USB controllers, disable usb legacy support
	//assume there IS a ps/2 controller
	//disable PS/2 devices
	SEND_8042_CMD(0xAD);
	SEND_8042_CMD(0xA7);
	//flush output buffer
	inb(0x60);
	//set configuration byte
	SEND_8042_CMD(0x20);
	uint8_t k = READ_8042();
	k &= (0xFF ^ (1 | 1<<6 | 1 <<4));
	SEND_8042_CMD(0x60);
	SEND_8042_DATA(k);
	//perform self test
	SEND_8042_CMD(0xAA);
	uint8_t j = READ_8042();
	if(j != 0x55){
		ERROR(ERR_8042_SELF_TEST, j);
	}
	//determine if controller is dual-channel
	SEND_8042_CMD(0xA8);
	SEND_8042_CMD(0x20);
	j = READ_8042();
	if(j & 0x1<<5){
		PS2_IS_DUAL_CHANNEL=0;
		PS2_SUPPORTED_CHANNELS=0x1;
	}else{
		PS2_IS_DUAL_CHANNEL=1;
		PS2_SUPPORTED_CHANNELS=0x3;
		SEND_8042_CMD(0xA7);
		j &= (~(0x1| 0x1<<5));
		SEND_8042_CMD(0x60);
		SEND_8042_DATA(j);
	}
	//perform interface tests
	SEND_8042_CMD(0xAB);
	if(READ_8042() != 0x0){
		PS2_SUPPORTED_CHANNELS &= 0x2;
	}
	if(PS2_IS_DUAL_CHANNEL){
		SEND_8042_CMD(0xAB);
		if(READ_8042() != 0x0){
			PS2_SUPPORTED_CHANNELS &= 0x1;
		}
	}
	if(PS2_SUPPORTED_CHANNELS == 0x0){
		ERROR(ERR_PS2_NO_CHANNELS, 0x0);
	}
	//enable working devices
	if(PS2_SUPPORTED_CHANNELS&0x1){
		SEND_8042_CMD(0xAE);
	}
	if(PS2_SUPPORTED_CHANNELS&0x2){
	//	SEND_8042_CMD(0xA8);
	//	temporarily disabled because we don't support mice yet
	}
	SEND_8042_CMD(0x20);
	j = READ_8042();
	j = (j&(~0x3))|PS2_SUPPORTED_CHANNELS|(1<<6); //enable tl
	SEND_8042_CMD(0x60); //TODO: enable scan convert
	SEND_8042_DATA(j);
	//reset devices
	PIC_PS2();

/*	SEND_8042_DATA(0xFF);
	READ_KB_DATA();
       	READ_KB_DATA(); //TODO: verify that the devices reset successfully
	if(PS2_IS_DUAL_CHANNEL){
		SEND_8042_CMD(0xD4);
		SEND_8042_DATA(0xFF);
		READ_KB_DATA2(); 
		READ_KB_DATA2();
	}*/

	//detect which channel is keyboard
	PS2_KB_CHANNEL=PS2_SUPPORTED_CHANNELS;
/*	if(PS2_SUPPORTED_CHANNELS&0x1){
		SEND_8042_DATA(0xF5);
		READ_KB_DATA();
		SEND_8042_DATA(0xF2);
		READ_KB_DATA();
		uint8_t b = READ_KB_DATA();
		if(b == 0xAB || b == 0xAC){
			uint8_t b2 = READ_KB_DATA();
		}
		if(b == 0x03 || b == 0x00 || b == 0x04){
			PS2_KB_CHANNEL ^= 0x1;
		}
		SEND_8042_DATA(0xF4);
		READ_KB_DATA();
	}
	if(PS2_SUPPORTED_CHANNELS&0x2){
		SEND_8042_CMD(0xD4);
		SEND_8042_DATA(0xF5);
		READ_KB_DATA();
		SEND_8042_CMD(0xD4);
		SEND_8042_DATA(0xF2);
		READ_KB_DATA();
		uint8_t b = READ_KB_DATA();
		if(b == 0xAB || b == 0xAC){
			uint8_t b2 = READ_KB_DATA();
		}
		if(b == 0x03 || b == 0x00 || b == 0x04){
			PS2_KB_CHANNEL ^= 0x2;
		}
		SEND_8042_CMD(0xD4);
		SEND_8042_DATA(0xF4);
		READ_KB_DATA();
	}*/
}

uint8_t * KM;
uint8_t KMI = 0;
uint8_t KeyMap(uint8_t keycode, uint8_t mods){
	if(!KMI){
		KM = KPALLOC();
		uint64_t m = OPEN("/etc/keymap"+CBASE, F_FG_RD);
		READ(m, KM, 0x200); //512 bytes == four layers
				     //layers: 0, shift, right alt, shift + right alt
		CLOSE(m);
		KMI=1;
	}
	uint16_t index = keycode;
	if(mods & 0x3) index+=0x80;
	if(mods & 0x8) index+=0x100;
	return KM[index];
	
}
uint8_t keyboard_init = 0x0;
uint8_t keyboard_is_raw = 0x1;
void PS2_DRIVER(void * UNUSED(arguments)){
	//to output we'll use WRITE(0, &k, sizeof(KP));
	uint8_t LsRsLaRaClNlLctRct = 0; //states obviously
	uint8_t b1 = 0;
	uint8_t b2 = 0;
	keyboard_is_raw = 0x1;
	KP * k = malloc(sizeof(KP)); //static arrays fuck it up -- don't know why it didn't break until now !
	PS2_INIT();
	InitKernelFd(); 
	PS2_DRIVER_BIND_STDIO();
	KeyMap(0, 0);
	//BREAK((uint64_t)current_task_TCB); //here 'tis RUNNING
	keyboard_init=1;
	while(1){
		if(PS2_KB_CHANNEL & 0x1){
			b1 = READ_KB_DATA();
		}//else if(PS2_KB_CHANNEL & 0x2){
		//	b1 = READ_KB_DATA2();
		//}
		else {
			ERROR(ERR_NOKEYBOARD,0x0);
		}
	//	BREAK(0x38);
		if(b1 == 0xe0){
			if(PS2_KB_CHANNEL & 0x1){
				b2 = READ_KB_DATA();
			}//else if(PS2_KB_CHANNEL & 0x2){
			//	b2 = READ_KB_DATA2();
			//}
			else {
				ERROR(ERR_NOKEYBOARD,0x0);
			}
			k->keycode=b2|0x80;
			k->pR = (b2&0x80)>>7;
			switch(k->keycode^0x80){
				case 0x1D:
					LsRsLaRaClNlLctRct ^= 0x80;
					break;
				case 0x38:
				       LsRsLaRaClNlLctRct^=0x8;	
				       break;
				default:
				       k->ascii = KeyMap(k->keycode, LsRsLaRaClNlLctRct);
				       k->states = LsRsLaRaClNlLctRct;
			       	       if(keyboard_is_raw) WRITE(0, k, sizeof(KP));	       
			}
		}else{
			k->keycode = (b1|0x80)^0x80; //&0x7F is better
			k->pR = (b1&0x80)>>7;
			switch(k->keycode){
				case 0x1D:
					LsRsLaRaClNlLctRct ^= 0x40; //lct
					break;
				case 0x2A:
					LsRsLaRaClNlLctRct ^=0x1; //ls
					break;
				case 0x36:
					LsRsLaRaClNlLctRct^=0x2; //rs
					break;
				case 0x38:
					LsRsLaRaClNlLctRct ^= 0x4; //la
					break;
				case 0x3A:
					LsRsLaRaClNlLctRct ^= 0x10; //fuck colemak users
					break;
				case 0x45:
					LsRsLaRaClNlLctRct ^= 0x20; //nl
					break;
				default:
					if(k->keycode > 0x80){
						ERROR(ERR_KC_OOR, k->keycode);
					}
					k->ascii = KeyMap(k->keycode, LsRsLaRaClNlLctRct);
					k->states = LsRsLaRaClNlLctRct;
			       	        if(keyboard_is_raw) WRITE(0, k, sizeof(KP));	       
					else if(k->pR){
						write_to_screen((char*)(&(k->ascii)), 1);
						 WRITE(0, &(k->ascii), 1);
					}
			}
		}
	}
}
