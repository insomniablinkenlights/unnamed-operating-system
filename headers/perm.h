#ifndef H_PERM
#define H_PERM
#include "stdint.h"
#include "umperm.h"
struct perm_desc{
	void * t_root;
	uint64_t PM;
	//TODO: flags, flag restrictions!	
};
struct perm_desc * cPD(uint64_t bp, void * root);
struct perm_desc * VERIFY_PERMS_EXEC(struct Pdesc * target);
#endif
