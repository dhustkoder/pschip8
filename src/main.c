#include "system.h"
#include "chip8.h"


enum Menu {
	MENU_MAIN,
	MENU_CDROM,
	MENU_MEMORY_CARD,
	MENU_OPTIONS,
	MENU_NMENUS
};

static char fntbuff[512];

static Button button_tbl[] = {
	BUTTON_DOWN, BUTTON_L1, BUTTON_UP, BUTTON_R1,
	BUTTON_LEFT, BUTTON_CROSS, BUTTON_RIGHT,
	BUTTON_L2, BUTTON_DOWN, BUTTON_R2,
	BUTTON_START, BUTTON_SELECT, BUTTON_SQUARE,
	BUTTON_CIRCLE, BUTTON_TRIANGLE
};

static int chip8_freq = CHIP8_FREQ;

static void sprite_move(const Vec2 vec2, Sprite* const sprite)
{
	sprite->spos.x = vec2.x;
	sprite->spos.y = vec2.y;
}

static void sprite_animation_update(const uint8_t fwd_div,
                                    const uint8_t mov_div,
                                    uint32_t* const msec_last,
                                    uint32_t* const fwd_last,
                                    bool* const fwd,
                                    Sprite* const sprite)
{
	if ((get_msec() - *fwd_last) > 1000u / fwd_div) {
		*fwd_last = get_msec();
		*fwd = !(*fwd);
	}
	
	if ((get_msec() - *msec_last) > 1000u / mov_div) {
		*msec_last = get_msec();
		if (*fwd)
			++sprite->spos.x;
		else
			--sprite->spos.x;
	}

}

static void run_menu(const enum Menu menu, void* out)
{
	// Main Menu
	const char* main_menu_fmt =
		"                - PSCHIP8 -\n\n\n"
		"          CDROM               Memory Card\n\n\n"
		"                  Options\n";

	const Vec2 hand_pos_main_menu[] = {
		{ (10 * 6) - 32, 3 * 8 },
		{ (30 * 6) - 32, 3 * 8 },
		{ (18 * 6) - 32, 6 * 8 }
	};

	const enum Menu main_menu_out[] = {
		MENU_CDROM, MENU_MEMORY_CARD, MENU_OPTIONS
	};

	// CDROM Menu
	const char* cdrom_menu_fmt =
		"             - CDROM -\n\n\n"
		"                Brix\n\n"
		"               Missile\n\n"
		"                Tank";

	const Vec2 hand_pos_cdrom_menu[] = {
		{ (15 * 6) - 32, 3 * 8 },
		{ (15 * 6) - 32, 5 * 8 },
		{ (15 * 6) - 32, 7 * 8 }
	};

	const char* const cdrom_menu_out[] = {
		"\\BRIX.CH8;1", "\\MISSILE.CH8;1", "\\TANK.CH8;1"
	};


	// Logic
	Sprite hand = {
		.size  = { .w = 26, .h = 14  },
		.tpos  = { .u = 0,  .v = 0   }
	};

	uint32_t hand_anim_last = 0;
	uint32_t hand_fwd_last = 0;
	bool hand_fwd = true;

	uint16_t pad = get_paddata();
	uint16_t pad_old = pad;
	short index = 0;
	short index_old = index;
	short max_index;
	const Vec2* hand_pos;

	switch (menu) {
	default:
	case MENU_MAIN:
		max_index = 2;
		hand_pos = hand_pos_main_menu;
		strcpy(fntbuff, main_menu_fmt);
		break;
	case MENU_CDROM:
		max_index = 2;
		hand_pos = hand_pos_cdrom_menu;
		strcpy(fntbuff, cdrom_menu_fmt);
		break;
	}

	sprite_move(hand_pos[index], &hand);

	for (;;) {
		pad = get_paddata();

		if (pad != pad_old) {
			if (pad&BUTTON_CIRCLE)
				break;

			if (menu == MENU_MAIN) {
				if (pad&BUTTON_LEFT)
					index = 0;
				else if (pad&BUTTON_RIGHT)
					index = 1;
				else if (pad&BUTTON_DOWN)
					index = 2;
			} else if (menu == MENU_CDROM) {
				if (pad&BUTTON_CROSS) {
					index = -1;
					break;
				}

				if (pad&BUTTON_DOWN && index < max_index)
					++index;
				else if (pad&BUTTON_UP && index > 0)
					--index;
			}

			if (index != index_old) {
				sprite_move(hand_pos[index], &hand);
				index_old = index;
			}

			pad_old = pad;
		}

		sprite_animation_update(2u, 8u, &hand_anim_last,
					&hand_fwd_last, &hand_fwd, &hand);

		font_print(0, 0, fntbuff, NULL);
		draw_sprites(&hand, 1);
		update_display(true);
	}

	if (menu == MENU_MAIN) {
		*((int*)out) = main_menu_out[index];
	} else if (menu == MENU_CDROM) {
		if (index != -1)
			*((const char**)out) = cdrom_menu_out[index];
		else
			*((const char**)out) = NULL;
	}

}

static void run_game(const char* const gamepath)
{
	extern Chip8Key chip8_keys;
	extern CHIP8_GFX_TYPE chip8_gfx[CHIP8_GFX_HEIGHT][CHIP8_GFX_WIDTH];

	const uint32_t usecs_per_step = 1000000u / chip8_freq;

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
	const char* cdrom_menu_ret;
	enum Menu main_menu_ret;

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
		run_menu(MENU_MAIN, &main_menu_ret);
		switch (main_menu_ret) {
		case MENU_CDROM:
			run_menu(MENU_CDROM, &cdrom_menu_ret);
			if (cdrom_menu_ret != NULL)
				run_game(cdrom_menu_ret);
			break;
		default:
			break;
		}
	}
}


