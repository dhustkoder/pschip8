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
#define SCREEN_HEIGHT  (240)
#ifdef DISPLAY_TYPE_PAL
#define NSEC_PER_HSYNC (64000u)
#else
#define NSEC_PER_HSYNC (63560u)
#endif

#define LOGINFO(...)  printf("INFO: " __VA_ARGS__)
#define LOGERROR(...) printf("ERROR: " __VA_ARGS__)

#define fatal_failure(...) {                     \
	LOGERROR("FATAL FAILURE: " __VA_ARGS__); \
	LOGERROR("\n");                          \
	SystemError(_get_errno(), _get_errno()); \
}

#ifdef DEBUG

#define LOGDEBUG(...) printf("DEBUG: " __VA_ARGS__)

#define ASSERT_MSG(cond, ...) {              \
	if (!(cond))                         \
		fatal_failure(__VA_ARGS__);  \
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

typedef uint8_t DispFlag;
enum DispFlag {
	DISPFLAG_DRAWSYNC    = 0x01,
	DISPFLAG_VSYNC       = 0x02,
	DISPFLAG_SWAPBUFFERS = 0x04
};

typedef struct Sprite {
	struct { short  x, y; } spos;
	struct { u_char u, v; } tpos;
	struct { short  w, h; } size;
} Sprite;


void init_system(void);
void update_display(DispFlag flags); // shall be called once every frame.
void load_bkg_image(const char* cdpath);
void load_sprite_sheet(const char* cdpath);
void draw_sprites(const Sprite* sprites, short size);
void update_timers(void);
void reset_timers(void);
void load_files(const char* const * filenames,
                void* const * dsts,
                int nfiles);


static inline void draw_ram_buffer(void* pixels,
                                   const short screen_x,
                                   const short screen_y,
                                   const short width,
                                   const short height)
{
	extern const DRAWENV* sys_curr_drawenv;

	RECT rect = { 
		.x = screen_x + sys_curr_drawenv->clip.x,
		.y = screen_y + sys_curr_drawenv->clip.y,
		.w = width,
		.h = height
	};

	LoadImage(&rect, pixels);
}

static inline void draw_vram_buffer(const short dst_x, const short dst_y,
                                    const short src_x, const short src_y,
                                    const short w, const short h)
{
	extern const DRAWENV* sys_curr_drawenv;

	RECT rect = {
		.x = src_x,
		.y = src_y,
		.w = w,
		.h = h
	};

	MoveImage(&rect,
	          sys_curr_drawenv->clip.x + dst_x,
	          sys_curr_drawenv->clip.y + dst_y);

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
