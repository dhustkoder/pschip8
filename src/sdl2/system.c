#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "system.h"


/* input */
uint16_t sys_paddata;

/* timers */
uint32_t sys_msec_timer;
static uint32_t last_ticks;


/* video libsdl */
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* bkg_tex = NULL;
static SDL_Texture* font_tex = NULL;
static SDL_Texture* sprite_sheet_tex = NULL;
static SDL_Texture* ram_buffer_tex = NULL;
static SDL_Rect ram_buffer_rect;
static struct vec2 char_csize;
static struct vec2 char_tsize;
static uint8_t char_ascii_index;


/* audio */
static Mix_Chunk** snds_chunks = NULL;
static uint8_t* snds_chans = NULL;
static short nsnds_chunks;


static void update_paddata(void)
{
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
}

static void set_bmp_tex(const void* const data,
                        SDL_Texture** const texp,
                        bool magic_pink)
{
	const uint32_t size = *((uint32_t*)(((uint8_t*)data) + 0x02));

	if (*texp != NULL)
		SDL_DestroyTexture(*texp);

	SDL_Surface* const surf = SDL_LoadBMP_RW(SDL_RWFromConstMem(data, size), 1);

	if (magic_pink) {
		SDL_SetColorKey(surf, SDL_TRUE,
				surf->format->Rmask|surf->format->Bmask);
	}

	*texp = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_FreeSurface(surf);
}


void init_system(void)
{
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK);
	SDL_ShowCursor(SDL_DISABLE);

	/* graphics */
	window = SDL_CreateWindow("PSCHIP8",
	                          SDL_WINDOWPOS_UNDEFINED,
	                          SDL_WINDOWPOS_UNDEFINED,
	                          0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	/* input */
	sys_paddata = 0;

	/* audio */
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);


	/* timers */
	reset_timers();
	update_display();
}

void term_system(void)
{
	if (snds_chunks != NULL) {
		for (int i = 0; i < nsnds_chunks; ++i)
			Mix_FreeChunk(snds_chunks[i]);
		free(snds_chunks);
		free(snds_chans);
	}

	SDL_DestroyTexture(bkg_tex);
	SDL_DestroyTexture(font_tex);
	SDL_DestroyTexture(sprite_sheet_tex);
	Mix_Quit();
	SDL_Quit();
}

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
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);

	if (bkg_tex != NULL)
		SDL_RenderCopy(renderer, bkg_tex, NULL, NULL);

	update_timers();
	update_paddata();
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

		SDL_RenderCopy(renderer, font_tex, &src, &dst);

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
		SDL_RenderCopy(renderer, sprite_sheet_tex, &src, &dst);
	}
}

void draw_ram_buffer(void)
{
	SDL_RenderCopy(renderer, ram_buffer_tex, NULL, &ram_buffer_rect);
}

void assign_snd_chan(const uint8_t chan, const uint8_t snd_index)
{
	snds_chans[chan] = snd_index;
}

void enable_chan(const uint8_t chan)
{
	Mix_PlayChannel(chan, snds_chunks[snds_chans[chan]], 0);
}

void load_sprite_sheet(const void* const data, const short max_sprites_on_screen)
{
	set_bmp_tex(data, &sprite_sheet_tex, true);
}

void load_bkg(const void* const data)
{
	set_bmp_tex(data, &bkg_tex, false);
}

void load_font(const void* const data, const struct vec2* const charsize,
               const uint8_t ascii_idx, const short max_chars_on_scr)
{
	set_bmp_tex(data, &font_tex, true);

	const uint32_t w = *((uint32_t*)(((uint8_t*)data) + 0x12));
	const uint32_t h = *((uint32_t*)(((uint8_t*)data) + 0x16));

	char_tsize.x = w;
	char_tsize.y = h;
	char_csize.x = charsize->x;
	char_csize.y = charsize->y;
	char_ascii_index = ascii_idx;
}

void load_snd(void* const* const snds, const short nsnds)
{
	if (snds_chunks != NULL) {
		for (int i = 0; i < nsnds_chunks; ++i)
			Mix_FreeChunk(snds_chunks[i]);
		free(snds_chunks);
		free(snds_chans);
	}

	nsnds_chunks = nsnds;
	snds_chunks = malloc(sizeof(Mix_Chunk*) * nsnds_chunks);
	snds_chans = malloc(sizeof(uint8_t) * nsnds_chunks);

	for (int i = 0; i < nsnds; ++i)
		snds_chunks[i] = Mix_QuickLoad_WAV(snds[i]);
}

void load_ram_buffer(void* const pixels,
                     const struct vec2* const pos,
                     const struct vec2* const size,
                     const uint8_t scale)
{
	SDL_Surface* const surf =
		SDL_CreateRGBSurfaceWithFormatFrom(
                  pixels,
	          size->x, size->y, 16, size->x * sizeof(uint16_t),
	          SDL_PIXELFORMAT_ARGB1555);

	if (surf == NULL) {
		FATALERROR("%s", SDL_GetError());
	}

	ram_buffer_rect = (SDL_Rect) {
		.x = pos->x - ((size->x * scale) / 2),
		.y = pos->y - ((size->y * scale) / 2),
		.w = size->x * scale,
		.h = size->y * scale
	};

	if (ram_buffer_tex != NULL)
		SDL_DestroyTexture(ram_buffer_tex);

	ram_buffer_tex = SDL_CreateTextureFromSurface(renderer, surf);

	SDL_FreeSurface(surf);
}

void load_files(const char* const* const filenames,
                void** const dsts, const short nfiles)
{
	char namebuffer[24];
	for  (short i = 0; i < nfiles; ++i) {
		sprintf(namebuffer,"data/%s", filenames[i]);
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

