#include <stdlib.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libapi.h>
#include <sys/errno.h>
#include "types.h"
#include "chip8.h"
#include "log.h"
#include "system.h"


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack

uint16_t sys_chip8_gfx[CHIP8_HEIGHT][CHIP8_WIDTH];
static uint16_t chip8_gfx_last[CHIP8_HEIGHT][CHIP8_WIDTH];

uint16_t sys_paddata;
unsigned long sys_msec_timer;

static unsigned long sys_usec_timer;
static unsigned short rcnt1_last;

static DISPENV dispenv;

void init_systems(void)
{
	// VIDEO SYSTEM
	ResetGraph(0);
	SetDispMask(1);
	SetVideoMode(MODE_PAL);

	// init dispenv
	memset(&dispenv, 0, sizeof dispenv);
	dispenv.disp.w = SCREEN_WIDTH;
	dispenv.disp.h = SCREEN_HEIGHT;
	dispenv.screen.w = SCREEN_WIDTH;
	dispenv.screen.h = SCREEN_HEIGHT;

	PutDispEnv(&dispenv);
	ClearImage(&dispenv.disp, 0, 0, 0);

	FntLoad(960, 256);
	SetDumpFnt(FntOpen(32, 32, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 32));

	// INPUT SYSTEM
	PadInit(0);
	sys_paddata = 0;

	// TIMER SYSTEM
	reset_timers();
}

void update_display(void)
{
	static int fps = 0;
	static unsigned long msec_last = 0;

	RECT chip8_area = {
		.x = (SCREEN_WIDTH / 2) - CHIP8_WIDTH,
		.y = (SCREEN_HEIGHT / 2) - CHIP8_HEIGHT,
		.w = CHIP8_WIDTH,
		.h = CHIP8_HEIGHT
	};

	++fps;

	if ((sys_msec_timer - msec_last) >= 1000) {
		msec_last = sys_msec_timer;
		FntPrint("FPS: %d", fps);
		FntFlush(-1);
		fps = 0;
	}

	if (fps == 0 || memcmp(sys_chip8_gfx, chip8_gfx_last, sizeof chip8_gfx_last) != 0) {
		LoadImage(&chip8_area, (void*)&sys_chip8_gfx);
		memcpy(chip8_gfx_last, sys_chip8_gfx, sizeof chip8_gfx_last);
	}

}

void update_pads(void)
{
	sys_paddata = PadRead(0)&0x0000FFFF;
}

void update_timers(void)
{
	const long rcnt1 = GetRCnt(RCntCNT1);

	if (rcnt1 < rcnt1_last) {
		sys_usec_timer += (0xFFFF - rcnt1_last) * 64;
		rcnt1_last = 0;
	}

	sys_usec_timer += (rcnt1 - rcnt1_last) * 64;

	if (sys_usec_timer >= 1000) {
		sys_msec_timer += sys_usec_timer / 1000;
		sys_usec_timer -= (sys_usec_timer / 1000) * 1000;
	}

	rcnt1_last = rcnt1;
}

void reset_timers(void)
{
	rcnt1_last = 0;
	sys_usec_timer = 0;
	sys_msec_timer = 0;
	StopRCnt(RCntCNT1);
	SetRCnt(RCntCNT1, 0xFFFF, RCntMdNOINTR);
	StartRCnt(RCntCNT1);
}

void fatal_failure(const char* const msg)
{
	const int err = _get_errno();
	logerror("FATAL FAILURE: %s: ERRNO: %d\n", msg, err);
	SystemError((char)err, err);
}

