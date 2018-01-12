#include <stdlib.h>
#include <stdio.h>
#include <libmath.h>
#include <libgte.h>
#include <libapi.h>
#include <libgpu.h>
#include "log.h"
#include "system.h"
#include "chip8.h"


static char fntbuff[256] = { '\0' };


int main(void)
{
	extern uint16_t sys_paddata;
	uint32_t timer = 0;
	uint32_t step_last = 0;
	uint32_t fps_last = 0;
	int freq = CHIP8_FREQ;
	int fps = 0;
	int steps = 0;
	unsigned int usecs_per_step = (1000000u / freq);

	init_systems();
	chip8_loadrom("\\BRIX.CH8;1");
	chip8_reset();
	reset_timers();

	FntLoad(SCREEN_WIDTH, 0);
	SetDumpFnt(FntOpen(0, 0, SCREEN_WIDTH, 64, 0, 256));

	for (;;) {
		timer = get_usec_now();
		while ((timer - step_last) > usecs_per_step) {
			chip8_step();
			++steps;
			step_last += usecs_per_step;
		}

		update_pads();
		if (sys_paddata&BUTTON_UP && freq < 1000) {
			++freq;
			usecs_per_step = (1000000u / freq);
		} else if (sys_paddata&BUTTON_DOWN && freq > 10) {
			--freq;
			usecs_per_step = (1000000u / freq);
		}

		update_display(true);
		FntPrint(fntbuff);
		FntFlush(-1);

		++fps;
		if ((timer - fps_last) >= 1000000u) {
			sprintf(fntbuff, 
			        "PSCHIP8 - Chip8 Interpreter for PS1!\n\n"
			        "Frames per second: %d\n\n"
			        "Steps per second: %d\n\n"
			        "UP and DOWN to control Chip8 Hz: %d.",
			        fps, steps, freq);
			fps = 0;
			steps = 0;
			fps_last = timer;
		}
	}
}

