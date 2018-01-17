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

const DISPENV* sys_curr_dispenv;
const DRAWENV* sys_curr_drawenv;

static uint32_t nsec_timer;
static uint16_t rcnt1_last;

static DISPENV dispenv[2];
static DRAWENV drawenv[2];

static SPRT sprt_prims[32];
static unsigned long ot[64];
static unsigned long tpage_id;
static RECT spritesheet_rect;
static RECT bkg_rect;
static bool bkg_img_loaded;

static inline void update_pads(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = PadRead(0);
}

static void swap_buffers(void)
{
	static uint8_t buffer_idx = 1;

	ResetGraph(1);
	buffer_idx = 1 - buffer_idx;
	sys_curr_dispenv = &dispenv[buffer_idx];
	sys_curr_drawenv = &drawenv[buffer_idx];
	PutDispEnv(&dispenv[buffer_idx]);
	PutDrawEnv(&drawenv[buffer_idx]);
	if (bkg_img_loaded) {
		MoveImage(&bkg_rect,
		          sys_curr_drawenv->clip.x,
		          sys_curr_drawenv->clip.y);
	}
}

static void vsync_callback(void)
{
	update_timers();
	update_pads();
}


void init_system(void)
{
	ResetCallback();
	ResetGraph(0);

	#ifdef DISPLAY_TYPE_PAL
	SetVideoMode(MODE_PAL);
	#else
	SetVideoMode(MODE_NTSC);
	#endif

	SpuInit();
	PadInit(0);

	sys_paddata = 0;
	bkg_img_loaded = false;

	SetDefDispEnv(&dispenv[0], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDrawEnv(&drawenv[0], 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDispEnv(&dispenv[1], 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDrawEnv(&drawenv[1], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	dispenv[0].screen.w = dispenv[1].screen.w = SCREEN_WIDTH;
	dispenv[0].screen.h = dispenv[1].screen.h = SCREEN_HEIGHT;
	drawenv[0].isbg = drawenv[1].isbg = 1;
	drawenv[0].r0 = drawenv[1].r0 = 50;
	drawenv[0].g0 = drawenv[1].g0 = 50;
	drawenv[0].b0 = drawenv[1].b0 = 127;

	// clears the whole framebuffer 
	ClearImage(&(RECT){.x = 0, .y = 0, .w = 1024, .h = 512}, 255, 0, 255);

	#ifdef DEBUG
	SetGraphDebug(1);
	#else
	SetGraphDebug(0);
	#endif

	swap_buffers();
	reset_timers();
	VSyncCallback(vsync_callback);
	SetDispMask(1);
}

void update_display(const DispFlag flags)
{
	if (flags&DISPFLAG_DRAWSYNC)
		DrawSync(0);
	if (flags&DISPFLAG_VSYNC)
		VSync(0);
	if (flags&DISPFLAG_SWAPBUFFERS)
		swap_buffers();
}

void load_bkg_image(const char* const cdpath)
{
	void* const p = malloc((2048 * 114) + 2048);
	load_files(&cdpath, &p, 1);

	bkg_rect = (RECT) {
		.x = SCREEN_WIDTH,
		.y = SCREEN_HEIGHT,
		.w = ((uint16_t*)p)[0],
		.h = ((uint16_t*)p)[1]
	};

	LoadImage2(&bkg_rect, ((uint32_t*)p) + 1);
	bkg_img_loaded = true;

	free(p);
}

void load_sprite_sheet(const char* const cdpath)
{
	short i;
	void* const p = malloc((2048 * 114) + 2048);

	load_files(&cdpath, &p, 1);

	spritesheet_rect = (RECT){
		.x = SCREEN_WIDTH,
		.y = 0,
		.w = ((uint16_t*)p)[0],
		.h = ((uint16_t*)p)[1]
	};

	for (i = 0; i < 32; ++i) {
		SetSprt(&sprt_prims[i]);
		setRGB0(&sprt_prims[i], 127, 127, 127);
	}

	tpage_id = LoadTPage((void*)(((uint32_t*)p) + 1),
	                     2, 0,
	                     spritesheet_rect.x, spritesheet_rect.y,
	                     spritesheet_rect.w, spritesheet_rect.h);

	DrawSync(0);
	free(p);
}

void draw_sprites(const Sprite* const sprites, const short size)
{
	DR_TPAGE tpage;
	short i;

	ClearOTag(ot, sizeof(ot) / sizeof(ot[0]));
	SetDrawTPage(&tpage, 0, 1, tpage_id);
	AddPrim(&ot[0], &tpage);

	for (i = 0; i < size; ++i) {
		setXY0(&sprt_prims[i], sprites[i].spos.x, sprites[i].spos.y);
		setUV0(&sprt_prims[i], sprites[i].tpos.u, sprites[i].tpos.v);
		setWH(&sprt_prims[i], sprites[i].size.w, sprites[i].size.h);
	}

	AddPrims(&ot[1], &sprt_prims[0], &sprt_prims[size - 1]);
	DrawOTag(ot);
}

void update_timers(void)
{
	const uint16_t rcnt1 = GetRCnt(RCntCNT1);

	nsec_timer += ((uint16_t)(rcnt1 - rcnt1_last)) * NSEC_PER_HSYNC;
	
	sys_usec_timer += nsec_timer / 1000u;
	sys_msec_timer += nsec_timer / 1000000u;

	nsec_timer -= (nsec_timer / 1000u) * 1000u;
	rcnt1_last = rcnt1;
}

void reset_timers(void)
{
	sys_msec_timer = 0;
	sys_usec_timer = 0;
	nsec_timer = 0;
	rcnt1_last = 0;
	SetRCnt(RCntCNT1, 0xFFFF, RCntMdNOINTR);
	ResetRCnt(RCntCNT1);
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

		for (j = 0; j < 10; ++j) {
			if (CdSearchFile(&fp, namebuff) != 0)
				break;
		}

		if (j == 10)
			fatal_failure("Couldn't find file %s in CDROM\n", namebuff);
		
		loginfo("Found file %s with size: %lu\n", namebuff, fp.size);

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



