#include "system.h"
#include "chip8.h"


enum SubMenu {
	SUBMENU_CDROM,
	SUBMENU_MEMORY_CARD,
	SUBMENU_OPTIONS,
	SUBMENU_NSUBMENUS
};


static char fntbuff[512];

static Button button_tbl[] = {
	BUTTON_DOWN, BUTTON_L1, BUTTON_UP, BUTTON_R1,
	BUTTON_LEFT, BUTTON_CROSS, BUTTON_RIGHT,
	BUTTON_L2, BUTTON_DOWN, BUTTON_R2,
	BUTTON_START, BUTTON_SELECT, BUTTON_SQUARE,
	BUTTON_CIRCLE, BUTTON_TRIANGLE
};

static Sprite hand = {
	.size  = { .w = 26, .h = 14  },
	.tpos  = { .u = 0,  .v = 0   }
};


static void hand_move(const Vec2 vec2)
{
	hand.spos.x = vec2.x;
	hand.spos.y = vec2.y;
}

static void hand_animation_update(void)
{
	static uint32_t msec_last;
	static uint32_t fwd_last;
	static bool fwd = true;

	if ((get_msec() - fwd_last) > 1000u / 2u) {
		fwd_last = get_msec();
		fwd = !fwd;
	}
	
	if ((get_msec() - msec_last) > 1000u / 8u) {
		msec_last = get_msec();
		if (fwd)
			++hand.spos.x;
		else
			--hand.spos.x;
	}

}

static enum SubMenu main_menu(void)
{
	static const Vec2 hand_positions[SUBMENU_NSUBMENUS] = {
		{ .x = 40,  .y = 38 },
		{ .x = 142, .y = 38 },
		{ .x = 82,  .y = 62 }
	};

	static enum SubMenu option = SUBMENU_CDROM;


	char* fntbuff_p = fntbuff;
	uint16_t pad = get_paddata();
	uint16_t pad_old = pad;
	enum SubMenu option_old = option;

	fntbuff_p += sprintf(fntbuff_p,
			"                  - PSCHIP8 -\n"
			"      Chip8 Interpreter For PlayStation 1\n\n\n"
			"           CDROM            Memory Card\n\n\n"
			"                  Options");

	hand_move(hand_positions[option]);

	for (;;) {
		pad = get_paddata();
		if (pad != pad_old) {
			if (pad&BUTTON_CIRCLE)
				break;

			if (pad&BUTTON_RIGHT)
				option = SUBMENU_MEMORY_CARD;
			else if (pad&BUTTON_LEFT)
				option = SUBMENU_CDROM;
			else if (pad&BUTTON_DOWN)
				option = SUBMENU_OPTIONS;
			
			if (option != option_old) {
				hand_move(hand_positions[option]);
				option_old = option;
			}

			pad_old = pad;
		}

		hand_animation_update();
		font_print(6, 6, fntbuff);
		draw_sprites(&hand, 1);
		update_display(true);
	}

	return option;
}

static const char* cdrom_menu(void)
{
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

	static const int8_t ngames = sizeof(games) / sizeof(games[0]);

	static int8_t cursor = 0;
	static Vec2 hand_pos = { .x = 30, .y = 8 };

	uint16_t pad = get_paddata();
	uint16_t pad_old = pad;
	char* fntbuff_ptr = fntbuff;
	int8_t cursor_old = cursor;
	int8_t i;

	hand_move(hand_pos);

	for (i = 0; i < ngames; ++i)
		fntbuff_ptr += sprintf(fntbuff_ptr, "%s\n", games[i].name);

	for (;;) {
		pad = get_paddata();
		
		if (pad != pad_old) {
			if (pad&BUTTON_CIRCLE)
				break;

			if (cursor < (ngames - 1) &&
			   (pad&BUTTON_DOWN)      &&
			   !(pad_old&BUTTON_DOWN)) {
				++cursor;
				hand_pos.y += 8;
			} else if (cursor > 0 &&
				  (pad&BUTTON_UP) &&
				  !(pad_old&BUTTON_UP)) {
				--cursor;
				hand_pos.y -= 8;
			}

			pad_old = pad;

			if (cursor != cursor_old) {
				cursor_old = cursor;
				hand_move(hand_pos);
			}
		}

		hand_animation_update();
		font_print(62, 8, fntbuff);
		draw_sprites(&hand, 1);
		update_display(true);
	}

	return games[cursor].cdpath;
}

static void run_game(const char* const gamepath)
{
	extern Chip8Key chip8_keys;
	extern CHIP8_GFX_TYPE chip8_gfx[CHIP8_GFX_HEIGHT][CHIP8_GFX_WIDTH];

	static const uint32_t usecs_per_step = (1000000u / CHIP8_FREQ);

	uint32_t timer = 0;
	uint32_t last_step = 0;
	uint32_t last_sec = 0;
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

		if ((timer - last_sec) >= 1000000u) {
			sprintf(fntbuff_ptr, "Steps Per Second: %d", steps);
			steps = 0;
			last_sec = timer;
		}
	}
}



int main(void)
{
	const char* cdpath;

	init_system();

	#ifdef DISPLAY_TYPE_PAL
	LOGINFO("display type is PAL\n");
	#else
	LOGINFO("display type is NTSC\n");
	#endif
	
	load_bkg("\\BKG.TIM;1");
	load_font("\\FONT3.TIM;1", 6, 8, 32, 256);
	load_sprite_sheet("\\HAND.TIM;1", 1);

	reset_timers();
	for (;;) {
		switch (main_menu()) {
		case SUBMENU_CDROM:
			cdpath = cdrom_menu();
			if (cdpath != NULL)
				run_game(cdpath);
			break;
		default: break;
		}
	}

}


