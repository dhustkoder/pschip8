#include <string.h>
#include "system.h"
#include "chip8.h"


enum Chan {
	CHAN_HNDMOVE,
	CHAN_HNDBACK,
	CHAN_HNDCLICK
};

enum Snd {
	SND_HNDMOVE,
	SND_HNDBACK,
	SND_HNDCLICK
};

enum MainMenuOpt {
	MAINMENUOPT_GAMES,
	#if defined(PLATFORM_SDL2)
	MAINMENUOPT_EXIT,
	#endif
	MAINMENUOPT_NOPTS
};

enum MenuSprites {
	MENUSPRT_HAND,
	MENUSPRT_CIRCLE,
	MENUSPRT_CROSS 
};

static struct sprite menu_sprites[] = {
	[MENUSPRT_HAND] = {
		.size = { 26, 14  },
		.tpos = { 0,  0   }
	},
	[MENUSPRT_CIRCLE] = {
		.size = { 16, 16 },
		.spos = { 8,  SCREEN_HEIGHT - 24  },
		.tpos = { 42, 0  }
	},
	[MENUSPRT_CROSS] = {
		.size = { 16, 16 },
		.spos = { 64, SCREEN_HEIGHT - 24 },
		.tpos = { 26, 0  }
	}
};

static button_t button_tbl[] = {
	BUTTON_DOWN,   BUTTON_L1,     BUTTON_UP, BUTTON_R1,
	BUTTON_LEFT,   BUTTON_CROSS,  BUTTON_RIGHT,
	BUTTON_L2,     BUTTON_DOWN,   BUTTON_R2,
	BUTTON_START,  BUTTON_SELECT, BUTTON_SQUARE,
	BUTTON_CIRCLE, BUTTON_TRIANGLE
};


static void sprite_animation_update(const uint8_t fwd_div,
                                    const uint8_t mov_div,
                                    uint32_t* const msec_last,
                                    uint32_t* const fwd_last,
                                    bool* const fwd,
                                    struct sprite* const sprite)
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

static int8_t run_select_menu(const char* const title,
                              const char* const* opts,
                              const int8_t nopts,
                              const bool exit_on_cross)
{
	const struct sprite* const circle = &menu_sprites[MENUSPRT_CIRCLE];
	const struct sprite* const cross  = &menu_sprites[MENUSPRT_CROSS];
	const struct vec2 title_pos = {
		.x = (SCREEN_WIDTH - (6 * strlen(title))) / 2u,
		.y = 16
	};
	const struct vec2 select_pos = {
		.x = circle->spos.x + circle->size.x,
		.y = circle->spos.y + ((circle->size.y - 8) / 2)
	};
	const struct vec2 back_pos = {
		.x = cross->spos.x + cross->size.x,
		.y = cross->spos.y + ((cross->size.y - 8) / 2)
	};

	struct sprite* const hand = &menu_sprites[MENUSPRT_HAND];
	uint32_t hand_anim_last = 0;
	uint32_t hand_fwd_last = 0;
	bool hand_fwd = true;

	uint16_t pad = get_paddata();
	uint16_t pad_old = pad;
	int8_t index = 0;
	int8_t index_old = index;

	struct vec2 opts_pos[nopts];
	int8_t i;

	for (i = 0; i < nopts; ++i) {
		opts_pos[i].x = (SCREEN_WIDTH - 6 * strlen(opts[i])) / 2;
		opts_pos[i].y = 32 + i * 14;
	}

	hand->spos.x = opts_pos[index].x - 32;
	hand->spos.y = opts_pos[index].y;

	for (;;) {
		pad = get_paddata();

		if (pad != pad_old) {
			if (exit_on_cross && pad&BUTTON_CROSS &&
			   !(pad_old&BUTTON_CROSS)) {
				index = -1;
				enable_chan(CHAN_HNDBACK);
				break;
			}

			if (pad&BUTTON_CIRCLE && !(pad_old&BUTTON_CIRCLE)) {
				enable_chan(CHAN_HNDCLICK);
				break;
			}

			if (pad&BUTTON_DOWN && !(pad_old&BUTTON_DOWN) &&
			    index < (nopts - 1)) {
				++index;
			} else if (pad&BUTTON_UP && !(pad_old&BUTTON_UP) &&
			         index > 0) {
				--index;
			}

			if (index != index_old) {
				enable_chan(CHAN_HNDMOVE);
				hand->spos.x = opts_pos[index].x - 32;
				hand->spos.y = opts_pos[index].y;
				index_old = index;
			}

			pad_old = pad;
		}

		sprite_animation_update(4u, 8u, &hand_anim_last,
		                        &hand_fwd_last, &hand_fwd, hand);

		font_print(&title_pos, title, NULL);
		for (i = 0; i < nopts; ++i)
		       font_print(&opts_pos[i], opts[i], NULL);

		font_print(&select_pos, "SELECT", NULL);

		if (exit_on_cross) {
			font_print(&back_pos, "BACK", NULL);
			draw_sprites(menu_sprites, 3);
		} else {
			draw_sprites(menu_sprites, 2);
		}

		update_display();
	}

	return index;
}

