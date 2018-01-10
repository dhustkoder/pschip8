#include <stdlib.h>
#include <stdio.h>
#include <libcd.h>
#include <libapi.h>
#include <libmath.h>
#include "log.h"
#include "system.h"
#include "chip8.h"

static int ReadFromCD(const long nbytes , void * const dst, int mode)
// "nbytes" = number of bytes to read
// "dst" = address in memory where items will be written.
{
	int nsection, cnt, nsector;
	unsigned char com;

	nsector = (nbytes + 2047) / 2048;

	com=mode;
	CdControlB( CdlSetmode, &mode, 0);
	VSync(3);

	// Start reading.
	CdRead(nsector, dst, mode);

	while ((cnt = CdReadSync(1, 0)) > 0)
		VSync(0);

	return (cnt);

}


static void MyLoadFileFromCD(const char* const lpszFilename, void * const dst)
{
	CdlFILE fp;  // File

	int i, cnt;

	// Try 10 times to find the file.
	for (i=0; i<10; i++) {
		if (CdSearchFile(&fp, lpszFilename)==0)
			continue;  // Failed read, retry.

		// Set target position
		CdControl(CdlSetloc, (u_char*) &(fp.pos), 0);

		cnt = ReadFromCD(fp.size, (void*) dst, CdlModeSpeed); 
		if (!cnt) 
			break; 
	}

	if (i == 10)
		fatal_failure("Couldn't read CD");
}


static uint8_t buffer[0x800];

int main(void)
{
	uint32_t timer = 0;
	uint32_t step_last = 0;
	uint32_t fps_last = 0;
	short fps = 0;
	short steps = 0;

	init_systems();
	CdInit();

	MyLoadFileFromCD("\\BRIX.CH8;1", buffer);

	chip8_loadrom(buffer, sizeof buffer);
	chip8_reset();

	reset_timers();
	for (;;) {
		timer = get_usec_now();
		while ((timer - step_last) > (1000000u / CHIP8_FREQ)) {
			chip8_step();
			++steps;
			step_last += (1000000u / CHIP8_FREQ);
		}

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

