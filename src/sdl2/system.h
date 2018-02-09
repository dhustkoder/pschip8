#ifndef PSCHIP8_SYSTEM_H_ /* PSCHIP8_SYSTEM_H_ */
#define PSCHIP8_SYSTEM_H_ 
#include <stdio.h>
#include <stdlib.h>
#ifdef DEBUG /* DEBUG */
#include <assert.h>
#endif /* DEBUG */
#include <SDL2/SDL.h>


#define uint8_t  Uint8
#define uint16_t Uint16
#define uint32_t Uint32
#define int8_t   Sint8
#define int16_t  Sint16
#define int32_t  Sint32
#define bool     Uint8
#define true     ((bool)1)
#define false    ((bool)0)


#define MALLOC  malloc
#define REALLOC realloc
#define FREE    free


#define SCREEN_WIDTH  (320)
#define SCREEN_HEIGHT (256)

/* logging / crash */
void sys_log(const char* cat, const char* fmt, ...);
void sys_fatalerror(const char* fmt, ...);
#define LOGAUX(category, fmt, ...) sys_log(category, fmt, __VA_ARGS__)
#define FATALERROR(...)            sys_fatalerror(__VA_ARGS__)
#define LOGINFO(...)               LOGAUX("[INFO]", __VA_ARGS__)
#define LOGERROR(...)              LOGAUX("[ERROR]", __VA_ARGS__)

#ifdef DEBUG /* DEBUG */

#define LOGDEBUG(...) LOGAUX("[DEBUG]", __VA_ARGS__)

#define ASSERT_MSG(cond, ...) {          \
	if (!(cond))                     \
		FATALERROR(__VA_ARGS__); \
}

#else

#define LOGDEBUG(...)   ((void)0)
#define ASSERT_MSG(...) ((void)0)

#endif /* DEBUG */



typedef uint16_t button_t;
enum Button {
	BUTTON_UP       = 0x1000,
	BUTTON_DOWN     = 0x2000,
	BUTTON_LEFT     = 0x4000,
	BUTTON_RIGHT    = 0x8000,
	BUTTON_R2       = 0x0100,
	BUTTON_L2       = 0x0200,
	BUTTON_START    = 0x0010,
	BUTTON_SELECT   = 0x0020,
	BUTTON_L1       = 0x0040,
	BUTTON_R1       = 0x0080,
	BUTTON_SQUARE   = 0x0001,
	BUTTON_CROSS    = 0x0002,
	BUTTON_CIRCLE   = 0x0004,
	BUTTON_TRIANGLE = 0x0008

};


struct vec2 {
	int16_t x, y;
};

struct sprite {
	struct vec2 spos;
	struct vec2 size;
	struct vec2 tpos;
};


void init_system(void);
void term_system(void);
void reset_timers(void);
void update_timers(void);
void update_display(void);
void font_print(const struct vec2* pos, const char* fmt, const void* const* varpack);
void draw_sprites(const struct sprite* sprites, short nsprites);
void draw_ram_buffer(void);
void assign_snd_chan(uint8_t chan, uint8_t snd_index);
void enable_chan(uint8_t chan);
void load_sprite_sheet(const void* data, short max_sprites_on_screen);
void load_bkg(const void* data);
void load_font(const void* data, const struct vec2* charsize,
               uint8_t ascii_idx, short max_chars_on_scr);
void load_snd(void* const* snds, short nsnds);
void load_ram_buffer(void* pixels, const struct vec2* pos,
                     const struct vec2* size, uint8_t scale);
void load_files(const char* const* filenames, void** dsts, short nfiles);


static inline void load_sync(void)
{
	const uint32_t ticks = SDL_GetTicks();
	while (SDL_GetTicks() < (ticks + 5000))
		update_display();
}

static inline uint16_t get_paddata(void)
{
	extern uint16_t sys_paddata;
	return sys_paddata;
}

static inline uint32_t get_msec(void)
{
	extern uint32_t sys_msec_timer;
	return sys_msec_timer;
}

static inline uint32_t get_msec_now(void)
{
	extern uint32_t sys_msec_timer;
	update_timers();
	return sys_msec_timer;
}




#endif /* PSCHIP8_SYSTEM_H_ */
