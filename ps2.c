#include "headers/stdint.h"
#include "headers/addresses.h"
#include "headers/filesystem.h"
#include "headers/ps2.h"
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
uint8_t READ_KB_DATA(){
	wait_for_irq(0x1, 0x0, NULL);
	//TODO: fix wfirq
	uint8_t j = inb(0x60);
	PIC_sendEOI(0x1);
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
		SEND_8042_CMD(0xA8);
	}
	SEND_8042_CMD(0x20);
	j = READ_8042();
	j = (j&(~0x3))|PS2_SUPPORTED_CHANNELS;
	SEND_8042_CMD(0x60);
	SEND_8042_DATA(j);
	//reset devices
	SEND_8042_DATA(0xFF);
	READ_KB_DATA();
       	READ_KB_DATA(); //TODO: verify that the devices reset successfully
	if(PS2_IS_DUAL_CHANNEL){
		SEND_8042_CMD(0xD4);
		SEND_8042_DATA(0xFF);
		READ_KB_DATA2(); 
		READ_KB_DATA2();
	}
}

uint8_t * KM;
uint8_t KMI = 0;
uint8_t KeyMap(uint8_t keycode, uint8_t mods){
	if(!KMI){
		KM = KPALLOC();
		InitKernelFd();
		uint64_t m = OPEN("/etc/keymap"+CBASE, 0x3);
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
KP * keyboard_buffer = NULL;
uint64_t keyboard_buffer_end;
uint8_t keyboard_init = 0x0;
void init_keyboard_PS2(){
	keyboard_buffer  = KPALLOC();
	keyboard_buffer_end = 0;
}
void PS2_DRIVER(){
	/* we'll push stuff onto the keyboard buffer, and if the length is too much remove the older stuff */
	uint8_t LsRsLaRaClNlLctRct = 0; //states obviously
	uint8_t b1 = 0;
	uint8_t b2 = 0;
	KP k;
	PS2_INIT();
	init_keyboard_PS2();
	keyboard_init=1;
	while(1){
		b1 = READ_KB_DATA();
		if(b1 == 0xe0){
			b2 = READ_KB_DATA();
			k.keycode=b2|0x80;
			k.pR = (b2&0x80)>>7;
			switch(k.keycode^0x80){
				case 0x1D:
					LsRsLaRaClNlLctRct ^= 0x80;
					break;
				case 0x38:
				       LsRsLaRaClNlLctRct^=0x8;	
				       break;
				default:
				       k.ascii = KeyMap(k.keycode, LsRsLaRaClNlLctRct);
				       k.states = LsRsLaRaClNlLctRct;
					if((keyboard_buffer_end+1)*8<0x1000){ //8 is sizeof KP
						keyboard_buffer[keyboard_buffer_end] = k;
						keyboard_buffer_end++;
					}else{
						for(uint64_t i = 0; i<keyboard_buffer_end-1; i++){
							keyboard_buffer[i] = keyboard_buffer[i+1];
						}
						keyboard_buffer[keyboard_buffer_end] = k;
					}
			}
		}else{
			k.keycode = b1&~(0x80);
			k.pR = (b1&0x80)>>7;
			switch(k.keycode){
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
					k.ascii = KeyMap(k.keycode, LsRsLaRaClNlLctRct);
					k.states = LsRsLaRaClNlLctRct;
					if((keyboard_buffer_end+1)*8<0x1000){ //8 is sizeof KP
						keyboard_buffer[keyboard_buffer_end] = k;
						keyboard_buffer_end++;
					}else{
						for(uint64_t i = 0; i<keyboard_buffer_end-1; i++){
							keyboard_buffer[i] = keyboard_buffer[i+1];
						}
						keyboard_buffer[keyboard_buffer_end] = k;
					}

			}
		}
	}
}
