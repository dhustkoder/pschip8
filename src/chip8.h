#ifndef PSCHIP8_CHIP8_H_
#define PSCHIP8_CHIP8_H_
#include "ints.h"


#define CHIP8_WIDTH         (64)
#define CHIP8_HEIGHT        (32)
#define CHIP8_SCALED_WIDTH  ((int)(CHIP8_WIDTH * 2.5))
#define CHIP8_SCALED_HEIGHT ((int)(CHIP8_HEIGHT * 2.5))
#define CHIP8_FREQ          (380)
#define CHIP8_DELAY_FREQ    (60)
#define CHIP8_BUZZ_FREQ     (480)


void chip8_loadrom(const char* filename);
void chip8_reset(void);
void chip8_step(void);


#endif
