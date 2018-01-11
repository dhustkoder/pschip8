#include <stdlib.h>
#include <stdio.h>
#include <libapi.h>
#include <libmath.h>
#include "log.h"
#include "system.h"
#include "chip8.h"


int main(void)
{
	uint32_t timer = 0;
	uint32_t step_last = 0;
	uint32_t fps_last = 0;
	short fps = 0;
	short steps = 0;

	init_systems();
	reset_timers();
	for (;;) {
		/*
		timer = get_usec_now();
		while ((timer - step_last) > (1000000u / CHIP8_FREQ)) {
			chip8_step();
			++steps;
			step_last += (1000000u / CHIP8_FREQ);
		}
		*/
		update_display(true);

		++fps;
		if ((timer - fps_last) >= 1000000) {
			loginfo("FPS: %d\n", fps);
			loginfo("STEPS: %d\n", steps);
			fps = 0;
			steps = 0;
			fps_last = timer;
		}
	}
}

