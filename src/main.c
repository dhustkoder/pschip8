#include "system.h"
#include "chip8.h"


static const struct GameInfo {
	const char* const name;
	const char* const cdpath;
} games[] = {
	{ "Brix",        "\\BRIX.CH8;1"     },
	{ "Blitz",       "\\BLITZ.CH8;1"    },
	{ "Invaders",    "\\INVADERS.CH8;1" },
	{ "Missile",     "\\MISSILE.CH8;1"  },
	{ "Pong",        "\\PONG.CH8;1"     },
	{ "Tetris",      "\\TETRIS.CH8;1"   },
	{ "UFO",         "\\UFO.CH8;1"      },
	{ "Tank",        "\\TANK.CH8;1"     },
	{ "Pong 2",      "\\PONG2.CH8;1"    },
	{ "VBrix",       "\\VBRIX.CH8;1"    },
	{ "Blinky",      "\\BLINKY.CH8;1"   },
	{ "Tic Tac Toe", "\\TICTAC.CH8;1"   }
};


static char fntbuff[512];

static const char* game_select_menu(void)
{
	static Sprite hand = {
		.spos  = { .x = 4, .y = 36  },
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
	int8_t i;

	reset_timers();

	fntbuff_ptr += sprintf(fntbuff_ptr,
	                       "     - PSCHIP8 - \n"
			       "    Chip8 for PS1!\n");

	for (i = 0; i < ngames; ++i)
		fntbuff_ptr += sprintf(fntbuff_ptr, "%s\n", games[i].name);

	while (!(pad&BUTTON_CIRCLE)) {
		if ((get_msec() - last_fps) >= 1000u) {
			last_fps = get_msec();
			sprintf(fntbuff_ptr, "FPS: %d\n", fps);
			fps = 0;
		}

		pad = get_paddata();

		if (cursor < (ngames - 1) && (pad&BUTTON_DOWN) && !(pad_old&BUTTON_DOWN)) {
			++cursor;
			hand.spos.y += 12;
		} else if (cursor > 0 && (pad&BUTTON_UP) && !(pad_old&BUTTON_UP)) {
			--cursor;
			hand.spos.y -= 12;
		}

		pad_old = pad;

		if ((get_msec() - last_move) > 1000u/8u) {
			last_move = get_msec();
			if (movefwd) {
				if (++hand.spos.x >= 8)
					movefwd = false;

			} else {
				if (--hand.spos.x <= 4)
					movefwd = true;
			}
		}

		font_print(26 + 8, 12, fntbuff);
		draw_sprites(&hand, 1);
		update_display(true);
		++fps;
	}

	return games[cursor].cdpath;
}

static void run_game(const char* const gamepath)
{
	extern Chip8Key chip8_keys;
	extern CHIP8_GFX_TYPE chip8_gfx[CHIP8_GFX_HEIGHT][CHIP8_GFX_WIDTH];

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
	int i;

	fntbuff_ptr += sprintf(fntbuff_ptr, "Reset: Start & Select\n");

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
					chip8_keys |= 0x01<<i;
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


		font_print(8, 8, fntbuff);
		draw_ram_buffer(chip8_gfx,
		                (SCREEN_WIDTH / 2), (SCREEN_HEIGHT / 2),
		                CHIP8_GFX_WIDTH, CHIP8_GFX_HEIGHT, 3, 3);
		update_display(true);

		++fps;
		if ((timer - last_fps) >= 1000000u) {
			sprintf(fntbuff_ptr,
			        "Frames Per Second: %d\n"
			        "Steps Per Second: %d", fps, steps);
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
	
	load_bkg("\\BKG.TIM;1");
	load_font("\\FONT2.TIM;1", 12, 12, 32, 256);
	load_sprite_sheet("\\HAND.TIM;1", 1);

	for (;;) {
		gamepath = game_select_menu();
		run_game(gamepath);
	}

}


