#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libapi.h>
#include <libmath.h>
#include <libetc.h>
#include <libcd.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <libspu.h>
#include <libsnd.h>
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

#define SPU_MAX_MALLOC       (24)

u_long _ramsize   = 0x00200000; /* force 2 megabytes of RAM */
u_long _stacksize = 0x00004000; /* force 16 kilobytes of stack */

/* input */
uint16_t sys_paddata;

/* timers */
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;
static uint32_t nsec_timer;
static uint16_t rcnt1_last;

/* libgs */
static GsOT oth[2];
static GsOT_TAG otu[2][1<<OT_LENGTH];
static PACKET gpu_pckt_buff[2][MAX_PACKETS];
static GsOT* curr_drawot = NULL;

static GsSPRITE* ss_sprites = NULL;   /* used to sort sprite sheet sprites */
static int16_t ss_sprites_size = 0;
static GsSPRITE* char_sprites = NULL; /* used to sort font characteres sprites */
static GsSPRITE ram_buff_spr;
static int16_t char_sprites_size = 0;
static uint8_t char_ascii_idx = 0;
static struct vec2 char_csize;
static struct vec2 char_tsize;
static GsSPRITE bkg_sprites[2];
static bool bkg_loaded = false;

/* libspu */
static uint8_t spu_nsnd = 0;
static int32_t spu_snd_addrs[SPU_MAX_MALLOC];
static uint16_t spu_snd_freqs[SPU_MAX_MALLOC];
static uint8_t spu_malloc[SPU_MALLOC_RECSIZ * (SPU_MAX_MALLOC + 1)];


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

	/* heap memory */
	InitHeap3((void*)0x801F8000, 0x100000);


	/* graphics */
	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsNONINTER|GsOFSGPU, 1, 0);
	GsDefDispBuff(0, 0, 0, SCREEN_HEIGHT);
	GsClearDispArea(0, 0, 0);

	oth[0].length = OT_LENGTH;
	oth[1].length = OT_LENGTH;
	oth[0].org = otu[0];
	oth[1].org = otu[1];
	setup_curr_drawot();

	/* sound */
	{
		SpuCommonAttr cattr = {
			.mask = SPU_COMMON_MVOLL|SPU_COMMON_MVOLR,
			.mvol = { .left = 0x3FFF, .right = 0x3FFF }
		};
		SpuVoiceAttr vattr = {
			.mask   = (SPU_VOICE_VOLL|SPU_VOICE_VOLR|
			           SPU_VOICE_ADSR_AMODE|SPU_VOICE_ADSR_SMODE|
			           SPU_VOICE_ADSR_RMODE|SPU_VOICE_ADSR_AR|
			           SPU_VOICE_ADSR_DR|SPU_VOICE_ADSR_SR|
			           SPU_VOICE_ADSR_RR|SPU_VOICE_ADSR_SL),
			.voice  = SPU_ALLCH,
			.volume = { .left = 0x3FFF, .right = 0x3FFF },
			.a_mode = SPU_VOICE_LINEARIncN,
			.s_mode = SPU_VOICE_LINEARIncN,
			.r_mode = SPU_VOICE_LINEARDecN,
			.ar     = 0x00,
			.dr     = 0x00,
			.sr     = 0x00,
			.rr     = 0x00,
			.sl     = 0x0F
		};

		SpuInit();
		SpuInitMalloc(SPU_MAX_MALLOC, spu_malloc);
		SpuSetCommonAttr(&cattr);
		SpuSetTransferMode(SpuTransByDMA);
		SpuSetVoiceAttr(&vattr);
		memset(spu_snd_addrs, 0, sizeof spu_snd_addrs);
	}

	/* input */
	PadInit(0);
	sys_paddata = 0;

	/* timers */
	reset_timers();

	/* callbacks */
	VSyncCallback(vsync_callback);

	update_display();
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

