#include "system.h"
#include "chip8.h"


enum SubMenu {
	SUBMENU_CDROM,
	SUBMENU_MEMORY_CARD,
	SUBMENU_OPTIONS,
	SUBMENU_NSUBMENUS
};

enum ClbkArg {
	CLBKARG_INIT,
	CLBKARG_UPDATE,
	CLBKARG_EXIT_CLBK
};

enum ClbkRet {
	CLBKRET_INIT_CONFIRMED,
	CLBKRET_UPDATE_NO_CHANGE,
	CLBKRET_UPDATE_CHANGE,
	CLBKRET_EXIT_CLBK_CONFIRMED,
	CLBKRET_EXIT_MENU
};

typedef enum ClbkRet(*MenuClbk)(enum ClbkArg, void* clbkdata);

typedef struct MenuOptions {
	const char* fmt;
	const void* const* varpack;
	const Vec2* hand_positions;
	MenuClbk* clbks;
	void* clbkdata;
	short x, y;
	uint8_t size;
} MenuOptions;


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

static uint8_t run_menu(const MenuOptions opts)
{

	uint8_t index = 0;
	uint8_t index_old = index;
	bool clbk_action = false;
	bool clbk_action_old = clbk_action;
	bool exit_menu = false;
	uint16_t pad = get_paddata();
	uint16_t pad_old = pad;
	enum ClbkArg clbkarg;

	hand_move(opts.hand_positions[index]);
	
	while (!exit_menu) {
		pad = get_paddata();

		if (pad != pad_old) {
			if (!clbk_action) {
				if (pad&BUTTON_CIRCLE) {
					clbk_action = true;
				} else {
					if (pad&BUTTON_DOWN && index < (opts.size - 1))
						++index;
					else if (pad&BUTTON_UP && index > 0)
						--index;

					if (index != index_old) {
						hand_move(opts.hand_positions[index]);
						index_old = index;
					}
				}

			} else if (clbk_action && pad&BUTTON_CROSS) {
				clbk_action = false;
			}

			pad_old = pad;
		}

		if (clbk_action || clbk_action_old) {
			if (opts.clbks == NULL)
				break;

			if (clbk_action && !clbk_action_old) {
				clbkarg = CLBKARG_INIT;
			} else if (!clbk_action && clbk_action_old) {
				clbkarg = CLBKARG_EXIT_CLBK;
			} else {
				clbkarg = CLBKARG_UPDATE;
			}

			switch (opts.clbks[index](clbkarg, opts.clbkdata)) {
			case CLBKRET_INIT_CONFIRMED: clbk_action_old = true; break;
			case CLBKRET_EXIT_CLBK_CONFIRMED: clbk_action_old = false; break;
			case CLBKRET_EXIT_MENU: exit_menu = true; break;
			default: break;
			}
		}

		hand_animation_update();
		font_print(opts.x, opts.y, opts.fmt, opts.varpack);
		draw_sprites(&hand, 1);
		update_display(true);
	}

	return index;
}

static enum SubMenu main_menu(void)
{
	const Vec2 hand_positions[SUBMENU_NSUBMENUS] = {
		{ .x = 40,  .y = 38 },
		{ .x = 142, .y = 38 },
		{ .x = 82,  .y = 62 }
	};
	
	const MenuOptions menuopts = {
		.fmt = fntbuff,
		.varpack = NULL,
		.hand_positions = hand_positions,
		.clbks = NULL,
		.clbkdata = NULL,
		.size = 3,
		.x = 6,
		.y = 6
	};

	const enum SubMenu options[SUBMENU_NSUBMENUS] = {
		SUBMENU_CDROM, SUBMENU_MEMORY_CARD, SUBMENU_OPTIONS
	};

	sprintf(fntbuff,
	        "                  - PSCHIP8 -\n"
	        "      Chip8 Interpreter For PlayStation 1\n\n\n"
	        "           CDROM            Memory Card\n\n\n"
	        "                  Options");

	
	return options[run_menu(menuopts)];
}

static const char* cdrom_menu(void)
{
	const char* cdpaths[] = {
		"\\BRIX.CH8;1",
		"\\MISSILE.CH8;1",
		"\\INVADERS.CH8;1"
	};

	const Vec2 hand_positions[] = {
		{ 0, 16 },
		{ 0, 24 },
		{ 0, 32 }
	};

	const MenuOptions opts = {
		.fmt = "Brix\nMissile\nInvaders",
		.varpack = NULL,
		.clbks = NULL,
		.clbkdata = NULL,
		.hand_positions = hand_positions,
		.x = 32,
		.y = 16,
		.size = 3
	};

	return cdpaths[run_menu(opts)];
}

static void run_game(const char* const gamepath)
{
	extern Chip8Key chip8_keys;
	extern CHIP8_GFX_TYPE chip8_gfx[CHIP8_GFX_HEIGHT][CHIP8_GFX_WIDTH];

	const uint32_t usecs_per_step = 1000000u / CHIP8_FREQ;

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


		font_print(8, 8, fntbuff, NULL);
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
	load_sprite_sheet("\\SPRITES1.TIM;1", 32);

	reset_timers();

	for (;;) {
		switch (main_menu()) {
		case SUBMENU_CDROM:
			cdpath = cdrom_menu();
			if (cdpath != NULL)
				run_game(cdpath);
			break;
		case SUBMENU_OPTIONS:
			break;
		default:
			break;
		}
	}
}