static enum MainMenuOpt main_menu(void)
{
	const char* const opts[MAINMENUOPT_NOPTS] = {
		"Games"
		#if defined(PLATFORM_SDL2)
		,
		"Exit"
		#endif
	};
	const enum MainMenuOpt out[MAINMENUOPT_NOPTS] = {
		MAINMENUOPT_GAMES,
		#if defined(PLATFORM_SDL2)
		MAINMENUOPT_EXIT
		#endif
	};

	const uint8_t index =
		run_select_menu("- PSCHIP8 -", opts,
				MAINMENUOPT_NOPTS, false);

	return out[index];
}

static const char* games_menu(void)
{
	const char* const opts[] = {
		"Brix", "Missile", "Tank", "Pong 2",
		"Maze", "Invaders", "UFO", "Tetris",
		"Blinky"
	};
	const char* const out[] = {
		"BRIX.CH8", "MISSILE.CH8", "TANK.CH8", "PONG2.CH8",
		"MAZE.CH8", "INVADERS.CH8", "UFO.CH8", "TETRIS.CH8",
		"BLINKY.CH8"
	};
	const int8_t nopts = sizeof(opts) / sizeof(opts[0]);
	const int8_t index = run_select_menu("- GAMES -", opts, nopts, true);
	return index != -1 ? out[index] : NULL;
}

static void run_game(const char* const gamepath)
{
	extern chip8_gfx_t chip8_gfx[CHIP8_GFX_HEIGHT][CHIP8_GFX_WIDTH];
	extern chip8key_t chip8_keys;
	extern bool chip8_draw_flag;

	const struct vec2 pos = { (SCREEN_WIDTH / 2), (SCREEN_HEIGHT / 2) };
	const struct vec2 size = { CHIP8_GFX_WIDTH, CHIP8_GFX_HEIGHT };

	uint32_t timer = 0;
	uint32_t last_sec = 0;
	int steps = 0;
	int steps_cnt = 0;
	int fps = 0;
	int fps_cnt = 0;
	int steps_per_frame = 0;
	const void* const varpack[] = { &fps, &steps };
	button_t pad_old = 0;
	button_t pad;
	int i;

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

		timer = get_msec();
		for (i = 0; i < steps_per_frame; ++i) {
			chip8_step();
			++steps_cnt;
		}

		font_print(&(struct vec2){ 8, 8 },
		           "Press START & SELECT to reset\n"
		           "Frames per second: %d\n"
		           "Steps per second: %d", varpack);

		if (chip8_draw_flag)
			load_ram_buffer(chip8_gfx, &pos, &size, 3);

		draw_ram_buffer();
		update_display();

		++fps_cnt;
		if ((timer - last_sec) >= 1000u) {
			steps = steps_cnt;
			fps = fps_cnt;
			steps_per_frame = CHIP8_FREQ / fps;
			steps_cnt = 0;
			fps_cnt = 0;
			last_sec = timer;
		}
	}
}

void pschip8()
{
	{
		enum Files { 
			BKG, FONT, SPRITES, HNDMOVESND, HNDBACKSND, HNDCLICKSND,
			NFILES
		};

		const char* const cdpaths[NFILES] = {
			"WIZTOWER.BKG", "FONT3.SPR", "SPRITES1.SPR",
			"HNDMOVE.SND", "HNDBACK.SND", "HNDCLICK.SND"
		};

		void* data[NFILES] = { NULL, NULL, NULL, NULL, NULL, NULL };
		int i;

		load_files(cdpaths, data, NFILES);
		load_bkg(data[BKG]);
		load_font(data[FONT], &(struct vec2){6, 8}, 32, 256);
		load_sprite_sheet(data[SPRITES], 32);
		load_snd(&data[HNDMOVESND], 3);
		load_sync();
		for (i = 0; i < NFILES; ++i)
			FREE(data[i]);
	}

	assign_snd_chan(CHAN_HNDMOVE, SND_HNDMOVE);
	assign_snd_chan(CHAN_HNDCLICK, SND_HNDCLICK);
	assign_snd_chan(CHAN_HNDBACK, SND_HNDBACK);

	reset_timers();
	for (;;) {
		switch (main_menu()) {
			case MAINMENUOPT_GAMES: {
				const char* const gamepath = games_menu();
				if (gamepath != NULL)
					run_game(gamepath);
				break;
			}
			#if defined(PLATFORM_SDL2)
			case MAINMENUOPT_EXIT:
				return;
				break;
			#endif
			default:
				break;
		}
	}
}

