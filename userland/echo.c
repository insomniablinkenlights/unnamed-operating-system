#include <stdint.h>
#include "stdlib.h"
int main(int argc, char ** argv){
	//if(argc == 1) puts(argv[0]);
//	puts("HELLO WORLD");
//	puts((char*)argv); //temporary fix
	fputs("tty", stdout);
	FILE * b = fopen("/dev/com", "a+");
	fputs("com", b);
	fclose(b);
	return 0;	
}
