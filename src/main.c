#include "system.h"
#include "chip8.h"


static const struct GameInfo {
	const char* const name;
	const char* const cdpath;
} games[] = {
	{ "BRIX",     "\\BRIX.CH8;1"     },
	{ "BLITZ",    "\\BLITZ.CH8;1"    },
	{ "INVADERS", "\\INVADERS.CH8;1" },
	{ "MISSILE",  "\\MISSILE.CH8;1"  },
	{ "PONG",     "\\PONG.CH8;1"     },
	{ "TETRIS",   "\\TETRIS.CH8;1"   },
	{ "UFO",      "\\UFO.CH8;1"      },
	{ "TANK",     "\\TANK.CH8;1"     },
	{ "PONG2",    "\\PONG2.CH8;1"    },
	{ "VBRIX",    "\\VBRIX.CH8;1"    },
	{ "BLINKY",   "\\BLINKY.CH8;1"   },
	{ "TICTAC",   "\\TICTAC.CH8;1"   }
};

static long game_select_menu_fnt_stream;
static long run_game_fnt_stream;

static const char* game_select_menu(void)
{
	static Sprite hand = {
		.tpos  = { .u = 0, .v = 0   },
		.spos  = { .x = 0, .y = 8   },
		.size  = { .w = 26, .h = 14 },
	};
	static int8_t cursor = 0;
	static bool movefwd = true;
	
	const int8_t ngames = sizeof(games) / sizeof(games[0]);

	uint16_t pad_old = 0;
	uint16_t pad = 0;
	int last_move = 0;
	int8_t i;

	SetDumpFnt(game_select_menu_fnt_stream);
	reset_timers();

	while (!(pad&BUTTON_CIRCLE)) {
		for (i = 0; i < ngames; ++i)
			FntPrint("%s\n\n", games[i].name);

		FntPrint("%d:%d:%d", get_msec()/1000u,
		         get_msec()%1000, get_usec()%1000);

		pad = get_paddata();

		if (cursor < (ngames - 1) && (pad&BUTTON_DOWN) && !(pad_old&BUTTON_DOWN)) {
			++cursor;
			hand.spos.y += 16;
		} else if (cursor > 0 && (pad&BUTTON_UP) && !(pad_old&BUTTON_UP)) {
			--cursor;
			hand.spos.y -= 16;
		}

		pad_old = pad;

		if ((get_msec() - last_move) > 50) {
			last_move = get_msec();
			if (movefwd) {
				if (++hand.spos.x >= 8)
					movefwd = false;

			} else {
				if (--hand.spos.x <= 0)
					movefwd = true;
			}
		}

		FntFlush(-1);
		draw_sprites(&hand, 1);
		update_display(DISPFLAG_DRAWSYNC|DISPFLAG_VSYNC|DISPFLAG_SWAPBUFFERS);
	}

	return games[cursor].cdpath;
}

static void update_chip8_gfx(void)
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
	extern bool chip8_draw_flag;

	static char fntbuff[256];

	const uint32_t usecs_per_step = (1000000u / CHIP8_FREQ);

	uint32_t timer = 0;
	uint32_t step_last = 0;
	uint32_t fps_last = 0;
	int fps = 0;
	int steps = 0;
	uint16_t pad;
	DispFlag dispflags;

	fntbuff[0] = '\0';
	SetDumpFnt(run_game_fnt_stream);
	chip8_loadrom(gamepath);
	chip8_reset();
	reset_timers();

	for (;;) {
		pad = get_paddata();
		if ((pad&BUTTON_START) && (pad&BUTTON_SELECT))
			break;


		timer = get_usec();
		while ((timer - step_last) > usecs_per_step) {
			chip8_step();
			++steps;
			step_last += usecs_per_step;
		}
		
		dispflags = 0;

		if (chip8_draw_flag) {
			FntPrint(fntbuff);
			FntFlush(-1);
			update_chip8_gfx();
			dispflags |= DISPFLAG_DRAWSYNC|DISPFLAG_SWAPBUFFERS;
			chip8_draw_flag = false;
		}

		update_display(dispflags|DISPFLAG_VSYNC);

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

	#ifdef DISPLAY_TYPE_PAL
	loginfo("display type is PAL\n");
	#else
	loginfo("display type is NTSC\n");
	#endif

	load_sprite_sheet("\\HAND.TIM;1");

	FntLoad(960, 0);

	game_select_menu_fnt_stream = 
		FntOpen(36, 8,
		        SCREEN_WIDTH - 36,
			SCREEN_HEIGHT - 8, 
	                0, 512);

	run_game_fnt_stream =
		FntOpen(0, 12,
		        SCREEN_WIDTH,
		        SCREEN_HEIGHT - 12,
			0, 256);

	for (;;) {
		gamepath = game_select_menu();
		run_game(gamepath);
	}
		
}


