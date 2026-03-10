#include <stdint.h>
uint64_t INT0x80(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx);
void * sbrk(uint64_t size){
	return (void*)INT0x80(5, size, 0, 0);
}
#define pid_t uint64_t
#define size_t uint64_t
typedef struct FILE{
	uint64_t fd;
}FILE;
FILE * stdin;
FILE * stdout;
typedef struct proc{
	pid_t pid;
	FILE stdin;
	FILE stdout;
}proc;
size_t strlen(const char * a){
	size_t i = 0;
	while(a[i])i++;
	return i;
}
size_t fwrite(const void *__restrict __ptr, int __size, int __n, FILE *__restrict __s){
	asm volatile("xchgw %bx, %bx");
	return INT0x80(1, __s->fd, (uint64_t)__ptr, __size*__n);
}
int putc(int __c, FILE *__stream){
	return fwrite(&__c, 1, 1, __stream);
}
int fputs(const char *__restrict __s, FILE *__restrict __stream){
	return fwrite(__s, 1, strlen(__s), __stream);
}
void * malloc(uint64_t size){
	uint64_t * a = ((uint64_t*)sbrk(size+8))+1;
	a[-1] = size;
	return a;
}
void free(void * a){
	return;
}
void memcpy(void * b, void * a, uint64_t size){
	for(uint64_t i = 0; i<size; i++){
		((char*)b)[i] = ((char*)a)[i];
	}
}
void * realloc(void * a, uint64_t size){ //assume we're not making it smaller
	void * b = malloc(size);
	memcpy(b, a, ((uint64_t*)a)[-1]);
	free(a);
	return b;
}
int puts(const char *__restrict __s){
	char * s2 = malloc(strlen(__s)+2);
	memcpy(s2, __s, strlen(__s));
	s2[strlen(__s)] = '\n';
	s2[strlen(__s)+1] = 0;
	int res = fputs(s2, stdout);
	free(s2);
	return res;
}
size_t fread(void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __stream){
	return INT0x80(0, __stream->fd, (uint64_t)__ptr, __size*__n);
}
int getc(FILE *__stream){
	char r;
	fread(&r, 0x1, 0x1, __stream);
	return r;
}
char *fgets (char * __s, int __n, FILE *__stream){ //get a newline terminated string of finite length
	int i = 0;
	while(i<__n){
		__s[i] = getc(__stream);
		if(__s[i] == '\n')return __s;
		i++;
	} 
	return __s;
}

proc * execv(const char *__path, char *const __argv[]){
	proc * k = malloc(sizeof(proc));
	k->pid = INT0x80(7, (uint64_t)__path, 0, (uint64_t)__argv);
	
}
pid_t wait(int * __stat_loc){
	return INT0x80(10,0,0,0);
}
void bind(FILE * a, FILE * b){
	INT0x80(0x8, a->fd, b->fd, 0);
}
void bindT(proc* a){
	INT0x80(0xb, a->pid,0,0);
}
int main(){
	asm volatile("xchgw %bx, %bx");
	stdin = malloc(sizeof(FILE));
	asm volatile("xchgw %bx, %bx");
	stdin->fd = 0;
	stdout = stdin;
	putc(0x11, stdout); //modeset, will send one ACK as conf
	puts("\ns HELL V0\n(c) 2026 KCW");
	asm volatile("xchgw %bx, %bx");
	char * buf = malloc(0x1000); //we can actually do this in UM!
	int bufL = 0x1000;
	int i = 0;
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
	#define NULL ((void*)0x0)
		proc * k = execv(buf, NULL); //todo arguments
		//somehow get the fucking shit to the fuck fuck shit
		//stdout and stdin of the new process are to be the TERMINAL
		
	//	bind(stdin, k->stdin); //stdin passes to k->stdin
		//bind(k->stdout, stdout); //k->stdout passes to stdout
		bindT(k);
		//so  k->stdio->pairout = stdio, stdio->pairin = k->stdio
		//but we want k->stdio->pairout=stdio->pairout, stdio->pairin->pairout=k->stdio
		//this doesn't work! we'll need to bind specifically the pairin and pairout together...
		wait(NULL); //suspend execution until one of our children terminates
			//TODO: a concept of child processes
			//after the process terminates it should return stdin and stdout to its parent
	}
}
