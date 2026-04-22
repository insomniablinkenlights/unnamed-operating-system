#include <stdint.h>
#include "stdlib.h"
int main(int argc, char ** argv){
	//if(argc == 1) puts(argv[0]);
//	puts("HELLO WORLD");
//	puts((char*)argv); //temporary fix
	int k = 0;
	int i = 0;
	int m = 0;
	for(i = 0; i<argc; i++){
		k += strlen(argv[i])+1;
	}
	char * s = malloc(k+1);
	for(i = 0; i<argc; i++){
		strcpy(s+m, argv[i]);
		m += strlen(argv[i])+1;
		s[m-1] = ' ';
	}
	s[m] = '\n';
	s[m+1] = 0;
	fputs(s, stdout);
//	fputs("tty", stdout);
	//FILE * b = fopen("/dev/com", "a+");
	//fputs("com", b);
	//fclose(b);
	return 0;	
}
