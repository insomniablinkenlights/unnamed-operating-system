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
