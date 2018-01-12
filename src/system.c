#include <stdlib.h>
#include <string.h>
#include <libmath.h>
#include <libgte.h>
#include <libgpu.h>
#include <libetc.h>
#include <libcd.h>
#include "chip8.h"
#include "log.h"
#include "system.h"


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack


extern bool chip8_gfx[CHIP8_HEIGHT][CHIP8_WIDTH]; // the buffer filled by chip8.c
extern bool chip8_draw_flag;



uint16_t sys_paddata;
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;

static uint32_t usec_timer_last;
static uint16_t rcnt1_last;

static uint16_t scaled_chip8_gfx[CHIP8_SCALED_HEIGHT][CHIP8_SCALED_WIDTH];
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
	loginfo("INIT SYSTEM\n");

	ResetCallback();

	// wait ps logo music fade out
	reset_timers();
	while (sys_msec_timer < 8000)
		update_timers();

	ResetGraph(0);

	// wait gpu warm after ResetGraph
	reset_timers();
	while (sys_msec_timer < 2000)
		update_timers();

	// clears the whole framebuffer 
	ClearImage(&(RECT){.x = 0, .y = 0, .w = 1024, .h = 512}, 50, 50, 50);
	DrawSync(0);

	#ifdef DISPLAY_TYPE_PAL
	SetVideoMode(MODE_PAL);
	#else
	SetVideoMode(MODE_NTSC);
	#endif

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
	

	PadInit(0);
	sys_paddata = 0;

	VSyncCallback(vsync_callback);
}

void update_display(const bool vsync)
{
	const uint32_t xratio = ((CHIP8_WIDTH<<16) / CHIP8_SCALED_WIDTH) + 1;
	const uint32_t yratio = ((CHIP8_HEIGHT<<16) / CHIP8_SCALED_HEIGHT) + 1;

	RECT chip8_rect = {
		.x = (SCREEN_WIDTH / 2) - (CHIP8_SCALED_WIDTH / 2),
		.y = (SCREEN_HEIGHT / 2) - (CHIP8_SCALED_HEIGHT / 2),
		.w = CHIP8_SCALED_WIDTH,
		.h = CHIP8_SCALED_HEIGHT
	};

	uint32_t px, py;
	int16_t i, j;
	
	if (chip8_draw_flag) {
		// scale chip8 graphics
		for (i = 0; i < CHIP8_SCALED_HEIGHT; ++i) {
			py = (i * yratio)>>16;
			for (j = 0; j < CHIP8_SCALED_WIDTH; ++j) {
				px = (j * xratio)>>16;
				scaled_chip8_gfx[i][j] = chip8_gfx[py][px] ? ~0 : 0;
			}
		}

		chip8_rect.x += drawenv[buffer_idx].clip.x;
		chip8_rect.y += drawenv[buffer_idx].clip.y;

		DrawSync(0);
		if (vsync)
			VSync(0);

		LoadImage(&chip8_rect, (void*)scaled_chip8_gfx);
		chip8_draw_flag = 0;
		ResetGraph(1);
		buffer_idx = 1 - buffer_idx;
		PutDispEnv(&dispenv[buffer_idx]);
		PutDrawEnv(&drawenv[buffer_idx]);
	} else if (vsync) {
		VSync(0);
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
	rcnt1_last = 0;
	sys_usec_timer = 0;
	sys_msec_timer = 0;
	usec_timer_last = 0;
	StopRCnt(RCntCNT1);
	SetRCnt(RCntCNT1, 0xFFFF, RCntMdNOINTR);
	StartRCnt(RCntCNT1);
	ResetRCnt(RCntCNT1);
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



