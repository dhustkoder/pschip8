#ifndef PSCHIP8_SYSTEM_H_
#define PSCHIP8_SYSTEM_H_
#include <sys/errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <libapi.h>
#include <libgte.h>
#include <libgpu.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

typedef uint8_t bool;
#define false ((bool)0)
#define true  ((bool)1)

#define SCREEN_WIDTH   (320)
#ifdef DISPLAY_TYPE_PAL
#define SCREEN_HEIGHT  (256)
#define NSEC_PER_HSYNC (64000u)
#else
#define SCREEN_HEIGHT  (240)
#define NSEC_PER_HSYNC (63560u)
#endif

#define LOGINFO(...)  printf("INFO: " __VA_ARGS__)
#define LOGERROR(...) printf("ERROR: " __VA_ARGS__)

#define FATALERROR(...) {                        \
	LOGERROR("FATAL FAILURE: " __VA_ARGS__); \
	LOGERROR("\n");                          \
	SystemError(_get_errno(), _get_errno()); \
}

#ifdef DEBUG

#define LOGDEBUG(...) printf("DEBUG: " __VA_ARGS__)

#define ASSERT_MSG(cond, ...) {              \
	if (!(cond))                         \
		FATALERROR(__VA_ARGS__);     \
}

#else

#define LOGDEBUG(...)   ((void)0)
#define ASSERT_MSG(...) ((void)0)

#endif



typedef uint16_t Button;
enum Button {
	BUTTON_LEFT     = 0x8000,
	BUTTON_DOWN     = 0x4000,
	BUTTON_RIGHT    = 0x2000,
	BUTTON_UP       = 0x1000,
	BUTTON_START    = 0x0800,
	BUTTON_SELECT   = 0x0100,
	BUTTON_SQUARE   = 0x0080,
	BUTTON_CROSS    = 0x0040,
	BUTTON_CIRCLE   = 0x0020,
	BUTTON_TRIANGLE = 0x0010,
	BUTTON_R1       = 0x0008,
	BUTTON_L1       = 0x0004,
	BUTTON_R2       = 0x0002,
	BUTTON_L2       = 0x0001
};


typedef struct Vec2 {
	short x, y;
} Vec2;

typedef struct Sprite {
	struct { short x, y;  } spos;
	struct { short w, h;  } size;
	struct { u_char u, v; } tpos;
} Sprite;


void init_system(void);
void reset_timers(void);
void update_timers(void);
void update_display(bool vsync);

void font_print(short x, short y, const char* fmt, ...);
void draw_sprites(const Sprite* sprites, short nsprites);
void draw_ram_buffer(void* pixels,
                     const short screenx, const short screeny,
                     const short width, const short height,
                     const uint8_t scalex, const uint8_t scaley);

void load_bkg(const char* cdpath);
void load_sprite_sheet(const char* cdpath, short maxsprites_on_screen);
void load_font(const char* cdpath, uint8_t cw, uint8_t ch,
               uint8_t ascii_idx, short max_chars_on_scr);
void load_files(const char* const* filenames, void** dsts, short nfiles);



static inline void reset_pad(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = 0;
}

static inline uint16_t get_paddata(void)
{
	extern uint16_t sys_paddata;
	return sys_paddata;
}

// update_timers() is called in the VSync callback function
// msec counter resets every 4294967 seconds
// usec counter resets every 4294 seconds

// msec counter at last call to update_timers()
static inline uint32_t get_msec(void)
{
	extern uint32_t sys_msec_timer;
	return sys_msec_timer;
}

// msec counter at last call to update_timers()
static inline uint32_t get_usec(void)
{
	extern uint32_t sys_usec_timer;
	return sys_usec_timer;
}

// msec at this precise moment
static inline uint32_t get_msec_now(void)
{
	update_timers();
	return get_msec();
}

// usec at this precise moment
static inline uint32_t get_usec_now(void)
{
	update_timers();
	return get_usec();
}


#endif
