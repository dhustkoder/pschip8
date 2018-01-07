#include <stdio.h>
#include <stdlib.h>
#include <libetc.h>
#include <rand.h>
#include "types.h"
#include "log.h"


bool chip8_scrdata[64][32];

static struct {
	uint16_t pc;
	uint16_t i;
	uint8_t sp;
	uint8_t v[0x10];
	uint8_t dt;
	uint8_t st;
} rgs;

static uint8_t ram[0x1000];
static uint16_t stack[32];

static const uint8_t font[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


static void stackpush(const uint16_t value)
{
	stack[rgs.sp--] = value;
}

static uint16_t stackpop(void)
{
	return stack[++rgs.sp];
}

static void draw(uint8_t x, uint8_t y, const uint8_t n)
{
	/* set VF = collision. The interpreter reads n bytes from memory, starting at the address stored in I. 
	 * These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). 
	 * Sprites are XORed onto the existing screen. 
	 * If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. 
	 * If the sprite is positioned so part of it is outside the coordinates of the display, 
	 * it wraps around to the opposite side of the screen. See instruction 8xy3 for more information on XOR, 
	 * and section 2.4, Display, for more information on the Chip-8 screen and sprites.
	 */

	const uint8_t* sprite = &ram[rgs.i];
	int i, j;

	for (i = 0; i < n; ++i, y = (y + 1)&32) {
		for (j = 0; j < 8; ++j, x = (x + 1)&63) {
			if (chip8_scrdata[y][x] && sprite[i]&(1<<(7 - j)))
				rgs.v[0x0F] = 1;
			else
				rgs.v[0x0F] = 0;

			chip8_scrdata[y][x] ^= (sprite[i]&(1<<(7 - j))) != 0;
		}
	}
}

void chip8_loadrom(const uint8_t* const rom, const uint32_t size)
{
	memcpy(&ram[0x200], rom, size);
}

void chip8_reset(void)
{
	int i;
	for (i = 0; i < 0x200; ++i)
		ram[i] = font[i%80];

	memset(&rgs, 0, sizeof rgs);
	memset(stack, 0, sizeof stack);
	rgs.pc = 0x200;
	rgs.sp = 15;
	srand(VSync(1)|VSync(-1));
}

void chip8_logcpu(void)
{
	int i;

	loginfo("PC: $%.4X, SP: $%.4X, I: $%.4X\n",
	        rgs.pc, rgs.sp, rgs.i);
	loginfo("DT: $%.2X, ST: $%.2X\n",
		rgs.dt, rgs.st);

	for (i = 0; i < 0x10; ++i)
		loginfo("V%.1X: $%.2X\n", i, rgs.v[i]);

}

void chip8_step(void)
{
	const uint8_t ophi = ram[rgs.pc++];
	const uint8_t oplo = ram[rgs.pc++];
	const uint16_t opcode = (ophi<<8)|oplo;

	const uint8_t x = ophi&0x0F;
	const uint8_t y = (ophi&0xF0)>>4;


	logdebug("OPCODE: %.4X\n", opcode);

	switch ((ophi&0xF0)>>4) {
	case 0x00: {
		switch (oplo) {
		case 0xE0: // - CLS clear display
			memset(chip8_scrdata, 0, sizeof chip8_scrdata);
			break;
		case 0xEE: // - RET Return from a subroutine.
			rgs.pc = stackpop();
			break;
		}
		break;
	}

	case 0x01: // 1nnn - JP addr Jump to location nnn.
		rgs.pc = opcode&0x0FFF;
		break; 
	case 0x02: // 2nnn - CALL addr Call subroutine at nnn.
		stackpush(rgs.pc);
		rgs.pc = opcode&0x0FFF;
		break;
	case 0x03: // 3xkk - SE Vx, byte Skip next instruction if Vx = kk.
		if (rgs.v[x] == oplo)
			rgs.pc += 2;
		break;
	case 0x04: // 4xkk - SNE Vx, byte Skip next instruction if Vx != kk.
		if (rgs.v[x] != oplo)
			rgs.pc += 2;
		break;
	case 0x05:  // 5xy0 - SE Vx, Vy Skip next instruction if Vx = Vy.
		if (rgs.v[x] == rgs.v[y])
			rgs.pc += 2;
		break;
	case 0x06:  // 6xkk - LD Vx, byte Set Vx = kk. The interpreter puts the value kk into register Vx.
		rgs.v[x] = oplo;
		break;
	case 0x07:  // 7xkk - ADD Vx, byte Set Vx = Vx + kk.
		rgs.v[x] += oplo;
		break;
	case 0x08:
		switch (oplo&0x0F) {
		case 0x00: // 8xy0 - LD Vx, Vy Set Vx = Vy.
			rgs.v[x] = rgs.v[y];
			break;
		case 0x01: // 8xy1 - OR Vx, Vy Set Vx = Vx OR Vy.
			rgs.v[x] |= rgs.v[y];
			break;
		case 0x02: // 8xy2 - AND Vx, Vy Set Vx = Vx AND Vy.
			rgs.v[x] &= rgs.v[y];
			break;
		case 0x03: // 8xy3 - XOR Vx, Vy Set Vx = Vx XOR Vy. 
			rgs.v[x] ^= rgs.v[y];
			break;
		case 0x04: // 8xy4 - ADD Vx, Vy Set Vx = Vx + Vy, set VF = carry.
			rgs.v[0x0F] = (rgs.v[x] + rgs.v[y]) > 0xFF;
			rgs.v[x] += rgs.v[y];
			break;
		case 0x05: // 8xy5 - SUB Vx, Vy Set Vx = Vx - Vy, set VF = NOT borrow.
			rgs.v[0x0F] = rgs.v[y] > rgs.v[x];
			rgs.v[x] -= rgs.v[y];
			break;
		case 0x06: // 8xy6 - SHR Vx {, Vy} Set Vx = Vx SHR 1.
			rgs.v[0x0F] = rgs.v[x]&0x01;
			rgs.v[x] >>= 1;
			break;
		case 0x07: // 8xy7 - SUBN Vx, Vy Set Vx = Vy - Vx, set VF = NOT borrow.
			rgs.v[0x0F] = rgs.v[x] > rgs.v[y];
			rgs.v[x] = rgs.v[y] - rgs.v[x];
			break;

		case 0x0E: // 8xyE - SHL Vx {, Vy} Set Vx = Vx SHL 1. 
			rgs.v[0x0F] = (rgs.v[x]&0x80) != 0;
			rgs.v[x] <<= 1;
			break;
		}
		break;

	case 0x09: // 9xy0 - SNE Vx, Vy Skip next instruction if Vx != Vy.
		if (rgs.v[x] != rgs.v[y])
			rgs.pc += 2;
		break;
	case 0x0A: break; // Annn - LD I, addr Set I = nnn. The value of register I is set to nnn.
		rgs.i = opcode&0x0FFF;
		break;
	case 0x0B: // Bnnn - JP V0, addr Jump to location nnn + V0.
		rgs.pc = (opcode&0x0FFF) + rgs.v[0];
		break;
	case 0x0C: // Cxkk - RND Vx, byte Set Vx = random byte AND kk.
		rgs.v[x] = rand() & oplo;
		break;

	case 0x0D: // Dxyn - DRW Vx, Vy, nibble Display n-byte sprite starting at memory location I at (Vx, Vy)...
		draw(rgs.v[x], rgs.v[y], oplo&0x0F);
		break;

	case 0x0E: // TODO
		if (oplo == 0x9E) { // Ex9E - SKP Vx Skip next instruction if key with the value of Vx is pressed.

		} else if (oplo == 0xA1) { // ExA1 - SKNP Vx Skip next instruction if key with the value of Vx is not pressed.

		}
		break;

	case 0x0F:
		switch (oplo) {
		case 0x07: // Fx07 - LD Vx, DT Set Vx = delay timer value. The value of DT is placed into Vx.
			rgs.v[x] = rgs.dt;
			break;
		case 0x0A: // TODO Fx0A - LD Vx, K Wait for a key press, store the value of the key in Vx.
			break;
		case 0x15: // Fx15 - LD DT, Vx Set delay timer = Vx.
			rgs.dt = rgs.v[x];
			break;
		case 0x18: // Fx18 - LD ST, Vx Set sound timer = Vx.
			rgs.st = rgs.v[x];
			break;
		case 0x1E: // Fx1E - ADD I, Vx Set I = I + Vx.
			rgs.i += rgs.v[x];
			break;
		case 0x29: // Fx29 - LD F, Vx Set I = location of sprite for digit Vx.
			rgs.i = rgs.v[x] * 5;
			break;
		case 0x33: // TODO Fx33 - LD B, Vx Store BCD representation of Vx in memory locations I, I+1, and I+2. The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
			ram[rgs.i + 2] = rgs.v[x] % 10;
			ram[rgs.i + 1] = (rgs.v[x] / 10) % rgs.v[x] % 10;
			ram[rgs.i] = rgs.v[x] / 100;
			break;
		case 0x55: // Fx55 - LD [I], Vx Store registers V0 through Vx in memory starting at location I.
			memcpy(&ram[rgs.i], &rgs.v[0], x + 1);
			break;
		case 0x65: // Fx65 - LD Vx, [I] Read registers V0 through Vx from memory starting at location I.
			memcpy(&rgs.v[0], &ram[rgs.i], x + 1);
			break;
		}
		break;
	}

}


