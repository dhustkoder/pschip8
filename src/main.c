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
	uint32_t timer = 0;
	uint32_t step_last = 0;
	uint32_t fps_last = 0;
	int freq = CHIP8_FREQ;
	int fps = 0;
	int steps = 0;
	unsigned int usecs_per_step = (1000000u / freq);
	uint16_t paddata;

	init_system();

	FntLoad(SCREEN_WIDTH, 0);
	SetDumpFnt(FntOpen(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 256));

	chip8_loadrom("\\BRIX.CH8;1");
	chip8_reset();
	reset_timers();

	for (;;) {
		timer = get_usec();
		while ((timer - step_last) > usecs_per_step) {
			chip8_step();
			++steps;
			step_last += usecs_per_step;
		}

		paddata = get_paddata();
		if (paddata&BUTTON_UP && freq < 1000) {
			++freq;
			usecs_per_step = (1000000u / freq);
		} else if (paddata&BUTTON_DOWN && freq > 10) {
			--freq;
			usecs_per_step = (1000000u / freq);
		} else if (paddata&BUTTON_START && paddata&BUTTON_SELECT) {
			chip8_reset();
		}

		update_display(false);
		FntPrint(fntbuff);
		FntFlush(-1);

		++fps;
		if ((timer - fps_last) >= 1000000u) {
			sprintf(fntbuff, 
			        "PSCHIP8 - Chip8 Interpreter for PS1!\n\n"
			        "Frames per second: %d\n"
			        "Steps per second: %d\n"
			        "UP and DOWN to control Chip8 Hz: %d\n"
			        "Press START & SELECT to reset",
			        fps, steps, freq);
			fps = 0;
			steps = 0;
			fps_last = timer;
		}
	}
}

