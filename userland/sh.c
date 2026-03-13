#include <stdint.h>
#include "stdlib.h"
int main(){
//	stdin = malloc(sizeof(FILE));
//	stdin->fd = 0;
//	stdout = stdin;
	putc(0x11, stdout); //modeset, will send one ACK as conf
	puts("\ns HELL V0\n(c) 2026 KCW");
	char * buf = malloc(0x1000); //we can actually do this in UM!
	int bufL = 0x1000;
	int i = 0;
/*	while(1){
		char k = getc(stdin);
		if(k == 0x6){ //ACK
			break;
		} //TODO: make an actual error processing thing for file reads, maybe needs return packing TwT
	}*/
	while(1){
		fputs("/ # ", stdout); //honestly kinda ironic considering that we have no concept of users or of cd yet
		fgets(buf, bufL, stdin);
		for(i = 0; buf[i] != '\n'; i++){}
		buf[i] = 0;
		for(i = 0; buf[i] != ' '; i++){}
		buf[i] = 0;
		char * kk = buf+i+1;
		proc * k = execv(buf, &kk); //todo arguments
		//somehow get the fucking shit to the fuck fuck shit
		//stdout and stdin of the new process are to be the TERMINAL
		
	//	bind(stdin, k->stdin); //stdin passes to k->stdin
		//bind(k->stdout, stdout); //k->stdout passes to stdout
		bindT(k);
		unblock(k);
		//so  k->stdio->pairout = stdio, stdio->pairin = k->stdio
		//but we want k->stdio->pairout=stdio->pairout, stdio->pairin->pairout=k->stdio
		//this doesn't work! we'll need to bind specifically the pairin and pairout together...
		wait(NULL); //suspend execution until one of our children terminates
			//TODO: a concept of child processes
			//after the process terminates it should return stdin and stdout to its parent
	}
}
