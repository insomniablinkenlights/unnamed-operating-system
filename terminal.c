#include "headers/stdint.h"
#include "headers/addresses.h"
int terminal_x = 0;
int terminal_y = 0;
void write_to_screen(char * k, uint64_t len){
	for(uint64_t i = 0; i<len; i++){
		if(terminal_x>80){
			terminal_x=0; terminal_y++;
		}
		while(terminal_y>20){
			memcpy((char*)(CBASE+0xb8000), (char*)(CBASE+0xb8000)+160, 160*24);	
			terminal_y--;
		}
		if(terminal_y<20){
			if(k[i] == '\n'){
				while(terminal_x<80){
					((char*)(CBASE+0xb8000))[terminal_y*160+terminal_x*2] = ' ';
					((char*)(CBASE+0xb8000))[terminal_y*160+terminal_x*2+1] = 0x2a;
					terminal_x++;
				}
				terminal_x=0; terminal_y++;
			}
			else{
				((char*)(CBASE+0xb8000))[terminal_y*160+terminal_x*2] = k[i];
				((char*)(CBASE+0xb8000))[terminal_y*160+terminal_x*2+1] = 0x2a;
				terminal_x++;
			}
		}
	}
}
