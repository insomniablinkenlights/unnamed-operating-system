#include "headers/perm.h"
#include "headers/standard.h"
#include "headers/addresses.h"
#include "headers/proc.h"
#include "headers/umperm.h"
#include "headers/string.h"
#include "headers/filesystem.h"
#include "headers/usermode.h"
#include "headers/filesystem_internal.h"
bool checkEPL(uint64_t req){
	return (req&(current_task_TCB->perms->PM)) == req;
}
struct perm_desc * cPD(uint64_t bp, void * root){
	struct perm_desc * m = malloc(sizeof(struct perm_desc));
	m->t_root = root;
	m->PM = bp;
	return m;
}
struct perm_desc * VERIFY_PERMS_EXEC(struct Pdesc * target){ //Usermode -> Kernel, + Verification
	struct Pdesc * t_km = malloc(sizeof(struct Pdesc));
	struct perm_desc * to = malloc(sizeof(struct perm_desc));
	struct perm_desc * from = (struct perm_desc *)(current_task_TCB->perms); //what we check our perms against
	target->chrootfname = VERIFY_USER(target->chrootfname);
	t_km->chrootfname = malloc(32);
	memcpy(t_km->chrootfname, target->chrootfname, MIN(strlen(target->chrootfname), 32));
	//verify that the fname can be accessed as +x with our current priv level
	//to->t_root = VERIFY_INODE(target, from); 
	if(!checkEPL(target->PM)) ERROR(ERR_TODO_PROC_ERR,1);
	to->PM = target->PM;
	//to->t_root = (target->chrootfname);
	to->t_root = getFileFromFilename(from->t_root, target->chrootfname);
	return to;
}
/*uint8_t checkFPL(uint64_t fpl, uint64_t req){
	//check that no 1's in req are 0 in fpl
	if(req&fpl != req) return 0;
	return 1;
}*/
