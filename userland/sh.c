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
		bind(stdin, k->stdin);
		bind(stdout, k->stdout);
		wait(); //suspend execution until one of our children terminates
			//TODO: a concept of child processes
			//after the process terminates it should return stdin and stdout to its parent
	}
}
