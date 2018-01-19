#ifndef PSCHIP8_SYSTEM_H_
#define PSCHIP8_SYSTEM_H_
#include <stdio.h>
#ifdef DEBUG /* DEBUG */
#include <debug.h>
#endif /* DEBUG */


typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed short   int16_t;
typedef signed char    int8_t;
typedef signed int     int32_t;

typedef uint8_t bool;
#define true  ((bool)1)
#define false ((bool)0)

#define LOGINFO(...)  printf("INFO: " __VA_ARGS__)
#define LOGERROR(...) printf("ERROR: " __VA_ARGS__)

#define FATALERROR(...) {                        \
	LOGERROR("FATAL FAILURE: " __VA_ARGS__); \
	LOGERROR("\n");                          \
	abort();                                 \
}

#ifdef DEBUG /* DEBUG */

#define LOGDEBUG(...) printf("DEBUG: " __VA_ARGS__)

#define ASSERT_MSG(cond, ...) {              \
	if (!(cond))                         \
		FATALERROR(__VA_ARGS__);     \
}

#else

#define LOGDEBUG(...)   ((void)0)
#define ASSERT_MSG(...) ((void)0)

#endif /* DEBUG */


void init_system(void);


static inline uint32_t get_msec_now(void)
{
	return 0;
}

static inline uint32_t get_usec_now(void)
{
	return 0;
}

static inline void load_files(const char* const* filenames, void** dsts, short nfiles)
{
	/* ... */
}




#endif /* PSCHIP8_PSCHIP8_H */
