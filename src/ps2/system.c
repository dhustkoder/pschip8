#include <sifrpc.h>
#include <gsKit.h>
#include <dmaKit.h>
#include "system.h"


/* The minimum buffers needed for single buffered rendering. */
static GSGLOBAL* gs_global;


void init_system(void)
{
	SifInitRpc(0);
	LOGINFO("Init System");
	/* graphics */
	gs_global = gsKit_init_global();

	/*  By default the gsKit_init_global() uses an autodetected interlaced field mode */
	/*  To set a new mode set these five variables for the resolution desired and */
	/*  mode desired */

	/*  Some examples */
	/*  Make sure that gs_global->Height is a factor of the mode's gs_global->DH */

	/* gs_global->Mode = GS_MODE_NTSC */
	/* gs_global->Interlace = GS_INTERLACED; */
	/* gs_global->Field = GS_FIELD; */
	/* gs_global->Width = 640; */
	/* gs_global->Height = 448; */

	/* gs_global->Mode = GS_MODE_PAL; */
	/* gs_global->Interlace = GS_INTERLACED; */
	/* gs_global->Field = GS_FIELD; */
	/* gs_global->Width = 640; */
	/* gs_global->Height = 512; */

	/* gs_global->Mode = GS_MODE_DTV_480P; */
	/* gs_global->Interlace = GS_NONINTERLACED; */
	/* gs_global->Field = GS_FRAME; */
	/* gs_global->Width = 720; */
	/* gs_global->Height = 480; */

	/* gs_global->Mode = GS_MODE_DTV_1080I; */
	/* gs_global->Interlace = GS_INTERLACED; */
	/* gs_global->Field = GS_FIELD; */
	/* gs_global->Width = 640; */
	/* gs_global->Height = 540; */
	/* gs_global->PSM = GS_PSM_CT16; */
	/* gs_global->PSMZ = GS_PSMZ_16; */
	/* gs_global->Dithering = GS_SETTING_ON; */

	/*  A width of 640 would work as well */
	/*  However a height of 720 doesn't seem to work well */
	/* gs_global->Mode = GS_MODE_DTV_720P; */
	/* gs_global->Interlace = GS_NONINTERLACED; */
	/* gs_global->Field = GS_FRAME; */
	/* gs_global->Width = 640; */
	/* gs_global->Height = 360; */
	/* gs_global->PSM = GS_PSM_CT16; */
	/* gs_global->PSMZ = GS_PSMZ_16; */

	/*  You can use these to turn off Z/Double Buffering. They are on by default. */
	/*  gs_global->DoubleBuffering = GS_SETTING_OFF; */
	/*  gs_global->ZBuffering = GS_SETTING_OFF; */

	/*  This makes things look marginally better in half-buffer mode... */
	/*  however on some CRT and all LCD, it makes a really horrible screen shake. */
	/*  Uncomment this to disable it. (It is on by default) */
	/*  gs_global->DoSubOffset = GS_SETTING_OFF; */

	gs_global->PrimAlphaEnable = GS_SETTING_ON;
	gsKit_init_screen(gs_global);
	gsKit_mode_switch(gs_global, GS_PERSISTENT);
	gsKit_clear(gs_global, GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00));
	gsKit_set_test(gs_global, GS_ZTEST_OFF);
	gsKit_mode_switch(gs_global, GS_ONESHOT);
}

void update_display(const bool vsync)
{
	static int fps = 0;
	static int i = 0;

	gsKit_prim_sprite(gs_global, i, 0, i + 10, 10, 0, GS_SETREG_RGBAQ(0x00,0x00,0xFF,0x00,0x00));

	if (++fps >= 2) {
	        fps = 0;
	        if ((++i + 10) >= gs_global->Width)
	        	i = 0;

	}

	gsKit_queue_exec(gs_global);
	/* Flip before exec to take advantage of DMA execution double buffering. */
	gsKit_sync_flip(gs_global);
}






