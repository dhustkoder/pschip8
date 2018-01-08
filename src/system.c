#include <stdlib.h>
#include <stdio.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libapi.h>
#include <sys/errno.h>
#include "types.h"
#include "chip8.h"
#include "system.h"


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack

uint16_t sys_screen_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];
uint16_t sys_paddata;
unsigned long sys_msec_timer;
unsigned long sys_usec_timer;


static DISPENV dispenv;

static void fatal_failure(const char* const msg)
{
	const int err = _get_errno();
	printf("FATAL FAILURE: %s\nERRNO: %d\n", msg, err);
	SystemError((char)err, err);
}

void init_systems(void)
{
	// VIDEO SYSTEM
	SetVideoMode(MODE_PAL);
	ResetGraph(0);
	SetDispMask(1);
	
	// init dispenv
	memset(&dispenv, 0, sizeof dispenv);
	dispenv.disp.w = SCREEN_WIDTH;
	dispenv.disp.h = SCREEN_HEIGHT;
	dispenv.screen.w = SCREEN_WIDTH;
	dispenv.screen.h = SCREEN_HEIGHT;
	// ensure fb is full cleared
	PutDispEnv(&dispenv);
	ClearImage2(&dispenv.disp, 0, 0, 0);

	// INPUT SYSTEM
	PadInit(0);

	// TIMER SYSTEM
	// setup timer cnt1
	StopRCnt(RCntCNT1);
	SetRCnt(RCntCNT1, 0xFFFF, RCntMdNOINTR);
	StartRCnt(RCntCNT1);
	reset_timers();

}

void update_display(void)
{
	static int fps = 0;
	static unsigned long msec_last = 0;

	LoadImage(&dispenv.disp, (void*)&sys_screen_buffer);

	DrawSync(0);
	
	VSync(0);

	++fps;

	if ((sys_msec_timer - msec_last) >= 1000) {
		printf("FPS: %d\n", fps);
		fps = 0;
		msec_last = sys_msec_timer;
	}

}

void update_pads(void)
{
	sys_paddata = PadRead(0)&0x0000FFFF;
}

void update_timers(void)
{
	static unsigned short rcnt1_last = 0;

	long rcnt1 = GetRCnt(RCntCNT1);

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
	sys_usec_timer = 0;
	sys_msec_timer = 0;
	ResetRCnt(RCntCNT1);
}


