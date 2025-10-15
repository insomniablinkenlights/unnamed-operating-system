#include "headers/stdint.h"
void lba_2_chs(uint32_t lba, uint16_t* cyl, uint16_t* head, uint16_t* sector){
	*cyl = lba / (2*18);
	*head = ((lba %(2*18))/18);
	*sector = ((lba % (2*18))%18 +1);
}
