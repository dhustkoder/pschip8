#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <debug.h>
#include <libgs.h>
#include "system.h"


void __attribute__((noreturn)) main(void)
{
	SifInitRpc(0);
	init_scr();
	for (;;) {
		scr_clear();
		scr_printf("\tuint8_t: %d\n"
		           "\tuint16_t: %d\n"
		           "\tuint32_t: %d\n"
		           "\tint8_t: %d\n"
		           "\tint16_t: %d\n"
		           "\tint32_t: %d\n",
		           sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t),
		           sizeof(int8_t), sizeof(int16_t), sizeof(int32_t));
		GsDrawSync(0);
		GsVSync(0);
	}
}

