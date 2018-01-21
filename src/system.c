#include <stdlib.h>
#include <stdarg.h>
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

#define OT_LENGTH            (4)
#define MAX_PACKETS          (4096)
#define OTENTRY_FONT         (0)
#define OTENTRY_SPRITE       (1)
#define OTENTRY_BKG          (2)
#define SPRTSHEET_FB_X       (SCREEN_WIDTH)
#define SPRTSHEET_FB_Y       (0)
#define FONT_FB_X            (SCREEN_WIDTH + 256)
#define FONT_FB_Y            (0)
#define BKG_FB_X             (SCREEN_WIDTH)
#define BKG_FB_Y             (256)
#define TMPBUFF_FB_X         (SCREEN_WIDTH * 2)
#define TMPBUFF_FB_Y         (256)


u_long _ramsize   = 0x00200000; // force 2 megabytes of RAM
u_long _stacksize = 0x00004000; // force 16 kilobytes of stack


uint16_t sys_paddata;
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;


static uint32_t nsec_timer;
static uint16_t rcnt1_last;

static GsOT oth[2];
static GsOT_TAG otu[2][1<<OT_LENGTH];
static PACKET gpu_pckt_buff[2][MAX_PACKETS];
static GsOT* curr_drawot = NULL;
static GsSPRITE* ss_sprites = NULL;  // sprite sheet sprites
static GsSPRITE* char_sprites = NULL;// font characteres sprites
static short ss_sprites_size = 0;
static short char_sprites_size = 0;
static GsSPRITE bkg_sprites[2];
static bool bkg_loaded = false;

static inline void update_pads(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = PadRead(0);
}

static void setup_curr_drawot(void)
{
	const int idx = GsGetActiveBuff();
	curr_drawot = &oth[idx];
	GsSetWorkBase(gpu_pckt_buff[idx]);
	GsClearOt(0, 0, curr_drawot);
}

static void vsync_callback(void)
{
	update_timers();
	update_pads();
}


void init_system(void)
{
	ResetCallback();
	SetDispMask(1);

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
	GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT);
	GsClearDispArea(0, 0, 0);

	oth[0].length = OT_LENGTH;
	oth[1].length = OT_LENGTH;
	oth[0].org = otu[0];
	oth[1].org = otu[1];
	setup_curr_drawot();

	InitHeap3((void*)0x801F8000, 0x100000);
	SpuInit();

	PadInit(0);
	sys_paddata = 0;

	reset_timers();
	VSyncCallback(vsync_callback);
	update_display(true);
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

void update_timers(void)
{
	const uint16_t rcnt1 = GetRCnt(RCntCNT1);

	nsec_timer += ((uint16_t)(rcnt1 - rcnt1_last)) * NSEC_PER_HSYNC;
	
	sys_usec_timer += nsec_timer / 1000u;
	sys_msec_timer += nsec_timer / 1000000u;

	nsec_timer -= (nsec_timer / 1000u) * 1000u;
	rcnt1_last = rcnt1;
}

void update_display(const bool vsync)
{
	// finish last drawing
	DrawSync(0);
	if (vsync)
		VSync(0);

	// display finished draw / start new drawing
	GsSwapDispBuff();
	if (bkg_loaded) {
		GsSortFastSprite(&bkg_sprites[0], curr_drawot, OTENTRY_BKG);
		GsSortFastSprite(&bkg_sprites[1], curr_drawot, OTENTRY_BKG);
	} else {
		GsSortClear(50, 50, 128, curr_drawot);
	}
	GsDrawOt(curr_drawot);
	setup_curr_drawot();
}

void font_print(short scrx, short scry, const char* const fmt, ...)
{
	static char fnt_buffer[512];
	static GsSPRITE sort_sprites[512];
	static short sort_sprites_idx = 0;

	short i, x, y;
	const char* in;
	char* out;

	va_list args;

	va_start(args, fmt);

	in = fmt;
	out = fnt_buffer;
	while (*in != '\0') {
		switch (*in) {
		default:
			*out++ = *in++;
			break;
		case '%':
			++in;
			switch (*in) {
			default:
				*out++ = '%';
				break;
			case 'd':
				++in;
				out += sprintf(out, "%d", va_arg(args, int));
				break;
			case 's':
				++in;
				out += sprintf(out, "%s", va_arg(args, char*));
				break;
			}
			break;
		}
	}

	*out = '\0';
	va_end(args);

	x = scrx;
	y = scry;
	for (i = 0; fnt_buffer[i] != '\0'; ++i) {

		if (fnt_buffer[i] == '\n') {
			y += char_sprites[0].h;
			x = scrx;
			continue;
		}

		if (sort_sprites_idx >= (sizeof(sort_sprites)/sizeof(sort_sprites[0])))
			sort_sprites_idx = 0;

		memcpy(&sort_sprites[sort_sprites_idx],
		       &char_sprites[fnt_buffer[i] - 32],
		       sizeof(GsSPRITE));

		sort_sprites[sort_sprites_idx].x = x;
		sort_sprites[sort_sprites_idx].y = y;

		GsSortFastSprite(&sort_sprites[sort_sprites_idx],
		                 curr_drawot, OTENTRY_FONT);

		x += sort_sprites[sort_sprites_idx].w;
		++sort_sprites_idx;
	}
}


