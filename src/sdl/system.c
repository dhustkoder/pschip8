#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>
#include "system.h"


/* input */
uint16_t sys_paddata;

/* timers */
uint32_t sys_msec_timer;
static uint32_t last_ticks;


/* video libsdl */
static SDL_Surface* screen_surf;
static SDL_Surface* bkg_surf = NULL;
static SDL_Surface* font_surf = NULL;
static SDL_Surface* sprite_sheet_surf = NULL;
static struct vec2 char_csize;
static struct vec2 char_tsize;
static uint8_t char_ascii_index;

/* input */
#if defined(PLATFORM_PS2)
static SDL_Joystick* joy;
#endif


static void update_paddata(void)
{
	#if defined(PLATFORM_PS2)

	const int nbutns = SDL_JoystickNumButtons(joy);
	uint8_t bit = 0;

	SDL_JoystickUpdate();
	for (int i = 0; i < nbutns; ++i, ++bit) {
		if (SDL_JoystickGetButton(joy, i))
			sys_paddata |= (1<<bit);
		else
			sys_paddata &= ~(1<<bit);
	}

	const uint8_t dirs[] = { SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT };
	const uint8_t hat = SDL_JoystickGetHat(joy, 0);
	for (int i = 0; i < sizeof(dirs)/sizeof(dirs[0]); ++i, ++bit) {
		if (dirs[i]&hat)
			sys_paddata |= (1<<bit);
		else
			sys_paddata &= ~(1<<bit);
	}

	#elif defined(PLATFORM_LINUX)

	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		
		if (ev.type == SDL_QUIT) {
			term_system();
			exit(0);
		}

		if (ev.type != SDL_KEYDOWN && ev.type != SDL_KEYUP)
			continue;

		button_t button;
		switch (ev.key.keysym.sym) {
		case SDLK_UP: button = BUTTON_UP; break;
		case SDLK_DOWN: button = BUTTON_DOWN; break;
		case SDLK_LEFT: button = BUTTON_LEFT; break;
		case SDLK_RIGHT: button = BUTTON_RIGHT; break;
		case SDLK_v: button = BUTTON_START; break;
		case SDLK_c: button = BUTTON_SELECT; break;
		case SDLK_a: button = BUTTON_TRIANGLE; break;
		case SDLK_s: button = BUTTON_CIRCLE; break;
		case SDLK_x: button = BUTTON_CROSS; break;
		case SDLK_z: button = BUTTON_SQUARE; break;
		default: button = 0; break;
		}

		if (ev.type == SDL_KEYDOWN)
			sys_paddata |= button;
		else
			sys_paddata &= ~button;
	}

	#endif
}

static void set_bmp_surf(const void* const data, SDL_Surface** const surfp)
{
	const uint32_t size = *((uint32_t*)(((uint8_t*)data) + 0x02));

	if (*surfp != NULL)
		SDL_FreeSurface(*surfp);

	*surfp = SDL_LoadBMP_RW(SDL_RWFromConstMem(data, size), 1);
}


void init_system(void)
{
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK);
	SDL_ShowCursor(SDL_DISABLE);

	/* graphics */
	screen_surf = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT,
		                       32, SDL_DOUBLEBUF|SDL_HWSURFACE);
	/* input */
	#ifdef PLATFORM_PS2
	joy = SDL_JoystickOpen(0);
	#endif
	sys_paddata = 0;

	/* timers */
	reset_timers();
	update_display();
}

#if defined(PLATFORM_LINUX)
void term_system(void)
{
	SDL_FreeSurface(bkg_surf);
	SDL_FreeSurface(font_surf);
	SDL_FreeSurface(sprite_sheet_surf);
	SDL_FreeSurface(screen_surf);
	SDL_Quit();
}
#endif

void reset_timers(void)
{
	sys_msec_timer = 0;
	last_ticks = SDL_GetTicks();
}

void update_timers(void)
{
	const uint32_t ticks = SDL_GetTicks();
	sys_msec_timer += ticks - last_ticks;
	last_ticks = ticks;
}

