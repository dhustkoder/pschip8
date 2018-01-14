#include "system.h"
#include "chip8.h"


extern bool chip8_draw_flag;


static const struct GameInfo {
	const char* const name;
	const char* const cdpath;
} games[] = {
	{"BRIX", "\\BRIX.CH8;1" },
	{"BLITZ", "\\BLITZ.CH8;1" },
	{"INVADERS", "\\INVADERS.CH8;1" },
	{"MISSILE", "\\MISSILE.CH8;1" },
	{"PONG", "\\PONG.CH8;1" },
	{"TETRIS", "\\TETRIS.CH8;1" },
};

static char fntbuff[256] = { '\0' };

static const char* game_select_menu(void)
{
	const int8_t ngames = sizeof(games) / sizeof(games[0]);
	int8_t cursor = 0;
	uint16_t pad_old = 0;
	uint16_t pad = 0;
	int8_t i;

	while (!(pad&BUTTON_CIRCLE)) {
		for (i = 0; i < ngames; ++i)
			FntPrint("%c %s\n\n", cursor == i ? '>' : ' ', games[i].name);

		pad = get_paddata();

		if (cursor < (ngames - 1) && (pad&BUTTON_DOWN) && !(pad_old&BUTTON_DOWN))
			++cursor;
		else if (cursor > 0 && (pad&BUTTON_UP) && !(pad_old&BUTTON_UP))
			--cursor;

		pad_old = pad;

		FntFlush(-1);
		update_display(DISP_FLAG_DRAW_SYNC|DISP_FLAG_VSYNC|DISP_FLAG_SWAP_BUFFERS);
	}

	return games[cursor].cdpath;
}

static void draw_chip8_gfx(void)
{
	extern bool chip8_gfx[CHIP8_HEIGHT][CHIP8_WIDTH];

	static uint16_t scaled_chip8_gfx[CHIP8_SCALED_HEIGHT][CHIP8_SCALED_WIDTH];

	const uint32_t xratio = ((CHIP8_WIDTH<<16) / CHIP8_SCALED_WIDTH) + 1;
	const uint32_t yratio = ((CHIP8_HEIGHT<<16) / CHIP8_SCALED_HEIGHT) + 1;
	uint32_t px, py;
	short i, j;

	// scale chip8 graphics
	for (i = 0; i < CHIP8_SCALED_HEIGHT; ++i) {
		py = (i * yratio)>>16;
		for (j = 0; j < CHIP8_SCALED_WIDTH; ++j) {
			px = (j * xratio)>>16;
			scaled_chip8_gfx[i][j] = chip8_gfx[py][px] ? ~0 : 0;
		}
	}
	
	draw_ram_buffer(scaled_chip8_gfx,
	               (SCREEN_WIDTH / 2) - (CHIP8_SCALED_WIDTH / 2),
	               (SCREEN_HEIGHT / 2) - (CHIP8_SCALED_HEIGHT / 2),
	               CHIP8_SCALED_WIDTH, CHIP8_SCALED_HEIGHT);
}

static void run_game(const char* const gamepath)
{
	const uint32_t usecs_per_step = (1000000u / CHIP8_FREQ);

	uint32_t timer = 0;
	uint32_t step_last = 0;
	uint32_t fps_last = 0;
	int fps = 0;
	int steps = 0;
	DispFlag dispflags = 0;
	uint16_t pad;

	chip8_loadrom(gamepath);
	chip8_reset();
	reset_timers();

	for (;;) {
		timer = get_usec();
		while ((timer - step_last) > usecs_per_step) {
			chip8_step();
			++steps;
			step_last += usecs_per_step;
		}

		pad = get_paddata();
		if ((pad&BUTTON_START) && (pad&BUTTON_SELECT)) {
			break;
		}

		if (chip8_draw_flag) {
			draw_chip8_gfx();
			dispflags = DISP_FLAG_DRAW_SYNC|DISP_FLAG_SWAP_BUFFERS;
			chip8_draw_flag = 0;
		} else {
			dispflags = DISP_FLAG_DRAW_SYNC;
		}

		FntPrint(fntbuff);
		FntFlush(-1);
		update_display(dispflags);

		++fps;
		if ((timer - fps_last) >= 1000000u) {
			sprintf(fntbuff, 
			        "PSCHIP8 - Chip8 Interpreter for PS1!\n\n"
			        "Frames per second: %d\n"
			        "Steps per second: %d\n"
			        "Press START & SELECT to reset",
			        fps, steps);
			fps = 0;
			steps = 0;
			fps_last = timer;
		}
	}
}


int main(void)
{
	const char* gamepath;

	init_system();

	FntLoad(SCREEN_WIDTH, 0);
	SetDumpFnt(FntOpen(0, 12, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 256));

	for (;;) {
		gamepath = game_select_menu();
		run_game(gamepath);
	}
	
}


