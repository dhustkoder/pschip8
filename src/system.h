#ifndef PSCHIP8_SYSTEM_H_
#define PSCHIP8_SYSTEM_H_
#include <libetc.h>
#include "ints.h"

#define SCREEN_WIDTH  320     // screen width
#define SCREEN_HEIGHT 256     // screen height (240 NTSC, 256 PAL)

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
void fatal_failure(const char* msg);

static inline int32_t get_msec_now(void)
{
	extern int32_t sys_msec_timer;
	update_timers();
	return sys_msec_timer;
}

static inline void update_pads(void)
{
	extern uint16_t sys_paddata;
	sys_paddata = PadRead(0);
}


#endif
