int main();
void _init() __attribute__((weak));
void _fini() __attribute__((weak));
_Noreturn int __libc_start_main(int (*main) (int, char **, char**), int argc, char ** ubp_av, void (*init) (void), void (*fini) (void), void (*rtld_fini) (void), void (*stack_end)){
	int rv = main(argc, ubp_av, NULL);
	exit(rv);
	return rv;
}
void __cstart(long * p){
	int argc = p[0];
	char ** argv = (void*)(p+1);
	__libc_start_main(main, argc, argv, _init, _fini, 0);
}
