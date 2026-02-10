#include "stdint.h"
#ifndef ps2
#define ps2
typedef struct __attribute__((packed)) KP{
	uint8_t ascii;
	uint8_t keycode;
	uint8_t pR;
	uint8_t states;
}KP;
void PS2_DRIVER();
extern uint8_t keyboard_init;
extern uint8_t keyboard_is_raw;
#endif