void update_display(void)
{
	SDL_Flip(screen_surf);

	if (bkg_surf != NULL) {
		SDL_SoftStretch(bkg_surf, NULL, screen_surf, NULL);
	} else {
		SDL_FillRect(screen_surf, NULL,
		             SDL_MapRGB(screen_surf->format, 0x66, 0x66, 0xFF));
	}

	update_timers();
	update_paddata();

	#ifdef PLATFORM_LINUX
	/* vsync */
	static uint32_t lastticks = 0;
	const uint32_t ticks = SDL_GetTicks();
	if ((ticks - lastticks) < 16u)
		SDL_Delay(16u - (ticks - lastticks));
	lastticks = SDL_GetTicks();
	#endif
}

void font_print(const struct vec2* const pos,
                const char* const fmt,
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
			dst.x = pos->x;
			continue;
		}

		const uint8_t ch_idx = ch - char_ascii_index;
		src.x = (char_csize.x * ch_idx) % char_tsize.x;
		src.y = char_csize.y * ((char_csize.x * ch_idx) / char_tsize.x);

		SDL_BlitSurface(font_surf, &src, screen_surf, &dst);

		dst.x += dst.w;
	}
}

void draw_sprites(const struct sprite* const sprites, const short nsprites)
{
	for (short i = 0; i < nsprites; ++i) {
		SDL_Rect src = {
			.x = sprites[i].tpos.x,
			.y = sprites[i].tpos.y,
			.w = sprites[i].size.x,
			.h = sprites[i].size.y
		};
		SDL_Rect dst = {
			.x = sprites[i].spos.x,
			.y = sprites[i].spos.y,
			.w = sprites[i].size.x,
			.h = sprites[i].size.y
		};
		SDL_BlitSurface(sprite_sheet_surf, &src, screen_surf, &dst);
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
	          0x000000FF, 0x0000FF00, 0x00FF0000, 0x00);

	SDL_Rect dst = {
		pos->x - ((size->x * scale) / 2),
		pos->y - ((size->y * scale) / 2),
		size->x  * scale, size->y * scale
	};

	SDL_Surface* surf2 = SDL_DisplayFormat(surf);
	SDL_SoftStretch(surf2, NULL, screen_surf, &dst);
	SDL_FreeSurface(surf);
	SDL_FreeSurface(surf2);
}

void load_font(const void* const data, const struct vec2* const charsize,
               const uint8_t ascii_idx, const short max_chars_on_scr)
{
	set_bmp_surf(data, &font_surf);
	SDL_SetColorKey(font_surf, SDL_SRCCOLORKEY,
	                font_surf->format->Rmask|
	                font_surf->format->Bmask);

	const uint32_t w = *((uint32_t*)(((uint8_t*)data) + 0x12));
	const uint32_t h = *((uint32_t*)(((uint8_t*)data) + 0x16));

	char_tsize.x = w;
	char_tsize.y = h;
	char_csize.x = charsize->x;
	char_csize.y = charsize->y;
	char_ascii_index = ascii_idx;
}

void load_bkg(const void* const data)
{
	set_bmp_surf(data, &bkg_surf);
	SDL_Surface* const tmp = bkg_surf;
	bkg_surf = SDL_DisplayFormat(bkg_surf);
	SDL_FreeSurface(tmp);
}

void load_sprite_sheet(const void* const data, const short max_sprites_on_screen)
{
	set_bmp_surf(data, &sprite_sheet_surf);
	SDL_SetColorKey(sprite_sheet_surf, SDL_SRCCOLORKEY,
	                sprite_sheet_surf->format->Rmask|
	                sprite_sheet_surf->format->Bmask);
}

void load_files(const char* const* const filenames,
                void** const dsts, const short nfiles)
{
	char namebuffer[48];

	for  (short i = 0; i < nfiles; ++i) {
		sprintf(namebuffer, DATA_PATH_PREFIX "%s" DATA_PATH_POSTFIX,
		        filenames[i]);
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

