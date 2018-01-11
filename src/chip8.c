#include <stdlib.h>
#include <rand.h>
#include "ints.h"
#include "log.h"
#include "system.h"
#include "chip8.h"


extern bool sys_chip8_gfx[CHIP8_HEIGHT][CHIP8_WIDTH];


static struct {
	uint16_t pc;
	uint16_t i;
	uint8_t v[0x10];
	uint8_t sp;
	uint8_t dt;
	uint8_t st;
} rgs;

static uint16_t paddata_old;
static uint8_t pressed_key;
static bool waiting_keypress;
static uint8_t ram[0x1000];
static uint16_t stack[16];

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

static void unknown_opcode(const uint16_t opcode)
{
	fatal_failure("Unkown Opcode: $%.4X\n", opcode);
}

static void stackpush(const uint16_t value)
{
	stack[rgs.sp--] = value;
}

static uint16_t stackpop(void)
{
	return stack[++rgs.sp];
}

static void draw(const uint8_t vx, const uint8_t vy, const uint8_t n)
{
	const uint8_t* const sprite = &ram[rgs.i];
	uint16_t pixel;
	uint8_t i, j, x, y;
	
	rgs.v[0x0F] = 0;
	for (i = 0; i < n; ++i) {
		y = (vy + i)%CHIP8_HEIGHT;
		for (j = 0; j < 8; ++j) {
			x = (vx + j)%CHIP8_WIDTH;
			pixel = (sprite[i]&(0x80>>j)) != 0;
			rgs.v[0x0F] |= sys_chip8_gfx[y][x] && pixel;
			sys_chip8_gfx[y][x] ^= pixel;
		}
	}
}

static void update_keys(void)
{
	extern uint16_t sys_paddata;

	static const uint8_t keytable[14] = {
		0x2, 0x8, 0x4, 0x6,
		0x5, 0x1, 0x3, 0x7,
		0xf, 0xa, 0xa, 0x0,
		0xb, 0xc
	};

	static const uint16_t padtable[14] = {
		BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT,
		BUTTON_CIRCLE, BUTTON_CROSS, BUTTON_SQUARE, BUTTON_TRIANGLE,
		BUTTON_START, BUTTON_SELECT, BUTTON_R1, BUTTON_R2,
		BUTTON_L1, BUTTON_L2
	};


	bool paddata_change;
	uint8_t i;

	update_pads();
	paddata_change = sys_paddata != paddata_old;

	if (paddata_change) {
		pressed_key = 0xFF;
		for (i = 0; i < 14; ++i) {
			if (sys_paddata&padtable[i]) {
				pressed_key = keytable[i];
				break;
			}
		}
		paddata_old = sys_paddata;
	}
}

static void update_dt_st(void)
{
	static uint32_t msec_last = 0;
	const uint32_t timer = get_usec_now();

	while ((timer - msec_last) >= (1000000 / CHIP8_DELAY_FREQ)) {
		msec_last += (1000000 / CHIP8_DELAY_FREQ);
		if (rgs.dt > 0)
			--rgs.dt;
		if (rgs.st > 0)
			--rgs.st;
	}
}


void chip8_loadrom(const char* const fname)
{
	uint8_t* const p = &ram[0x200];
	const char* const fnames[] = { fname };
	uint8_t* const dsts[] = { p };
	open_cd_files(fnames, dsts, 1);
}

void chip8_reset(void)
{
	memset(&rgs, 0, sizeof rgs);
	memset(stack, 0, sizeof stack);
	memset(sys_chip8_gfx, 0, sizeof sys_chip8_gfx);
	memcpy(ram, font, sizeof font);
	rgs.pc = 0x200;
	rgs.sp = 15;
	paddata_old = 0x0000;
	pressed_key = 0xFF;
	waiting_keypress = false;
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
	uint8_t ophi, oplo, x, y;
	uint16_t opcode;

	update_dt_st();
	update_keys();

	if (waiting_keypress) {
		if (pressed_key == 0xFF) {
			logdebug("Waiting Key Press...\n");
			return;
		} else {
			logdebug("Key Press Received!\n");
			waiting_keypress = false;
		}
	}

	ophi = ram[rgs.pc++];
	oplo = ram[rgs.pc++];
	x = ophi&0x0F;
	y = (oplo&0xF0)>>4;
	opcode = (ophi<<8)|oplo;

	logdebug("PRESSED KEY: $%.2X\n", pressed_key);
	logdebug("OPCODE: $%.4X\n", opcode);

	switch ((ophi&0xF0)>>4) {
	default: unknown_opcode(opcode); break;
	case 0x00:
		switch (oplo) {
		default: unknown_opcode(opcode); break;
		case 0xE0: // - CLS clear display
			memset(sys_chip8_gfx, 0, sizeof sys_chip8_gfx);
			break;
		case 0xEE: // - RET Return from a subroutine.
			rgs.pc = stackpop();
			break;
		}
		break;

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
	case 0x06:  // 6xkk - LD Vx, byte Set Vx = kk.
		rgs.v[x] = oplo;
		break;
	case 0x07:  // 7xkk - ADD Vx, byte Set Vx = Vx + kk.
		rgs.v[x] += oplo;
		break;

	case 0x08:
		switch (oplo&0x0F) {
		default: unknown_opcode(opcode); break;
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
	case 0x0A: // Annn - LD I, addr Set I = nnn. The value of register I is set to nnn.
		rgs.i = opcode&0x0FFF;
		break;
	case 0x0B: // Bnnn - JP V0, addr Jump to location nnn + V0.
		rgs.pc = (opcode&0x0FFF) + rgs.v[0];
		break;
	case 0x0C: // Cxkk - RND Vx, byte Set Vx = random byte AND kk.
		rgs.v[x] = rand()&oplo;
		break;
	case 0x0D: // Dxyn - DRW Vx, Vy, nibble Display n-byte sprite starting at memory location I at (Vx, Vy)...
		draw(rgs.v[x], rgs.v[y], oplo&0x0F);
		break;
	case 0x0E:
		if (oplo == 0x9E) { // Ex9E - SKP Vx Skip next instruction if key with the value of Vx is pressed.
			if (pressed_key == rgs.v[x])
				rgs.pc += 2;
		} else if (oplo == 0xA1) { // ExA1 - SKNP Vx Skip next instruction if key with the value of Vx is not pressed.
			if (pressed_key != rgs.v[x])
				rgs.pc += 2;
		}
		break;
	case 0x0F:
		switch (oplo) {
		default: unknown_opcode(opcode); break;
		case 0x07: // Fx07 - LD Vx, DT Set Vx = delay timer value. The value of DT is placed into Vx.
			rgs.v[x] = rgs.dt;
			break;
		case 0x0A: // Fx0A - LD Vx, K Wait for a key press, store the value of the key in Vx.
			paddata_old = 0x0000;
			pressed_key = 0xFF;
			waiting_keypress = true;
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
		case 0x33: // Fx33 - LD B, Vx Store BCD representation of Vx in memory locations I, I+1, and I+2.
			ram[rgs.i + 2] = rgs.v[x] % 10;
			ram[rgs.i + 1] = (rgs.v[x] / 10) % 10;
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


