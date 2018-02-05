#ifndef PSCHIP8_SYSTEM_H_ /* PSCHIP8_SYSTEM_H_ */
#define PSCHIP8_SYSTEM_H_ 
#include <stdio.h>
#include <stdlib.h>
#ifdef DEBUG /* DEBUG */
#include <assert.h>
#endif /* DEBUG */
#include <gsKit.h>


typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed short       int16_t;
typedef signed char        int8_t;
typedef signed int         int32_t;
typedef uint8_t            bool;
#define true  ((bool)1)
#define false ((bool)0)


#define MALLOC malloc
#define FREE   free


#define SCREEN_WIDTH  (640)
#define SCREEN_HEIGHT (448)


#define LOGINFO(...)  {           \
	printf("INFO: " __VA_ARGS__); \
	putchar('\n');                \
}

#define LOGERROR(...) {            \
	printf("ERROR: " __VA_ARGS__); \
	putchar('\n');                 \
}

#define FATALERROR(...) {                    \
	printf("FATAL FAILURE: " __VA_ARGS__);   \
	putchar('\n');                           \
	abort();                                 \
}

#ifdef DEBUG /* DEBUG */

#define LOGDEBUG(...) {            \
	printf("DEBUG: " __VA_ARGS__); \
	putchar("\n");                 \
}

#define ASSERT_MSG(cond, ...) {  \
	if (!(cond))                 \
		FATALERROR(__VA_ARGS__); \
}

#else

#define LOGDEBUG(...)   ((void)0)
#define ASSERT_MSG(...) ((void)0)

#endif /* DEBUG */

/* chip8 settings */
#define CHIP8_GFX_BGC (GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00))
#define CHIP8_GFX_FGC (GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x00, 0x00))
typedef uint64_t chip8_gfx_t;


struct vec2 {
	int16_t x, y;
};

struct sprite {
	struct vec2 spos;
	struct vec2 size;
	struct vec2 tpos;
};


void init_system(void);
void reset_timers(void);
void update_timers(void);
void update_display(bool vsync);
void draw_ram_buffer(void* pixels, const struct vec2* pos,
                     const struct vec2* size, uint8_t scale);
void load_files(const char* const* filenames, void** dsts, short nfiles);


static inline uint32_t get_msec(void)
{
	extern uint32_t sys_msec_timer;
	return sys_msec_timer;
}

static inline uint32_t get_usec(void)
{
	extern uint32_t sys_usec_timer;
	return sys_usec_timer;
}

static inline uint32_t get_msec_now(void)
{
	extern uint32_t sys_msec_timer;
	update_timers();
	return sys_msec_timer;
}

static inline uint32_t get_usec_now(void)
{
	extern uint32_t sys_usec_timer;
	update_timers();
	return sys_usec_timer;
}




#endif /* PSCHIP8_SYSTEM_H_ */
