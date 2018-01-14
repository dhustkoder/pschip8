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
#include "system.h"


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack


uint16_t sys_paddata;
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;
const DISPENV* sys_dispenv;
const DRAWENV* sys_drawenv;

static uint32_t usec_timer_last;
static uint16_t rcnt1_last;

static uint8_t buffer_idx;
static DISPENV dispenv[2];
static DRAWENV drawenv[2];

static short sprites_loaded;
static uint8_t sprites_img_data[64][1024];
static RECT sprites_info[64];


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

static void swap_buffers(void)
{
	ResetGraph(1);
	buffer_idx = 1 - buffer_idx;
	sys_dispenv = &dispenv[buffer_idx];
	sys_drawenv = &drawenv[buffer_idx];
	PutDispEnv(&dispenv[buffer_idx]);
	PutDrawEnv(&drawenv[buffer_idx]);
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
	ClearImage(&(RECT){.x = 0, .y = 0, .w = 1024, .h = 512}, 255, 0, 255);
	DrawSync(0);

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

	buffer_idx = 0;
	sprites_loaded = 0;
	swap_buffers();
	SetDispMask(1);
}

void update_display(const DispFlag flags)
{
	if (flags&DISP_FLAG_DRAW_SYNC)
		DrawSync(0);

	if (flags&DISP_FLAG_VSYNC)
		VSync(0);

	if (flags&DISP_FLAG_SWAP_BUFFERS)
		swap_buffers();
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

void make_sprite_sheet(void* const* timbuffers, const short size)
{
	short i;
	for (i = 0; i < size; ++i) {
		sprites_info[i].w = ((uint16_t*)timbuffers[i])[0];
		sprites_info[i].h = ((uint16_t*)timbuffers[i])[1];
		sprites_info[i].x = 0;
		sprites_info[i].y = 0;
		memcpy(&sprites_img_data[i][0],
		      ((uint8_t*)timbuffers[i]) + sizeof(uint32_t),
		      sprites_info[i].w * sprites_info[i].h * sizeof(uint16_t));
	}

	sprites_loaded = size;
}

void set_sprite_pos(short sprite_id, short x, short y)
{
	sprites_info[sprite_id].x = x;
	sprites_info[sprite_id].y = y;
}

void draw_sprites(void)
{
	short i;
	RECT rect;
	for (i = 0; i < sprites_loaded; ++i) {
		rect.w = sprites_info[i].w;
		rect.h = sprites_info[i].h;
		rect.x = drawenv->clip.x + sprites_info[i].x;
		rect.y = drawenv->clip.y + sprites_info[i].y;
		LoadImage2(&rect, (void*)&sprites_img_data[i][0]);
	}
	DrawSync(0);
}

void load_files(const char* const* const filenames, 
                void* const* const dsts, 
                const int nfiles)
{
	CdlFILE fp;
	int i, j, cnt;
	char namebuff[16];

	CdInit();

	for (i = 0; i < nfiles; ++i) {
		strcpy(namebuff, filenames[i]);
		loginfo("LOADING %s...\n", namebuff);

		for (j = 0; j < 10; j++) {
			if (!CdSearchFile(&fp, namebuff))
				continue;
		}

		if (j == 10)
			fatal_failure("Couldn't read file %s from CDROM\n", namebuff);

		for (j = 0; j < 10; ++j) {
			CdReadFile(namebuff, (void*)dsts[i], fp.size);

			while ((cnt = CdReadSync(1, NULL)) > 0)
					VSync(0);
				
			if (cnt == 0)
				break;
		}

		if (j == 10)
			fatal_failure("Couldn't read file %s from CDROM\n", namebuff);
	}

	CdStop();
}



