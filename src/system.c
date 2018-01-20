#include <stdlib.h>
#include <string.h>
#include <libmath.h>
#include <libetc.h>
#include <libcd.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <libspu.h>
#include <libapi.h>
#include "chip8.h"
#include "system.h"


#define OT_LENGTH   (10)
#define MAX_PACKETS (64)


enum OTEntry {
	OTENTRY_SPRITE
};


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack


const Vec2* sys_curr_drawvec;

uint16_t sys_paddata;
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;


static uint32_t nsec_timer;
static uint16_t rcnt1_last;

static Vec2 drawvec[2];
static GsOT* curr_drawot;
static GsOT oth[2];
static GsOT_TAG otu[2][1<<OT_LENGTH];
static PACKET gpu_pckt_buff[2][MAX_PACKETS];
static GsSPRITE gs_sprites[MAX_SPRITES];
static RECT bkg_rect;
static bool bkg_loaded;

static inline void update_pads(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = PadRead(0);
}

static void swap_buffers(void)
{
	int idx;
	GsSwapDispBuff();
	idx = GsGetActiveBuff();
	curr_drawot = &oth[idx];
	GsSetWorkBase(gpu_pckt_buff[idx]);
	sys_curr_drawvec = &drawvec[idx];
	GsClearOt(0, 0, curr_drawot);
}

static void vsync_callback(void)
{
	update_timers();
	update_pads();
}


void init_system(void)
{
	short i;

	ResetCallback();
	ResetGraph(0);

	#ifdef DEBUG
	SetGraphDebug(1);
	#else
	SetGraphDebug(0);
	#endif

	#ifdef DISPLAY_TYPE_PAL
	SetVideoMode(MODE_PAL);
	#else
	SetVideoMode(MODE_NTSC);
	#endif

	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsNONINTER|GsOFSGPU, 1, 0);

	drawvec[0].x = drawvec[1].x = 0;
	drawvec[0].y = 0;
	drawvec[1].y = SCREEN_HEIGHT;

	GsDefDispBuff(drawvec[0].x, drawvec[0].y, drawvec[1].x, drawvec[1].y);
	GsClearDispArea(0, 0, 0);

	memset(gs_sprites, 0, sizeof(GsSPRITE) * MAX_SPRITES);
	for (i = 0; i < MAX_SPRITES; ++i)
		gs_sprites[i].attribute = (1<<6)|(1<<25)|(1<<27);

	bkg_loaded = false;
	oth[0].length = OT_LENGTH;
	oth[1].length = OT_LENGTH;
	oth[0].org = otu[0];
	oth[1].org = otu[1];
	swap_buffers();

	InitHeap3((void*)0x8003A000, ((0x801E9CE6 - 0x8003A000) / 8u) * 8u);
	SpuInit();

	PadInit(0);
	sys_paddata = 0;

	reset_timers();
	VSyncCallback(vsync_callback);
	SetDispMask(1);
	update_display(DISPFLAG_SWAPBUFFERS|DISPFLAG_VSYNC);
}

void update_display(const DispFlag flags)
{
	if (flags&DISPFLAG_SWAPBUFFERS) {
		// finish last drawing / frame
		DrawSync(0);
		if (flags&DISPFLAG_VSYNC)
			VSync(0);

		// begin new drawing / frame
		GsDrawOt(curr_drawot);
		swap_buffers();
		if (bkg_loaded) {
			draw_vram_buffer(0, 0, bkg_rect.x, bkg_rect.y,
			                 bkg_rect.w, bkg_rect.h);
		} else {
			GsSortClear(50, 50, 128, curr_drawot);
		}
	} else if (flags&DISPFLAG_VSYNC) {
		VSync(0);
	}
}

