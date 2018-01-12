#include <stdlib.h>
#include <stdio.h>
#include <libmath.h>
#include <libgte.h>
#include <libapi.h>
#include <libgpu.h>
#include "log.h"
#include "system.h"
#include "chip8.h"


int main(void)
{
	extern uint16_t sys_paddata;
	uint32_t timer = 0;
	uint32_t step_last = 0;
	uint32_t fps_last = 0;
	short freq = CHIP8_FREQ;
	short fps = 0;
	short steps = 0;
	unsigned int usecs_per_step = (1000000u / freq);

	init_systems();
	chip8_loadrom("\\BRIX.CH8;1");
	chip8_reset();
	reset_timers();

	FntLoad(SCREEN_WIDTH, 0);
	SetDumpFnt(FntOpen(0, 0, SCREEN_WIDTH, 64, 1, 256));

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

		++fps;
		if ((timer - fps_last) >= 1000000u) {
			FntPrint("PSCHIP8 - Chip8 Interpreter for PS1!\n\n");
			FntPrint("Frames per second: %d\n\n", fps);
			FntPrint("Steps per second: %d\n\n", steps);
			FntPrint("UP and DOWN to control Chip8 Hz: %d.", freq);
			FntFlush(-1);
			fps = 0;
			steps = 0;
			fps_last = timer;
		}
	}
}