void update_display(void)
{
	/* finish last drawing */
	DrawSync(0);
	VSync(0);

	/* display finished draw / start new drawing */
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


void font_print(const struct vec2* const pos,
                const char* const fmt,
                const void* const* varpack)
{
	static char fnt_buffer[512];
	static int16_t char_sprites_idx = 0;

	GsSPRITE* csprt;
	uint8_t char_idx;
	int16_t i, x, y;
	const char* in;
	char* out;

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
				out += sprintf(out, "%d", *((const int*)(*varpack)));
				++varpack;
				break;
			case 's':
				++in;
				out += sprintf(out, "%s", ((const char*)(*varpack)));
				++varpack;
				break;
			}
			break;
		}
	}

	*out = '\0';

	x = pos->x;
	y = pos->y;
	for (i = 0; fnt_buffer[i] != '\0'; ++i) {
		if (fnt_buffer[i] == ' ') {
			x += char_csize.x;
			continue;
		}
		
		if (fnt_buffer[i] == '\n' || x >= SCREEN_WIDTH) {
			y += char_csize.y;
			x = pos->x;
			if (fnt_buffer[i] == '\n')
				continue;
		}

		if (char_sprites_idx >= char_sprites_size)
			char_sprites_idx = 0;

		char_idx = fnt_buffer[i] - char_ascii_idx;
		csprt = &char_sprites[char_sprites_idx++];
		csprt->x = x;
		x += char_csize.x;
		csprt->y = y;
		csprt->u = (char_csize.x * char_idx) % char_tsize.x;
		csprt->v = char_csize.y * ((char_csize.x * char_idx) / char_tsize.x);
		GsSortFastSprite(csprt, curr_drawot, OTENTRY_FONT);
	}
}


void draw_sprites(const struct sprite* const sprites, const int16_t nsprites)
{
	int16_t i;
	for (i = 0; i < nsprites; ++i) {
		ss_sprites[i].x = sprites[i].spos.x;
		ss_sprites[i].y = sprites[i].spos.y;
		ss_sprites[i].w = sprites[i].size.x;
		ss_sprites[i].h = sprites[i].size.y;
		ss_sprites[i].u = sprites[i].tpos.x;
		ss_sprites[i].v = sprites[i].tpos.y;
		GsSortFastSprite(&ss_sprites[i], curr_drawot, OTENTRY_SPRITE);
	}
}

void draw_ram_buffer(void)
{
	GsSortSprite(&ram_buff_spr, curr_drawot, OTENTRY_SPRITE);
}

void assign_snd_chan(const uint8_t chan, const uint8_t snd_index)
{
	const uint16_t freq = spu_snd_freqs[snd_index];
	SpuSetVoiceStartAddr(chan, spu_snd_addrs[snd_index]);
	SpuSetVoicePitch(chan, (freq == 44100) ? 0x1000 : 0x1000 / 2);
}

void load_sprite_sheet(void* const data, const int16_t max_sprites_on_screen)
{
	int16_t i;
	int tpage;

	tpage = LoadTPage((void*)(((uint32_t*)data) + 1), 2, 0,
	                  SPRTSHEET_FB_X, SPRTSHEET_FB_Y,
	                  ((uint16_t*)data)[0], ((uint16_t*)data)[1]);

	ss_sprites_size = max_sprites_on_screen;
	ss_sprites = REALLOC(ss_sprites, sizeof(GsSPRITE) * ss_sprites_size);

	memset(ss_sprites, 0, sizeof(GsSPRITE) * ss_sprites_size);
	for (i = 0; i < ss_sprites_size; ++i) {
		ss_sprites[i].attribute = (1<<6)|(1<<25)|(1<<27);
		ss_sprites[i].tpage = tpage;
	}
}

void load_bkg(void* const data)
{
	RECT rect;

 	rect = (RECT) {
		.x = BKG_FB_X,
		.y = BKG_FB_Y,
		.w = ((uint16_t*)data)[0],
		.h = ((uint16_t*)data)[1]
	};

	LoadImage(&rect, (void*)(((uint32_t*)data) + 1));

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
}

