#ifndef PSCHIP8_SYSTEM_H_
#define PSCHIP8_SYSTEM_H_
#include <libetc.h>
#include <libapi.h>
#include <sys/errno.h>
#include "log.h"
#include "ints.h"

#define SCREEN_WIDTH  (320)
#ifdef DISPLAY_TYPE_PAL
#define SCREEN_HEIGHT (256)
#else
#define SCREEN_HEIGHT (240)
#endif

#define fatal_failure(...) logerror("FATAL FAILURE: " __VA_ARGS__); \
                           SystemError(_get_errno(), _get_errno())

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



void init_systems(void);
void update_display(bool vsync);
void update_timers(void);
void reset_timers(void);
void open_cd_files(const char* const * filenames,
                   uint8_t* const * dsts,
                   int nfiles);

// msec counter resets every 4294967 seconds
// usec counter resets every 4294 seconds

// msec counter at the last vsync or last call to update_timers()
static inline uint32_t get_msec(void)
{
	extern uint32_t sys_msec_timer;
	return sys_msec_timer;
}

// msec counter at the last vsync or last call to update_timers()
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

static inline void update_pads(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = PadRead(0);
}


#endif
