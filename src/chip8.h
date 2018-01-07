#ifndef PSCHIP8_CHIP8_H_
#define PSCHIP8_CHIP8_H_
#include "types.h"

void chip8_loadrom(const uint8_t* data, uint32_t size);
void chip8_reset(void);
void chip8_step(void);
void chip8_logcpu(void);

#endif
