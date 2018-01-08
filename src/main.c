#include <stdlib.h>
#include <stdio.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include "types.h"
#include "log.h"
#include "chip8.h"


#define SCREEN_WIDTH  320     // screen width
#define SCREEN_HEIGHT 256     // screen height (240 NTSC, 256 PAL)

u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack
uint16_t paddata;

static DISPENV dispenv;
static uint16_t screen[32][64];
static const uint8_t chip8rom_brix[] = {
	0x6E, 0x05, 0x65, 0x00, 0x6B, 0x06, 0x6A, 0x00, 0xA3, 0x0C, 0xDA, 0xB1, 0x7A, 0x04, 0x3A, 0x40, 
	0x12, 0x08, 0x7B, 0x02, 0x3B, 0x12, 0x12, 0x06, 0x6C, 0x20, 0x6D, 0x1F, 0xA3, 0x10, 0xDC, 0xD1, 
	0x22, 0xF6, 0x60, 0x00, 0x61, 0x00, 0xA3, 0x12, 0xD0, 0x11, 0x70, 0x08, 0xA3, 0x0E, 0xD0, 0x11, 
	0x60, 0x40, 0xF0, 0x15, 0xF0, 0x07, 0x30, 0x00, 0x12, 0x34, 0xC6, 0x0F, 0x67, 0x1E, 0x68, 0x01, 
	0x69, 0xFF, 0xA3, 0x0E, 0xD6, 0x71, 0xA3, 0x10, 0xDC, 0xD1, 0x60, 0x04, 0xE0, 0xA1, 0x7C, 0xFE, 
	0x60, 0x06, 0xE0, 0xA1, 0x7C, 0x02, 0x60, 0x3F, 0x8C, 0x02, 0xDC, 0xD1, 0xA3, 0x0E, 0xD6, 0x71, 
	0x86, 0x84, 0x87, 0x94, 0x60, 0x3F, 0x86, 0x02, 0x61, 0x1F, 0x87, 0x12, 0x47, 0x1F, 0x12, 0xAC, 
	0x46, 0x00, 0x68, 0x01, 0x46, 0x3F, 0x68, 0xFF, 0x47, 0x00, 0x69, 0x01, 0xD6, 0x71, 0x3F, 0x01, 
	0x12, 0xAA, 0x47, 0x1F, 0x12, 0xAA, 0x60, 0x05, 0x80, 0x75, 0x3F, 0x00, 0x12, 0xAA, 0x60, 0x01, 
	0xF0, 0x18, 0x80, 0x60, 0x61, 0xFC, 0x80, 0x12, 0xA3, 0x0C, 0xD0, 0x71, 0x60, 0xFE, 0x89, 0x03, 
	0x22, 0xF6, 0x75, 0x01, 0x22, 0xF6, 0x45, 0x60, 0x12, 0xDE, 0x12, 0x46, 0x69, 0xFF, 0x80, 0x60, 
	0x80, 0xC5, 0x3F, 0x01, 0x12, 0xCA, 0x61, 0x02, 0x80, 0x15, 0x3F, 0x01, 0x12, 0xE0, 0x80, 0x15, 
	0x3F, 0x01, 0x12, 0xEE, 0x80, 0x15, 0x3F, 0x01, 0x12, 0xE8, 0x60, 0x20, 0xF0, 0x18, 0xA3, 0x0E, 
	0x7E, 0xFF, 0x80, 0xE0, 0x80, 0x04, 0x61, 0x00, 0xD0, 0x11, 0x3E, 0x00, 0x12, 0x30, 0x12, 0xDE, 
	0x78, 0xFF, 0x48, 0xFE, 0x68, 0xFF, 0x12, 0xEE, 0x78, 0x01, 0x48, 0x02, 0x68, 0x01, 0x60, 0x04, 
	0xF0, 0x18, 0x69, 0xFF, 0x12, 0x70, 0xA3, 0x14, 0xF5, 0x33, 0xF2, 0x65, 0xF1, 0x29, 0x63, 0x37, 
	0x64, 0x00, 0xD3, 0x45, 0x73, 0x05, 0xF2, 0x29, 0xD3, 0x45, 0x00, 0xEE, 0xE0, 0x00, 0x80, 0x00, 
	0xFC, 0x00, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00
};


static void init_platform(void)
{
	SetVideoMode(MODE_PAL);
	ResetGraph(0);
	SetDispMask(1);
	SetDefDispEnv(&dispenv, 0, 0, 64, 32);
	PutDispEnv(&dispenv);
	PadInit(0);

	memset(screen, 0x00, sizeof screen);
}

static void update_screen_and_pad(void)
{
	extern bool chip8_scrdata[32][64];
	int i, j;
	u_long padinfo;

	for (i = 0; i < 32; ++i) {
		for (j = 0; j < 64; ++j) {
			screen[i][j] = chip8_scrdata[i][j] ? 0xFFFF : 0x0000;
		}
	}

	LoadImage(&dispenv.disp, (void*)&screen);
	loginfo("DISPENV.DISP: (%d,%d) (%dx%d)\n",
	         dispenv.disp.x, dispenv.disp.y,
	         dispenv.disp.w, dispenv.disp.h);

	padinfo = PadRead(0);
	paddata = padinfo&0x0000FFFF;

	DrawSync(0);
	VSync(0);
}


int main(void)
{
	init_platform();
	update_screen_and_pad();

	chip8_loadrom(chip8rom_brix, sizeof chip8rom_brix);
	chip8_reset();
	for (;;) {
		loginfo("PADDATA: $%.4X\n", paddata);
		chip8_step();
		chip8_logcpu();
		update_screen_and_pad();
	}
}
