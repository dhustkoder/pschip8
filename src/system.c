#include <stdlib.h>
#include <string.h>
#include <libmath.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libcd.h>
#include <libspu.h>
#include <libapi.h>
#include "chip8.h"
#include "log.h"
#include "system.h"


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack


uint16_t sys_paddata;
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;


static uint32_t usec_timer_last;
static uint16_t rcnt1_last;

static uint8_t buffer_idx;
static DISPENV dispenv[2];
static DRAWENV drawenv[2];


static inline void update_pads(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = PadRead(0);
}

static void vsync_callback(void)
{
	update_pads();
	update_timers();
}


void init_system(void)
{
	int i;

	StopCallback();
	ResetCallback();
	RestartCallback();

	// wait ps logo music fade out
	i = VSync(-1);
	while ((VSync(-1) - i) < 8 * 60)
		;

	ResetGraph(0);
	SpuInit();
	PadInit(0);
	sys_paddata = 0;
	reset_timers();
	VSyncCallback(vsync_callback);

	// wait  gpu warm up
	i = VSync(-1);
	while ((VSync(-1) - i) < 2 * 60)
		;

	#ifdef DISPLAY_TYPE_PAL
	SetVideoMode(MODE_PAL);
	#else
	SetVideoMode(MODE_NTSC);
	#endif

	// clears the whole framebuffer 
	ClearImage(&(RECT){.x = 0, .y = 0, .w = 1024, .h = 512}, 50, 50, 50);
	DrawSync(0);

	buffer_idx = 0;

	SetDefDispEnv(&dispenv[0], 0, 0,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	SetDefDrawEnv(&drawenv[0], 0, SCREEN_HEIGHT,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	SetDefDispEnv(&dispenv[1], 0, SCREEN_HEIGHT,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	SetDefDrawEnv(&drawenv[1], 0, 0,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	drawenv[0].isbg = drawenv[1].isbg = 1;
	drawenv[0].r0 = drawenv[1].r0 = 50;
	drawenv[0].g0 = drawenv[1].g0 = 50;
	drawenv[0].b0 = drawenv[1].b0 = 50;

	PutDispEnv(&dispenv[buffer_idx]);
	PutDrawEnv(&drawenv[buffer_idx]);
	SetDispMask(1);
}

void update_display(const DispFlag flags)
{
	if (flags&DISP_FLAG_DRAW_SYNC)
		DrawSync(0);

	if (flags&DISP_FLAG_VSYNC)
		VSync(0);

	if (flags&DISP_FLAG_SWAP_BUFFERS) {
		ResetGraph(1);
		buffer_idx = 1 - buffer_idx;
		PutDispEnv(&dispenv[buffer_idx]);
		PutDrawEnv(&drawenv[buffer_idx]);
	}
}

void update_timers(void)
{
	const long rcnt1 = GetRCnt(RCntCNT1);

	if (rcnt1 < rcnt1_last) {
		sys_usec_timer += (0xFFFF - rcnt1_last) * 64u;
		rcnt1_last = 0;
	}

	sys_usec_timer += (rcnt1 - rcnt1_last) * 64u;
	
	if ((sys_usec_timer - usec_timer_last) >= 1000u) {
		sys_msec_timer += (sys_usec_timer - usec_timer_last) / 1000u;
		usec_timer_last = sys_usec_timer;
	}

	rcnt1_last = rcnt1;
}

void reset_timers(void)
{
	EnterCriticalSection();
	rcnt1_last = 0;
	sys_usec_timer = 0;
	sys_msec_timer = 0;
	usec_timer_last = 0;
	StopRCnt(RCntCNT1);
	SetRCnt(RCntCNT1, 0xFFFF, RCntMdNOINTR);
	ResetRCnt(RCntCNT1);
	StartRCnt(RCntCNT1);
	ExitCriticalSection();
}


void open_cd_files(const char* const* const filenames, 
                   uint8_t* const* const dsts, 
                   const int nfiles)
{
	CdlFILE fp;
	int i, j, nsector, mode;
	char namebuff[10];

	CdInit();

	for (i = 0; i < nfiles; ++i) {
		strcpy(namebuff, filenames[i]);
		loginfo("LOADING %s...\n", namebuff);
		for (j = 0; j < 10; j++) {
			if (!CdSearchFile(&fp, namebuff))
				continue;

			CdControl(CdlSetloc, (void*)&fp.pos, NULL);
			nsector = (fp.size / 2048) + 1;

			mode = CdlModeSpeed;
			CdControlB(CdlSetmode, (void*)&mode, 0);
			VSync(3);

			CdRead(nsector, (void*)dsts[i], mode);

			while (CdReadSync(1, 0) > 0)
				VSync(0);

			break;
		}

		if (j == 10) {
			fatal_failure("Couldn't read file %s from CDROM\n",
			              namebuff);
		}
	}

	CdStop();
}



