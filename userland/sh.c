#include "stdlib/stdio.h"
#include "stdlib/stdlib.h"
int main(){
	char * buf = malloc(0x1000); //we can actually do this in UM!
	int bufL = 0x1000;
	int i = 0;
	putc(0x11, stdout); //modeset, will send one ACK as conf
	puts("\ns HELL V0\n(c) 2026 KCW");
	while(1){
		char k = getc(stdin);
		if(k == 0x6){ //ACK
			break;
		} //TODO: make an actual error processing thing for file reads, maybe needs return packing TwT
	}
	while(1){
		fputs("/ # ", stdout); //honestly kinda ironic considering that we have no concept of users or of cd yet
		fgets(buf, bufL, stdin);
		for(i = 0; buf[i] != '\n'; i++){}
		buf[i] = 0;
		proc * k = exec(buf, NULL); //todo arguments
		//pass through k's stdout to our stdout
		bind(0, k, 2 | 8); //1: our stdin, 2: our stdout, 4: k's stdin, 8: k's stdout, 16: new (on our side)
		bind(0, k, 1 | 4); //our stdin to k's stdin
		//once we have the process we can pass IO
		while(k->exists)
			wait(k, 0); //wait () will just return -1 instead of having a timeout_function
		//i think our stdio should be automatically unbound now
	}
}
