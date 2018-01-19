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


enum OTEntry {
	OTENTRY_SPRITE
};


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack


uint16_t sys_paddata;
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;

static uint32_t nsec_timer;
static uint16_t rcnt1_last;

static GsOT* current_oth;
static GsOT oth[2];
static GsOT_TAG otu[2][1<<10];
static PACKET gpu_pckt_area[2][64 * 1000];
static bool bkg_loaded;
static GsSPRITE bkg_sprite;
static GsSPRITE gs_sprites[MAX_SPRITES];

static inline void update_pads(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = PadRead(0);
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

	#ifdef DISPLAY_TYPE_PAL
	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsINTER|GsOFSGPU, 1, 0);
	#else
	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsNONINTER|GsOFSGPU, 1, 0);
	#endif
	GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT);

	memset(gs_sprites, 0, sizeof(GsSPRITE) * MAX_SPRITES);
	memset(&bkg_sprite, 0, sizeof bkg_sprite);

	bkg_sprite.attribute = (1<<6);
	bkg_sprite.attribute = (1<<25);
	bkg_sprite.scalex = 4096;
	bkg_sprite.scaley = 4096;
	for (i = 0; i < MAX_SPRITES; ++i) {
		gs_sprites[i].attribute = (1<<25);
		gs_sprites[i].attribute = (1<<6);
		gs_sprites[i].scalex = 4096;
		gs_sprites[i].scaley = 4096;
	}

	oth[0].length = 10;
	oth[1].length = 10;
	oth[0].org = otu[0];
	oth[1].org = otu[1];
	GsClearOt(0, 0, &oth[0]);
	GsClearOt(0, 0, &oth[1]);
	current_oth = &oth[GsGetActiveBuff()];

	InitHeap3((void*)0x8003A000, ((0x801E9CE6 - 0x8003A000) / 8u) * 8u);
	SpuInit();
	PadInit(0);

	sys_paddata = 0;
	bkg_loaded = false;

	reset_timers();
	VSyncCallback(vsync_callback);
	SetDispMask(1);
	update_display(DISPFLAG_DRAWSYNC|DISPFLAG_SWAPBUFFERS|DISPFLAG_VSYNC);
}

void update_display(const DispFlag flags)
{
	int buffer_id;
	if (flags&DISPFLAG_SWAPBUFFERS) {
		// finish frame
		if (flags&DISPFLAG_DRAWSYNC)
			DrawSync(0);
		if (flags&DISPFLAG_VSYNC)
			VSync(0);

		GsSwapDispBuff();

		if (bkg_loaded) {
			//GsSortFastSprite(&bkg_sprite, current_oth, OTENTRY_BKG);
		} else {
			GsSortClear(50, 50, 128, current_oth);
		}

		GsDrawOt(current_oth);

		// begin new frame
		buffer_id = GsGetActiveBuff();
		current_oth = &oth[buffer_id];
		GsSetWorkBase(gpu_pckt_area[buffer_id]);
		GsClearOt(0, 0, current_oth);
	} else if (flags&DISPFLAG_VSYNC) {
		VSync(0);
	}
}

void load_bkg_image(const char* const cdpath)
{
	RECT rect;
	void* p = NULL;

	load_files(&cdpath, &p, 1);

	rect = (RECT) {
		.x = SCREEN_WIDTH,
		.y = SCREEN_HEIGHT,
		.w = ((uint16_t*)p)[0],
		.h = ((uint16_t*)p)[1]
	};

	bkg_sprite.tpage = LoadTPage((void*)(((uint32_t*)p) + 1), 2, 0,
	                             rect.x, rect.y, rect.w, rect.h);
	bkg_sprite.w = rect.w;
	bkg_sprite.h = rect.h;
	bkg_loaded = true;

	DrawSync(0);
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
		GsSortFastSprite(&gs_sprites[i], current_oth, OTENTRY_SPRITE);
	}
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



