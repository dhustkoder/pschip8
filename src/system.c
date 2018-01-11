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


bool sys_chip8_gfx[CHIP8_HEIGHT][CHIP8_WIDTH]; // the buffer filled by chip8.c
static bool chip8_gfx_last[CHIP8_HEIGHT][CHIP8_WIDTH]; // save the last drawn buffer
static uint16_t chip8_disp_buffer[CHIP8_SCALED_HEIGHT][CHIP8_SCALED_WIDTH]; // the scaled display buffer for chip8 gfx


uint16_t sys_paddata;
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;

static uint32_t usec_timer_last;
static uint16_t rcnt1_last;

static uint8_t buffer_idx;
static DISPENV dispenv[2];
static DRAWENV drawenv[2];


void init_systems(void)
{
	// VIDEO SYSTEM
	ResetCallback();
	ResetGraph(0);
	#ifdef DISPLAY_TYPE_PAL
	SetVideoMode(MODE_PAL);
	#else
	SetVideoMode(MODE_NTSC);
	#endif

	SetDispMask(1);

	buffer_idx = 0;

	SetDefDispEnv(&dispenv[0], 0, 0,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	SetDefDrawEnv(&drawenv[0], 0, SCREEN_HEIGHT,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	SetDefDispEnv(&dispenv[1], 0, SCREEN_HEIGHT,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	SetDefDrawEnv(&drawenv[1], 0, 0,
	              SCREEN_WIDTH, SCREEN_HEIGHT);

	ClearImage(&dispenv[0].disp, 50, 50, 50);
	DrawSync(0);
	ClearImage(&drawenv[0].clip, 50, 50, 50);
	DrawSync(0);

	PutDispEnv(&dispenv[buffer_idx]);
	PutDrawEnv(&drawenv[buffer_idx]);

	FntLoad(SCREEN_WIDTH, 0);
	SetDumpFnt(FntOpen(0, 16, 128, 64, 0, 128));


	// INPUT SYSTEM
	PadInit(0);
	sys_paddata = 0;

	// TIMER SYSTEM
	reset_timers();
	VSyncCallback(update_timers);
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
	
	if (memcmp(sys_chip8_gfx, chip8_gfx_last, sizeof chip8_gfx_last) != 0) {
		// scale chip8 graphics
		for (i = 0; i < CHIP8_SCALED_HEIGHT; ++i) {
			py = (i * yratio)>>16;
			for (j = 0; j < CHIP8_SCALED_WIDTH; ++j) {
				px = (j * xratio)>>16;
				chip8_disp_buffer[i][j] = sys_chip8_gfx[py][px] ? ~0 : 0;
			}
		}

		chip8_rect.x += drawenv[buffer_idx].clip.x;
		chip8_rect.y += drawenv[buffer_idx].clip.y;

		LoadImage(&chip8_rect, (void*)chip8_disp_buffer);
		memcpy(chip8_gfx_last, sys_chip8_gfx, sizeof chip8_gfx_last);
		DrawSync(0);

		if (vsync)
			VSync(0);

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
	
	if ((sys_usec_timer - usec_timer_last) >= 1000) {
		sys_msec_timer += (sys_usec_timer - usec_timer_last) / 1000;
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
		for (j=0; j < 10; j++) {
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