void load_font(void* const data,
               const struct vec2* const charsize,
               const uint8_t ascii_idx,
               const int16_t max_chars_on_scr)
{
	int16_t i;
	int tpage;

	char_tsize = (struct vec2) {
		.x = ((uint16_t*)data)[0],
		.y = ((uint16_t*)data)[1]
	};

	memcpy(&char_csize, charsize, sizeof(struct vec2));

	tpage = LoadTPage((void*)(((uint32_t*)data) + 1), 2, 0,
	                  FONT_FB_X, FONT_FB_Y, char_tsize.x, char_tsize.y);

	char_ascii_idx = ascii_idx;
	char_sprites_size = max_chars_on_scr;
	char_sprites = REALLOC(char_sprites, sizeof(GsSPRITE) * max_chars_on_scr);
	memset(char_sprites, 0, sizeof(GsSPRITE) * max_chars_on_scr);
	for (i = 0; i < max_chars_on_scr; ++i) {
		char_sprites[i] = (GsSPRITE) {
			.attribute = (1<<6)|(1<<25)|(1<<27),
			.tpage = tpage,
			.w = char_csize.x,
			.h = char_csize.y
		};
	}
}

void load_snd(void* const* const data, const uint8_t nsnd)
{
	uint8_t i;
	for (i = 0; i < spu_nsnd; ++i)
		SpuFree(spu_snd_addrs[i]);

	for (i = 0; i < nsnd; ++i) {
		uint8_t* const p = data[i];
		const uint32_t size = (p[12]<<24)|(p[13]<<16)|(p[14]<<8)|p[15];

		spu_snd_freqs[i] = (p[18]<<8)|p[19];
		spu_snd_addrs[i] = SpuMalloc(size);
		/* transfer to spu */
		SpuIsTransferCompleted(SPU_TRANSFER_WAIT);
		SpuSetTransferStartAddr(spu_snd_addrs[i]);
		SpuWrite(p + SPU_ST_VAG_HEADER_SIZE, size);
	}

	spu_nsnd = nsnd;
}

void load_ram_buffer(void* const pixels,
                     const struct vec2* const pos,
                     const struct vec2* const size,
                     const uint8_t scale)
{
	memset(&ram_buff_spr, 0, sizeof ram_buff_spr);

	ram_buff_spr.tpage = LoadTPage(pixels, 2, 0,
	                       TMPBUFF_FB_X, TMPBUFF_FB_Y, size->x, size->y);
	ram_buff_spr.attribute = (1<<6)|(1<<25);
	ram_buff_spr.x = pos->x;
	ram_buff_spr.y = pos->y;
	ram_buff_spr.w = size->x;
	ram_buff_spr.h = size->y;
	ram_buff_spr.mx = size->x / 2u;
	ram_buff_spr.my = size->y / 2u;
	ram_buff_spr.scalex = ONE * scale;
	ram_buff_spr.scaley = ONE * scale;
}

void load_files(const char* const* const filenames, 
                void** const dsts, 
                const int16_t nfiles)
{
	CdlFILE fp;
	short i, j, cnt;
	bool need_alloc;
	char namebuff[24];

	CdInit();

	for (i = 0; i < nfiles; ++i) {
		sprintf(namebuff, "\\%s;1", filenames[i]);
		LOGINFO("LOADING %s...", namebuff);

		for (j = 0; j < 10; ++j) {
			if (CdSearchFile(&fp, namebuff) != 0)
				break;
		}

		if (j == 10)
			FATALERROR("Couldn't find file %s in CDROM", namebuff);
		
		LOGINFO("Found file %s with size: %lu", namebuff, fp.size);

		need_alloc = dsts[i] == NULL;
		if (need_alloc) {
			dsts[i] = MALLOC(2048 + fp.size);
			if (dsts[i] == NULL)
				FATALERROR("Couldn't allocate memory");
		}

		for (j = 0; j < 10; ++j) {
			CdReadFile(namebuff, (void*)dsts[i], fp.size);

			while ((cnt = CdReadSync(1, NULL)) > 0)
					VSync(0);
				
			if (cnt == 0)
				break;
		}

		if (j == 10)
			FATALERROR("Couldn't read file %s from CDROM", namebuff);
	}

	CdStop();
}

void free_files(void* const* const pointers, const int16_t nfiles)
{
	int16_t i;
	for (i = 0; i < nfiles; ++i)
		FREE(pointers[i]);
}

