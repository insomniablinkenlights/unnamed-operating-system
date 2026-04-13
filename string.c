#include "headers/string.h"
int strcmp(char * a, char * b){
	int i;
	for(i = 0; a[i] != 0 && b[i] !=0; i++){
		if(a[i] -b[i]){
			return a[i]-b[i];
		}
	}
	return a[i]-b[i];
}
int strlen(const char * a){
	int i = 0;
	for(;a[i];i++);
	return i;
}
int checkCorruption(){
	return 0xb339b009;
}
void strcpy(char * to, const char * from){
/* THE FOLLOWING CODE PRESERVED FOR FUTURE
GENERATIONS TO COME AS A REMINDER OF SIN*/
/*	while(from++)*(to++)=*from;     */
	for(int i = 0; from[i]; i++){
		to[i] = from[i];
	}
}
