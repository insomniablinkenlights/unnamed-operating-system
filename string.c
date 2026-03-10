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
int strlen(char * a){
	int i = 0;
	for(;a[i];i++);
	return i;
}
int checkCorruption(){
	return 0xb339b009;
}
