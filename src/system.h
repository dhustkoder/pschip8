#ifndef PSCHIP8_SYSTEM_H_
#define PSCHIP8_SYSTEM_H_

#define SCREEN_WIDTH  320     // screen width
#define SCREEN_HEIGHT 256     // screen height (240 NTSC, 256 PAL)

void init_systems(void);
void update_display(void);
void update_pads(void);
void update_timers(void);
void reset_timers(void);

static inline unsigned long get_msec_timer(void)
{
	extern unsigned long sys_msec_timer;
	update_timers();
	return sys_msec_timer;
}

#endif
