#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "system.h"

/* system */
bool sys_quit_flag = false;

/* input */
uint16_t sys_paddata;

/* timers */
uint32_t sys_msec_timer;
static uint32_t last_ticks;


/* video */
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


static void poll_events(void)
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		if (ev.type == SDL_QUIT) {
			sys_quit_flag = true;
			return;
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
                        const bool magic_pink)
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
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK) != 0)
		FATALERROR("%s", SDL_GetError());

	SDL_ShowCursor(SDL_DISABLE);

	/* graphics */
	window = SDL_CreateWindow("PSCHIP8",
	                          SDL_WINDOWPOS_UNDEFINED,
	                          SDL_WINDOWPOS_UNDEFINED,
	                          0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);

	if (window == NULL)
		FATALERROR("%s", SDL_GetError()); 

	renderer = SDL_CreateRenderer(window, -1,
	                              SDL_RENDERER_ACCELERATED|
				      SDL_RENDERER_PRESENTVSYNC);

	if (renderer == NULL)
		FATALERROR("%s", SDL_GetError());

	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	/* input */
	sys_paddata = 0;

	/* audio */
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0)
		FATALERROR("%s", Mix_GetError());

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
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_CloseAudio();
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

	poll_events();
	update_timers();
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
                  pixels, size->x, size->y,
                  16, size->x * sizeof(uint16_t),
	          SDL_PIXELFORMAT_ARGB1555);

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

const struct game_list* open_game_list(void)
{
	DIR* const dir = opendir("data/");
	if (dir == NULL) {
		LOGERROR("Couldn't opendir data/");
		return NULL;
	}

	uint8_t size = 0;
	uint8_t bufsize = 32;
	char** files = MALLOC(sizeof(char*) * bufsize);
	if (files == NULL) {
		LOGERROR("Couldn't allocate mem");
		return NULL;
	}

	struct dirent* ent;
	while ((ent = readdir(dir)) != NULL) {
		const int len = strlen(ent->d_name);
		if (strcmp(&ent->d_name[len - 4], ".CH8") == 0) {
			if (size > bufsize) {
				bufsize += 32;
				files = REALLOC(files, sizeof(char*) * bufsize);
			}
			files[size] = MALLOC(len + 1);
			strcpy(files[size], ent->d_name);
			size += 1;
		}
	}

	closedir(dir);
	struct game_list* gamelist = MALLOC(sizeof(struct game_list));
	gamelist->files = (const char* const*)files;
	gamelist->size = size;
	return gamelist;
}

void close_game_list(const struct game_list* const gamelist)
{
	for (int i = 0; i < gamelist->size; ++i)
		FREE((char*)gamelist->files[i]);
	FREE((char**)gamelist->files);
	FREE((struct game_list*)gamelist);
}

void sys_logaux(const char* const cat, const char* const fmt, va_list ap)
{
	printf("%s: ", cat);
	vprintf(fmt, ap);
	putchar('\n');
}	

void sys_log(const char* const cat, const char* const fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	sys_logaux(cat, fmt, ap);
	va_end(ap);
}

void sys_fatalerror(const char* const fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	sys_logaux("[FATAL ERROR]", fmt, ap);
	va_end(ap);
	term_system();
	exit(EXIT_FAILURE);
}

