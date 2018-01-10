#include <stdlib.h>
#include <sys/errno.h>
#include <libmath.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libapi.h>
#include "chip8.h"
#include "log.h"
#include "system.h"


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack


uint16_t sys_chip8_gfx[CHIP8_HEIGHT][CHIP8_WIDTH];
static uint16_t chip8_gfx_last[CHIP8_HEIGHT][CHIP8_WIDTH];
static uint16_t screen_gfx[SCREEN_HEIGHT][SCREEN_WIDTH];


uint16_t sys_paddata;
int32_t sys_msec_timer;
static uint32_t usec_timer;

static uint16_t rcnt1_last;
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

	// INPUT SYSTEM
	PadInit(0);
	sys_paddata = 0;

	// TIMER SYSTEM
	reset_timers();
}

void update_display(void)
{
	const uint16_t xratio = ((CHIP8_WIDTH<<16)/SCREEN_WIDTH) + 1;
	const uint16_t yratio = ((CHIP8_HEIGHT<<16)/SCREEN_HEIGHT) + 1;
	
	RECT chip8_area = {
		.x = (SCREEN_WIDTH / 2) - (CHIP8_WIDTH / 2),
		.y = (SCREEN_HEIGHT / 2) - (CHIP8_HEIGHT / 2),
		.w = CHIP8_WIDTH,
		.h = CHIP8_HEIGHT
	};

	uint16_t px, py;
	int16_t i, j;
	
	if (memcmp(sys_chip8_gfx, chip8_gfx_last, sizeof chip8_gfx_last) != 0) {
		// scale chip8 graphics
		/*	
		for (i = 0; i < SCREEN_HEIGHT; ++i) {
			for (j = 0; j < SCREEN_WIDTH; ++j) {
				py = (i * yratio)>>16;
				px = (j * xratio)>>16;
				screen_gfx[i][j] = sys_chip8_gfx[py][px];
			}
		}
		*/
		
		for (i = 0; i < CHIP8_HEIGHT; ++i) {
			memcpy(&screen_gfx[chip8_area.y+i][chip8_area.x],
			       &sys_chip8_gfx[i][0],
			       sizeof(uint16_t) * CHIP8_WIDTH);
		}
		
		DrawSync(0);
		LoadImage(&dispenv.disp, (void*)screen_gfx);
		memcpy(chip8_gfx_last, sys_chip8_gfx, sizeof chip8_gfx_last);
	}
}

void update_timers(void)
{
	const long rcnt1 = GetRCnt(RCntCNT1);

	if (rcnt1 < rcnt1_last) {
		usec_timer += (0xFFFF - rcnt1_last) * 64;
		rcnt1_last = 0;
	}

	usec_timer += (rcnt1 - rcnt1_last) * 64;

	if (usec_timer >= 1000) {
		sys_msec_timer += usec_timer / 1000;
		usec_timer -= (usec_timer / 1000) * 1000;
	}

	rcnt1_last = rcnt1;
}

void reset_timers(void)
{
	rcnt1_last = 0;
	usec_timer = 0;
	sys_msec_timer = 0;
	StopRCnt(RCntCNT1);
	SetRCnt(RCntCNT1, 0xFFFF, RCntMdNOINTR);
	StartRCnt(RCntCNT1);
	ResetRCnt(RCntCNT1);
}

void fatal_failure(const char* const msg)
{
	const int err = _get_errno();
	logerror("FATAL FAILURE: %s: ERRNO: %d\n", msg, err);
	SystemError((char)err, err);
}


