#include <timer.h>
#include <kernel.h>
#include <sifrpc.h>
#include <SDL.h>
#include "system.h"



/* timers */
uint32_t sys_msec_timer;
uint32_t sys_usec_timer;
static uint32_t cputicks;

/* video libsdl */
static SDL_Surface* surface;


void init_system(void)
{
	SifInitRpc(0);
	SDL_Init(SDL_INIT_VIDEO);

	/* graphics */
	surface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_DOUBLEBUF);
	SDL_FillRect(surface, &(SDL_Rect){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
	             SDL_MapRGB(surface->format, 0x66, 0x66, 0xFF));
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

	SDL_Flip(surface);
	update_timers();

	++fps;
	if ((sys_msec_timer - msec) >= 1000) {
		LOGINFO("FPS: %d", fps);
		fps = 0;
		msec = sys_msec_timer;
	}
}

void font_print(const struct vec2* pos, const char* fmt, const void* const* varpack)
{

}

void draw_ram_buffer(void* const pixels,
                     const struct vec2* const pos,
                     const struct vec2* const size,
                     const uint8_t scale)
{

}

void load_font(const void* const data, const struct vec2* const charsize,
               const uint8_t ascii_idx, const short max_chars_on_scr)
{
}

void load_files(const char* const* const filenames,
                void** const dsts, const short nfiles)
{
	FILE* file;
	char namebuffer[32];
	long size;
	int i;

	for  (i = 0; i < nfiles; ++i) {
		sprintf(namebuffer, "cdrom0:\\%s;1", filenames[i]);
		file = fopen(namebuffer, "r");

		if (file == NULL)
			FATALERROR("Couldn't open file %s", namebuffer);

		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (dsts[i] == NULL) {
			dsts[i] = MALLOC(size);
			if (dsts[i] == NULL)
				FATALERROR("Couldn't allocate memory!");
		}

		fread(dsts[i], 1, size, file);
	}
}

