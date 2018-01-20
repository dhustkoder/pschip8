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
static char fntbuff[512];

static const char* game_select_menu(void)
{
	static Sprite hand = {
		.spos  = { .x = 0, .y = 8   },
		.size  = { .w = 26,.h = 14  },
		.tpos  = { .u = 0, .v = 0   }
	};
	static int8_t cursor = 0;
	static bool movefwd = true;
	
	const int8_t ngames = sizeof(games) / sizeof(games[0]);

	uint16_t pad_old = 0;
	uint16_t pad = 0;
	uint32_t last_move = 0;
	uint32_t last_fps = 0;
	int fps = 0;
	char* fntbuff_ptr = fntbuff;
	bool need_disp_update = true;
	DispFlag dispflags;
	int8_t i;

	SetDumpFnt(game_select_menu_fnt_stream);
	reset_timers();

	for (i = 0; i < ngames; ++i)
		fntbuff_ptr += sprintf(fntbuff_ptr, "%s\n\n", games[i].name);

	while (!(pad&BUTTON_CIRCLE)) {
		if ((get_msec() - last_fps) >= 1000u) {
			last_fps = get_msec();
			sprintf(fntbuff_ptr, "FPS: %d\n", fps);
			fps = 0;
			need_disp_update = true;
		}

		pad = get_paddata();

		if (cursor < (ngames - 1) && (pad&BUTTON_DOWN) && !(pad_old&BUTTON_DOWN)) {
			++cursor;
			hand.spos.y += 16;
			need_disp_update = true;
		} else if (cursor > 0 && (pad&BUTTON_UP) && !(pad_old&BUTTON_UP)) {
			--cursor;
			hand.spos.y -= 16;
			need_disp_update = true;
		}

		pad_old = pad;

		if ((get_msec() - last_move) > 50) {
			last_move = get_msec();
			need_disp_update = true;
			if (movefwd) {
				if (++hand.spos.x >= 8)
					movefwd = false;

			} else {
				if (--hand.spos.x <= 0)
					movefwd = true;
			}
		}

		dispflags = 0;
		if (need_disp_update) {
			FntPrint(fntbuff);
			FntFlush(-1);
			draw_sprites(&hand, 1);
			need_disp_update = false;
			dispflags |= DISPFLAG_SWAPBUFFERS;
		}

		update_display(dispflags);
		++fps;
	}

	return games[cursor].cdpath;
}

static void update_chip8_gfx(void)
{
	extern CHIP8_GFX_TYPE chip8_gfx[CHIP8_HEIGHT][CHIP8_WIDTH];
	draw_ram_buffer_scaled(chip8_gfx,
	                       (SCREEN_WIDTH / 2),
			       (SCREEN_HEIGHT / 2),
	                       CHIP8_WIDTH, CHIP8_HEIGHT,
			       2, 2);
}

static void run_game(const char* const gamepath)
{
	extern bool chip8_draw_flag;
	extern Chip8Key chip8_keys;

	static const Button button_tbl[] = {
		BUTTON_DOWN, BUTTON_L1, BUTTON_UP, BUTTON_R1,
		BUTTON_LEFT, BUTTON_CROSS, BUTTON_RIGHT,
		BUTTON_L2, BUTTON_DOWN, BUTTON_R2,
		BUTTON_START, BUTTON_SELECT, BUTTON_SQUARE,
		BUTTON_CIRCLE, BUTTON_TRIANGLE
	};

	const uint32_t usecs_per_step = (1000000u / CHIP8_FREQ);

	uint32_t timer = 0;
	uint32_t last_step = 0;
	uint32_t last_fps = 0;
	int fps = 0;
	int steps = 0;
	char* fntbuff_ptr = fntbuff;
	Button pad_old = 0;
	Button pad;
	DispFlag dispflags;
	int i;

	fntbuff_ptr += sprintf(fntbuff_ptr, 
	        "PSCHIP8 - Chip8 Interpreter for PS1!\n\n"
	        "Press START & SELECT to reset.\n\n");

	SetDumpFnt(run_game_fnt_stream);
	chip8_loadrom(gamepath);
	chip8_reset();
	reset_timers();

	for (;;) {
		pad = get_paddata();
		if (pad != pad_old) {
			if ((pad&BUTTON_START) && (pad&BUTTON_SELECT))
				break;

			for (i = 0; i < sizeof(button_tbl)/sizeof(button_tbl[0]); ++i) {
				if (pad&button_tbl[i])
					chip8_keys |= 0x1<<i;
				else
					chip8_keys &= ~(0x01<<i);
			}

			pad_old = pad;
		}


		timer = get_usec();
		while ((timer - last_step) > usecs_per_step) {
			chip8_step();
			++steps;
			last_step += usecs_per_step;
		}
		
		dispflags = 0;

		if (chip8_draw_flag) {
			FntPrint(fntbuff);
			FntFlush(-1);
			update_chip8_gfx();
			dispflags |= DISPFLAG_SWAPBUFFERS;
			chip8_draw_flag = false;
		}

		update_display(dispflags);

		++fps;
		if ((timer - last_fps) >= 1000000u) {
			sprintf(fntbuff_ptr, 
			        "Frames per second: %d\n"
			        "Steps per second: %d\n",
			        fps, steps);
			fps = 0;
			steps = 0;
			last_fps = timer;
		}
	}
}


int main(void)
{
	const char* gamepath;

	init_system();

	#ifdef DISPLAY_TYPE_PAL
	LOGINFO("display type is PAL\n");
	#else
	LOGINFO("display type is NTSC\n");
	#endif

	load_sprite_sheet("\\HAND.TIM;1");
	load_bkg_image("\\BKG.TIM;1");

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