void draw_sprites(const Sprite* const sprites, const short nsprites)
{
	short i;
	for (i = 0; i < nsprites; ++i) {
		ss_sprites[i].x = sprites[i].spos.x;
		ss_sprites[i].y = sprites[i].spos.y;
		ss_sprites[i].w = sprites[i].size.w;
		ss_sprites[i].h = sprites[i].size.h;
		ss_sprites[i].u = sprites[i].tpos.u;
		ss_sprites[i].v = sprites[i].tpos.v;
		GsSortFastSprite(&ss_sprites[i], curr_drawot, OTENTRY_SPRITE);
	}
}

void draw_ram_buffer(void* pixels,
                     const short screenx, const short screeny,
                     const short width, const short height,
                     const uint8_t scalex, const uint8_t scaley)
{
	static GsSPRITE sprt;
	memset(&sprt, 0, sizeof sprt);
	sprt.tpage = LoadTPage(pixels, 2, 0,
	                       TMPBUFF_FB_X, TMPBUFF_FB_Y, width, height);
	sprt.attribute = (1<<6)|(1<<25);
	sprt.x = screenx;
	sprt.y = screeny;
	sprt.w = width + 1;
	sprt.h = height + 1;
	sprt.mx = width / 2u;
	sprt.my = height / 2u;
	sprt.scalex = ONE * scalex;
	sprt.scaley = ONE * scaley;

	GsSortSprite(&sprt, curr_drawot, OTENTRY_SPRITE);
}

void load_sprite_sheet(const char* const cdpath, short maxsprites_on_screen)
{
	short i;
	int tpage;
	void* p = NULL;

	load_files(&cdpath, &p, 1);

	tpage = LoadTPage((void*)(((uint32_t*)p) + 1), 2, 0,
	                  SPRTSHEET_FB_X, SPRTSHEET_FB_Y,
	                  ((uint16_t*)p)[0], ((uint16_t*)p)[1]);

	if (ss_sprites != NULL)
		free3(ss_sprites);

	ss_sprites_size = maxsprites_on_screen;
	ss_sprites = malloc3(sizeof(GsSPRITE) * ss_sprites_size);

	memset(ss_sprites, 0, sizeof(GsSPRITE) * ss_sprites_size);
	for (i = 0; i < ss_sprites_size; ++i) {
		ss_sprites[i].attribute = (1<<6)|(1<<25)|(1<<27);
		ss_sprites[i].tpage = tpage;
	}

	DrawSync(0);
	free3(p);
}

void load_bkg(const char* const cdpath)
{
	RECT rect;
	void* p = NULL;

	load_files(&cdpath, &p, 1);

 	rect = (RECT) {
		.x = BKG_FB_X,
		.y = BKG_FB_Y,
		.w = ((uint16_t*)p)[0],
		.h = ((uint16_t*)p)[1]
	};

	LoadImage(&rect, (void*)(((uint32_t*)p) + 1));

	memset(&bkg_sprites, 0, sizeof(GsSPRITE) * 2);
	bkg_sprites[0].attribute = bkg_sprites[1].attribute = (1<<6)|(1<<25)|(1<<27);
	bkg_sprites[0].w = 256;
	bkg_sprites[0].h = rect.h;
	bkg_sprites[1].x = 256;
	bkg_sprites[1].w = rect.w - 256;
	bkg_sprites[1].h = rect.h;
	bkg_sprites[0].tpage = GetTPage(2, 0, BKG_FB_X, BKG_FB_Y);
	bkg_sprites[1].tpage = GetTPage(2, 0, BKG_FB_X + 256, BKG_FB_Y);
	bkg_loaded = true;

	DrawSync(0);
	free3(p);
}

void load_font(const char* const cdpath,
               const uint8_t char_w,
               const uint8_t char_h)
{
	void* p = NULL;
	int tpage;
	uint16_t tw, th;
	uint16_t i, j, s;

	load_files(&cdpath, &p, 1);
	tw = ((uint16_t*)p)[0];
	th = ((uint16_t*)p)[1];

	tpage = LoadTPage((void*)(((uint32_t*)p) + 1), 2, 0,
	                  FONT_FB_X, FONT_FB_Y, tw, th);

	if (char_sprites != NULL)
		free3(char_sprites);

	char_sprites_size = ((tw / char_w) * (th / char_h)) + 1;
	char_sprites = malloc3(sizeof(GsSPRITE) * char_sprites_size);
	memset(char_sprites, 0, sizeof(GsSPRITE) * char_sprites_size);
	for (i = s = 0; i < th; i += char_h) {
		for (j = 0; j < tw; j += char_w) {
			char_sprites[s++] = (GsSPRITE) {
				.attribute = (1<<6)|(1<<25),
				.tpage = tpage,
				.w = char_w,
				.h = char_h,
				.u = j,
				.v = i,
			};
		}
	}

	DrawSync(0);
	free3(p);
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

