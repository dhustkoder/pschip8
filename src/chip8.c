#include <stdio.h>
#include "types.h"
#include "log.h"


static struct {
	uint16_t pc;
	uint16_t i;
	uint8_t sp;
	uint8_t v[0x10];
	uint8_t delay, sound;
} rgs;

static uint8_t ram[0x1000];
static uint16_t stack[16];


static void stackpush(const uint16_t value)
{
	stack[rgs.sp--] = value;
}

static uint16_t stackpop(void)
{
	return stack[++rgs.sp];
}


void chip8_loadrom(const uint8_t* const rom, const uint32_t size)
{
	memcpy(&ram[0x200], rom, size);
}

void chip8_reset(void)
{
	memset(&rgs, 0, sizeof rgs);
	memset(stack, 0, sizeof stack);
	rgs.pc = 0x200;
	rgs.sp = 15;
}

void chip8_logcpu(void)
{
	int i;

	loginfo("PC: $%.4X\n"
	        "SP: $%.4X\n"
	        "I:  $%.4X\n",
	        rgs.pc, rgs.sp, rgs.i);

	for (i = 0; i < 0x10; ++i)
		loginfo("V%.1X: $%.2X\n", i, rgs.v[i]);

	loginfo("Delay Timer: $%.2X\n"
	        "Sound Timer: $%.2X\n",
		rgs.delay, rgs.sound);

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
		case 0xE0: break; // CLS clear display
		case 0xEE: rgs.pc = stackpop(); break; // - RET Return from a subroutine. The interpreter sets the program counter to the address at the top of the stack, then subtracts 1 from the stack pointer.
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
		if ((x) != oplo)
			rgs.pc += 2;
		break;
	case 0x05:  // 5xy0 - SE Vx, Vy Skip next instruction if Vx = Vy.
		if (rgs.v[(x)] == rgs.v[y])
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
			rgs.v[0x0F] = (rgs.v[x] + rgs.[y]) > 0xFF;
			rgs.v[x] += rgs.v[y];
			break;
		case 0x05: // 8xy5 - SUB Vx, Vy Set Vx = Vx - Vy, set VF = NOT borrow.
			rgs.v[0x0F] = rgs.v[x] > rgs.v[y];
			rgs.v[x] -= rgs.v[y];
			break;
		case 0x06: // 8xy6 - SHR Vx {, Vy} Set Vx = Vx SHR 1.
			rgs.v[0x0F] = rgs.v[x]&0x01;
			rgs.v[x] >>= 1;
			break;
		case 0x07: // 8xy7 - SUBN Vx, Vy Set Vx = Vy - Vx, set VF = NOT borrow.
			rgs.v[0x0F] = rgs.v[y] > rgs.v[x];
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
	case 0x0D: 
		/* Dxyn - DRW Vx, Vy, nibble Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision. 
		The interpreter reads n bytes from memory, starting at the address stored in I. 
		These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). 
		Sprites are XORed onto the existing screen. 
		If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. 
		If the sprite is positioned so part of it is outside the coordinates of the display, 
		it wraps around to the opposite side of the screen. See instruction 8xy3 for more information on XOR, 
		and section 2.4, Display, for more information on the Chip-8 screen and sprites.
		*/
		break;
	case 0x0E:
		if (oplo == 0x9E) { // Ex9E - SKP Vx Skip next instruction if key with the value of Vx is pressed.

		} else if (oplo == 0xA1) { // ExA1 - SKNP Vx Skip next instruction if key with the value of Vx is not pressed.

		}
		break;
	case 0x0F:
	
		// Fx07 - LD Vx, DT Set Vx = delay timer value. The value of DT is placed into Vx.
		// Fx0A - LD Vx, K Wait for a key press, store the value of the key in Vx. All execution stops until a key is pressed, then the value of that key is stored in Vx.
		// Fx15 - LD DT, Vx Set delay timer = Vx. DT is set equal to the value of Vx.
		// Fx18 - LD ST, Vx Set sound timer = Vx. ST is set equal to the value of Vx.
		// Fx1E - ADD I, Vx Set I = I + Vx. The values of I and Vx are added, and the results are stored in I.
		// Fx29 - LD F, Vx Set I = location of sprite for digit Vx. The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx. See section 2.4, Display, for more information on the Chip-8 hexadecimal font.
		// Fx33 - LD B, Vx Store BCD representation of Vx in memory locations I, I+1, and I+2. The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
		// Fx55 - LD [I], Vx Store registers V0 through Vx in memory starting at location I. The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
		// Fx65 - LD Vx, [I] Read registers V0 through Vx from memory starting at location I. The interpreter reads values from memory starting at location I into registers V0 through Vx.
		break;
	}

}