void load_bkg_image(const char* const cdpath)
{
	void* p = NULL;

	load_files(&cdpath, &p, 1);

	bkg_rect = (RECT) {
		.x = SCREEN_WIDTH,
		.y = SCREEN_HEIGHT,
		.w = ((uint16_t*)p)[0],
		.h = ((uint16_t*)p)[1]
	};

	LoadImage2(&bkg_rect, (void*)(((uint32_t*)p) + 1));
	bkg_loaded = true;

	free3(p);
}

void load_sprite_sheet(const char* const cdpath)
{
	short i;
	RECT rect;
	u_short tpage_id;
	void* p = NULL;

	load_files(&cdpath, &p, 1);

	rect = (RECT){
		.x = SCREEN_WIDTH,
		.y = 0,
		.w = ((uint16_t*)p)[0],
		.h = ((uint16_t*)p)[1]
	};

	tpage_id = LoadTPage((void*)(((uint32_t*)p) + 1), 2, 0,
	                     rect.x, rect.y, rect.w, rect.h);

	for (i = 0; i < MAX_SPRITES; ++i)
		gs_sprites[i].tpage = tpage_id;

	DrawSync(0);
	free3(p);
}

void draw_sprites(const Sprite* const sprites, const short nsprites)
{
	short i;
	for (i = 0; i < nsprites; ++i) {
		gs_sprites[i].x = sprites[i].spos.x;
		gs_sprites[i].y = sprites[i].spos.y;
		gs_sprites[i].w = sprites[i].size.w;
		gs_sprites[i].h = sprites[i].size.h;
		gs_sprites[i].u = sprites[i].tpos.u;
		gs_sprites[i].v = sprites[i].tpos.v;
		GsSortFastSprite(&gs_sprites[i], curr_drawot, OTENTRY_SPRITE);
	}
}

void draw_ram_buffer_scaled(void* pixels,
                            const short screenx, const short screeny,
                            const short width, const short height,
                            const uint8_t scalex, const uint8_t scaley)
{
	static GsSPRITE sprt;
	static RECT tpage_rect;

	memset(&sprt, 0, sizeof sprt);
	tpage_rect = (RECT) {
		.x = SCREEN_WIDTH + 256,
		.y = 0,
		.w = width,
		.h = height
	};

	LoadImage(&tpage_rect, pixels);
	sprt.attribute = (1<<6)|(1<<25);
	sprt.tpage = GetTPage(2, 0, tpage_rect.x, tpage_rect.y);
	sprt.x = screenx;
	sprt.y = screeny;
	sprt.w = width;
	sprt.h = height;
	sprt.mx = width / 2u;
	sprt.my = height / 2u;
	sprt.scalex = ONE * scalex;
	sprt.scaley = ONE * scaley;

	GsSortSprite(&sprt, curr_drawot, OTENTRY_SPRITE);
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
                void** const dsts, 
                const short nfiles)
{
	CdlFILE fp;
	int i, j, cnt;
	bool need_alloc;
	char namebuff[16];

	CdInit();

	for (i = 0; i < nfiles; ++i) {
		strcpy(namebuff, filenames[i]);
		LOGINFO("LOADING %s...\n", namebuff);

		for (j = 0; j < 10; ++j) {
			if (CdSearchFile(&fp, namebuff) != 0)
				break;
		}

		if (j == 10)
			FATALERROR("Couldn't find file %s in CDROM\n", namebuff);
		
		LOGINFO("Found file %s with size: %lu\n", namebuff, fp.size);

		need_alloc = dsts[i] == NULL;
		if (need_alloc)
			dsts[i] = malloc3(2048 + fp.size);

		for (j = 0; j < 10; ++j) {
			CdReadFile(namebuff, (void*)dsts[i], fp.size);

			while ((cnt = CdReadSync(1, NULL)) > 0)
					VSync(0);
				
			if (cnt == 0)
				break;
		}

		if (j == 10) {
			if (need_alloc)
				free3(dsts[i]);
			FATALERROR("Couldn't read file %s from CDROM\n", namebuff);
		}
	}

	CdStop();
}



