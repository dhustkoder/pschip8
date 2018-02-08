#ifndef PSCHIP8_SYSTEM_H_ /* PSCHIP8_SYSTEM_H_ */
#define PSCHIP8_SYSTEM_H_
#include <sys/errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <libapi.h>
#include <libgte.h>
#include <libgpu.h>
#include <libspu.h>


typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef uint8_t bool;
#define false ((bool)0)
#define true  ((bool)1)


#define MALLOC  malloc3
#define REALLOC realloc3
#define FREE    free3

#define SCREEN_WIDTH   (320)
#ifdef DISPLAY_TYPE_PAL
#define SCREEN_HEIGHT  (256)
#define NSEC_PER_HSYNC (64000u)
#else
#define SCREEN_HEIGHT  (240)
#define NSEC_PER_HSYNC (63560u)
#endif


#define LOGINFO(...)  {               \
	printf("INFO: " __VA_ARGS__); \
	putchar('\n');                \
}

#define LOGERROR(...) {                \
	printf("ERROR: " __VA_ARGS__); \
	putchar('\n');                 \
}

#define FATALERROR(...) {                        \
	printf("FATAL FAILURE: " __VA_ARGS__);   \
	putchar('\n');                           \
	SystemError(_get_errno(), _get_errno()); \
}

#ifdef DEBUG /* DEBUG */

#define LOGDEBUG(...) {                \
	printf("DEBUG: " __VA_ARGS__); \
	putchar("\n");                 \
}

#define ASSERT_MSG(cond, ...) {          \
	if (!(cond))                     \
		FATALERROR(__VA_ARGS__); \
}

#else

#define LOGDEBUG(...)   ((void)0)
#define ASSERT_MSG(...) ((void)0)

#endif /* DEBUG */

#define MAX_VOLUME (0x3FFF)

/* chip8 settings */
#define CHIP8_FREQ        (512)
#define CHIP8_DELAY_FREQ  (120)
#define CHIP8_GFX_WIDTH   (68)
#define CHIP8_GFX_HEIGHT  (34)
#define CHIP8_GFX_BGC     (0x8000)
#define CHIP8_GFX_FGC     (0xFFFF)
#define CHIP8_GFX_TYPE    uint16_t


typedef uint16_t button_t;
enum Button {
	BUTTON_LEFT     = 0x8000,
	BUTTON_DOWN     = 0x4000,
	BUTTON_RIGHT    = 0x2000,
	BUTTON_UP       = 0x1000,
	BUTTON_START    = 0x0800,
	BUTTON_SELECT   = 0x0100,
	BUTTON_SQUARE   = 0x0080,
	BUTTON_CROSS    = 0x0040,
	BUTTON_CIRCLE   = 0x0020,
	BUTTON_TRIANGLE = 0x0010,
	BUTTON_R1       = 0x0008,
	BUTTON_L1       = 0x0004,
	BUTTON_R2       = 0x0002,
	BUTTON_L2       = 0x0001
};


struct vec2 {
	int16_t x, y;
};

struct sprite {
	struct vec2 spos;
	struct vec2 size;
	struct vec2  tpos;
};


void init_system(void);
void reset_timers(void);
void update_timers(void);
void update_display(void);

void font_print(const struct vec2* pos, const char* fmt, const void* const* varpack);
void draw_sprites(const struct sprite* sprites, short nsprites);
void draw_ram_buffer(void* pixels, const struct vec2* pos,
                     const struct vec2* size, uint8_t scale);
void assign_snd_chan(uint8_t chan, uint8_t snd_index);
void load_sprite_sheet(void* data, short max_sprites_on_screen);
void load_bkg(void* data);
void load_font(void* data, const struct vec2* charsize,
               uint8_t ascii_idx, short max_chars_on_scr);
void load_snd(void* const* data, uint8_t nsnd);
void load_files(const char* const* filenames, void** dsts, short nfiles);


static inline void set_chan_volume(const uint8_t chan, const uint16_t vol)
{
	SpuSetVoiceVolume(chan, vol, vol);
}

static inline void enable_chan(const uint8_t chan)
{
	SpuSetKey(SPU_ON, SPU_VOICECH(chan));
}

static inline void disable_chan(const uint8_t chan)
{
	SpuSetKey(SPU_OFF, SPU_VOICECH(chan));	
}

static inline void load_sync(void)
{
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT);
	DrawSync(0);
}

static inline uint16_t get_paddata(void)
{
	extern uint16_t sys_paddata;
	return sys_paddata;
}

/* update_timers() is called in the VSync callback function
 * msec counter resets every 4294967 seconds
 * usec counter resets every 4294 seconds
 */

/* msec counter at last call to update_timers() */
static inline uint32_t get_msec(void)
{
	extern uint32_t sys_msec_timer;
	return sys_msec_timer;
}

/* msec counter at last call to update_timers() */
static inline uint32_t get_usec(void)
{
	extern uint32_t sys_usec_timer;
	return sys_usec_timer;
}

/* msec at this precise moment */
static inline uint32_t get_msec_now(void)
{
	update_timers();
	return get_msec();
}

/* usec at this precise moment */
static inline uint32_t get_usec_now(void)
{
	update_timers();
	return get_usec();
}


#endif /* PSCHIP8_SYSTEM_H_ */
