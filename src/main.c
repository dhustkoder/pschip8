#include <stdlib.h>
#include <stdio.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <libetc.h>
#include <libpad.h>
#include "types.h"
#include "log.h"
#include "chip8.h"


#define OT_LENGTH     14            // the ordertable length
#define PACKETMAX     1024 * 720   // the maximum number of objects on the screen
#define SCREEN_WIDTH  320           // screen width
#define SCREEN_HEIGHT 256           // screen height (240 NTSC, 256 PAL)

u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack
uint16_t paddata;


static GsOT_TAG myOT_TAG[2][1<<OT_LENGTH]; // ordering table unit
static GsOT myOT[2] = {                    // ordering table header
	{ 
		.length = OT_LENGTH, .org = myOT_TAG[0]
	},
	{
		.length = OT_LENGTH, .org = myOT_TAG[1]
	} 
};
static PACKET GPUPacketArea[2][PACKETMAX]; // GPU packet data
static short current_buffer = 0;           // holds the current buffer number

static const uint8_t chip8rom_brix[] = {
	0xA2, 0xCD, 0x69, 0x38, 0x6A, 0x08, 0xD9, 0xA3, 0xA2, 0xD0, 0x6B, 0x00, 0x6C, 0x03, 0xDB, 0xC3, 0xA2, 0xD6, 0x64, 0x1D, 0x65, 0x1F, 0xD4, 0x51, 0x67, 0x00, 0x68, 0x0F, 0x22, 0xA2, 0x22, 0xAC, 
	0x48, 0x00, 0x12, 0x22, 0x64, 0x1E, 0x65, 0x1C, 0xA2, 0xD3, 0xD4, 0x53, 0x6E, 0x00, 0x66, 0x80, 0x6D, 0x04, 0xED, 0xA1, 0x66, 0xFF, 0x6D, 0x05, 0xED, 0xA1, 0x66, 0x00, 0x6D, 0x06, 0xED, 0xA1, 
	0x66, 0x01, 0x36, 0x80, 0x22, 0xD8, 0xA2, 0xD0, 0xDB, 0xC3, 0xCD, 0x01, 0x8B, 0xD4, 0xDB, 0xC3, 0x3F, 0x00, 0x12, 0x92, 0xA2, 0xCD, 0xD9, 0xA3, 0xCD, 0x01, 0x3D, 0x00, 0x6D, 0xFF, 0x79, 0xFE, 
	0xD9, 0xA3, 0x3F, 0x00, 0x12, 0x8C, 0x4E, 0x00, 0x12, 0x2E, 0xA2, 0xD3, 0xD4, 0x53, 0x45, 0x00, 0x12, 0x86, 0x75, 0xFF, 0x84, 0x64, 0xD4, 0x53, 0x3F, 0x01, 0x12, 0x46, 0x6D, 0x08, 0x8D, 0x52, 
	0x4D, 0x08, 0x12, 0x8C, 0x12, 0x92, 0x22, 0xAC, 0x78, 0xFF, 0x12, 0x1E, 0x22, 0xA2, 0x77, 0x05, 0x12, 0x96, 0x22, 0xA2, 0x77, 0x0F, 0x22, 0xA2, 0x6D, 0x03, 0xFD, 0x18, 0xA2, 0xD3, 0xD4, 0x53, 
	0x12, 0x86, 0xA2, 0xF8, 0xF7, 0x33, 0x63, 0x00, 0x22, 0xB6, 0x00, 0xEE, 0xA2, 0xF8, 0xF8, 0x33, 0x63, 0x32, 0x22, 0xB6, 0x00, 0xEE, 0x6D, 0x1B, 0xF2, 0x65, 0xF0, 0x29, 0xD3, 0xD5, 0x73, 0x05, 
	0xF1, 0x29, 0xD3, 0xD5, 0x73, 0x05, 0xF2, 0x29, 0xD3, 0xD5, 0x00, 0xEE, 0x01, 0x7C, 0xFE, 0x7C, 0x60, 0xF0, 0x60, 0x40, 0xE0, 0xA0, 0xF8, 0xD4, 0x6E, 0x01, 0x6D, 0x10, 0xFD, 0x18, 0x00, 0xEE
};

static void init_platform(void)
{
	// VIDEO	
	SetVideoMode(MODE_PAL);

	// set the graphics mode resolutions (GsNONINTER for NTSC, and GsINTER for PAL)
	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsINTER|GsOFSGPU, 1, 0);

	// tell the GPU to draw from the top left coordinates of the framebuffer
	GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT);
	
	// clear the ordertables
	GsClearOt(0, 0, &myOT[0]);
	GsClearOt(0, 0, &myOT[1]);
}

static void update_screen_and_pad(void)
{
	extern bool chip8_scrdata[32][64];

	GsBOXF box = {
		.attribute = 0,
		.w = 5,
		.h = 8,
		.x = 0,
		.y = 0,
		.r = 0xFF,
		.g = 0xFF,
		.b = 0xFF
	};

	int i, j, otpos = 0;
	u_long padinfo;

	PadInit(0);

	current_buffer = GsGetActiveBuff();
	GsSetWorkBase((PACKET*)GPUPacketArea[current_buffer]);
	GsClearOt(0, 0, &myOT[current_buffer]);
	DrawSync(0);
	VSync(0);

	GsSwapDispBuff();
	GsSortClear(50, 50, 50, &myOT[current_buffer]);

	for (i = 0; i < 32; ++i, box.y += 8) {
		for (j = 0, box.x = 0; j < 64; ++j, box.x += 5) {
			if (chip8_scrdata[i][j])
				GsSortBoxFill(&box, &myOT[current_buffer], otpos++);
		}
	}

	GsDrawOt(&myOT[current_buffer]);
	
	padinfo = PadRead(0);
	paddata = padinfo&0x0000FFFF;
	PadStop();
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
