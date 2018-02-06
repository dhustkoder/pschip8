#include <stdio.h>
#include <string.h>
#include <timer.h>
#include <kernel.h>
#include <sifrpc.h>
#include <SDL.h>
#include "system.h"


/* input */
uint16_t sys_paddata;

/* timers */
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;
static uint32_t cputicks;

/* video libsdl */
static SDL_Surface* screen_surf;
static SDL_Surface* bkg_surf = NULL;
static SDL_Surface* font_surf = NULL;
static struct vec2 char_csize;
static struct vec2 char_tsize;
static uint8_t char_ascii_index;


static void update_paddata(void)
{
	static SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_JOYBUTTONDOWN:
			sys_paddata |= 1<<ev.jbutton.button;
			break;
		case SDL_JOYBUTTONUP:
			sys_paddata &= ~(1<<ev.jbutton.button);
			break;
		}
	}
}


void init_system(void)
{
	SifInitRpc(0);
	SDL_Init(SDL_INIT_VIDEO);
	SDL_ShowCursor(SDL_DISABLE);

	/* graphics */
	screen_surf = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT,
	                               32, SDL_DOUBLEBUF|SDL_HWSURFACE|SDL_NOFRAME);
	/* input */
	sys_paddata = 0;

	/* timers */
	reset_timers();
	update_display(true);
}

void reset_timers(void)
{
	sys_msec_timer = 0;
	sys_usec_timer = 0;
	cputicks = cpu_ticks();
}

void update_timers(void)
{
	const uint32_t now = cpu_ticks();
	sys_usec_timer += (now - cputicks) / 299u;
	sys_msec_timer = sys_usec_timer / 1000u;
	cputicks = now;
}

void update_display(const bool vsync)
{
	static uint32_t msec = 0;
	static int fps = 0;

	SDL_Flip(screen_surf);
	update_timers();
	update_paddata();

	if (bkg_surf != NULL) {
		SDL_SoftStretch(bkg_surf, NULL, screen_surf, NULL);
	} else {
		SDL_FillRect(screen_surf, NULL,
		             SDL_MapRGB(screen_surf->format, 0x66, 0x66, 0xFF));
	}

	++fps;
	if ((sys_msec_timer - msec) >= 1000) {
		LOGINFO("FPS: %d", fps);
		fps = 0;
		msec = sys_msec_timer;
	}
}

void font_print(const struct vec2* const pos,
                const char* fmt,
                const void* const* varpack)
{
	static char buffer[512];
	
	const char* in = fmt;
	char* out = buffer;
	bool vp = false;
	while (*in != '\0') {
		if (!vp) {
			switch (*in) {
			default: *out++ = *in++; break;
			case '%': ++in; vp = true; break;
			}
		} else {
			switch (*in) {
			case 'd':
				++in;
				out += sprintf(out, "%d", *((int*)*varpack++));
				break;
			case 's':
				++in;
				out += sprintf(out, "%s", ((char*)*varpack++));
				break;
			}
			vp = false;
		}
	}
	*out = '\0';

	SDL_Rect src = { 0, 0, char_csize.x, char_csize.y };
	SDL_Rect dst = { pos->x, pos->y, char_csize.x, char_csize.y };
	for (int i = 0; buffer[i] != '\0'; ++i) {
		const char ch = buffer[i];
		if (ch == ' ') {
			dst.x += dst.w;
			continue;
		} else if (ch == '\n') {
			dst.y += dst.h;
			continue;
		}

		const uint8_t ch_idx = ch - char_ascii_index;
		src.x = (char_csize.x * ch_idx) % char_tsize.x;
		src.y = char_csize.y * ((char_csize.x * ch_idx) / char_tsize.x);

		SDL_BlitSurface(font_surf, &src, screen_surf, &dst);

		dst.x += dst.w;
	}
}

void draw_ram_buffer(void* const pixels,
                     const struct vec2* const pos,
                     const struct vec2* const size,
                     const uint8_t scale)
{
	SDL_Surface* const surf =
		SDL_CreateRGBSurfaceFrom(pixels,
	          size->x, size->y, 32, size->x * sizeof(uint32_t),
	          0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000);

	SDL_Rect dst = {
		pos->x - ((size->x * scale) / 2),
		pos->y - ((size->y * scale) / 2),
		size->x  * scale, size->y * scale
	};

	SDL_SoftStretch(surf, NULL, screen_surf, &dst);
	SDL_FreeSurface(surf);
}

void load_font(const void* const data, const struct vec2* const charsize,
               const uint8_t ascii_idx, const short max_chars_on_scr)
{
	const uint32_t size = *((uint32_t*)(((uint8_t*)data) + 0x02));
	const uint32_t w = *((uint32_t*)(((uint8_t*)data) + 0x12));
	const uint32_t h = *((uint32_t*)(((uint8_t*)data) + 0x16));

	if (font_surf != NULL)
		SDL_FreeSurface(font_surf);

	font_surf = SDL_LoadBMP_RW(SDL_RWFromConstMem(data, size), 1);

	SDL_SetColorKey(font_surf, SDL_SRCCOLORKEY,
	                font_surf->format->Rmask|font_surf->format->Bmask);

	char_tsize.x = w;
	char_tsize.y = h;
	char_csize.x = charsize->x;
	char_csize.y = charsize->y;
	char_ascii_index = ascii_idx;
}

void load_bkg(const void* const data)
{
	const uint32_t size = *((uint32_t*)(((uint8_t*)data) + 0x02));
	//const uint32_t w = *((uint32_t*)(((uint8_t*)data) + 0x12));
	//const uint32_t h = *((uint32_t*)(((uint8_t*)data) + 0x16));

	SDL_Surface* const surf = SDL_LoadBMP_RW(SDL_RWFromConstMem(data, size), 1);

	if (bkg_surf != NULL)
		SDL_FreeSurface(bkg_surf);

	bkg_surf = SDL_DisplayFormat(surf);

	SDL_FreeSurface(surf);
}

void load_files(const char* const* const filenames,
                void** const dsts, const short nfiles)
{
	char namebuffer[32];

	for  (short i = 0; i < nfiles; ++i) {
		sprintf(namebuffer, "cdrom0:\\%s;1", filenames[i]);
		FILE* const file = fopen(namebuffer, "r");

		if (file == NULL)
			FATALERROR("Couldn't open file %s", namebuffer);

		fseek(file, 0, SEEK_END);
		const long size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (dsts[i] == NULL) {
			dsts[i] = MALLOC(size);
			if (dsts[i] == NULL)
				FATALERROR("Couldn't allocate memory!");
		}

		fread(dsts[i], 1, size, file);
	}
}

