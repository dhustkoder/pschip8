#ifndef PSCHIP8_CHIP8_H_
#define PSCHIP8_CHIP8_H_
#include "system.h"

// the drawing will be at the gfx buffer with a resolution of 64x32.
// the gfx buffer must be at least 64x32.
// if gfx buffer is greater, the drawing will be centered in the buffer.
// This fixes the GsSPRITE scale problem for now...
// TODO: Fix GsSPRITE scale problem, so we can get rid of this workaround.
#define CHIP8_GFX_WIDTH   (68)
#define CHIP8_GFX_HEIGHT  (34)
#define CHIP8_FREQ        (380)
#define CHIP8_BUZZ_FREQ   (480)
#define CHIP8_DELAY_FREQ  (60)
#define CHIP8_GFX_BGC     (0x8000)
#define CHIP8_GFX_FGC     (0xFFFF)
#define CHIP8_GFX_TYPE    uint16_t


typedef uint16_t Chip8Key;
enum Chip8Key {
	CHIP8KEY_0 = 0x0001,
	CHIP8KEY_1 = 0x0002,
	CHIP8KEY_2 = 0x0004,
	CHIP8KEY_3 = 0x0008,
	CHIP8KEY_4 = 0x0010,
	CHIP8KEY_5 = 0x0020,
	CHIP8KEY_6 = 0x0040,
	CHIP8KEY_7 = 0x0080,
	CHIP8KEY_8 = 0x0100,
	CHIP8KEY_9 = 0x0200,
	CHIP8KEY_A = 0x0400,
	CHIP8KEY_B = 0x0800,
	CHIP8KEY_C = 0x1000,
	CHIP8KEY_D = 0x2000,
	CHIP8KEY_E = 0x4000,
	CHIP8KEY_F = 0x8000,
};

void chip8_loadrom(const char* filename);
void chip8_reset(void);
void chip8_step(void);


#endif
