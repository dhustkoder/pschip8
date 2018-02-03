#include <stdio.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <debug.h>
#include <libgs.h>
#include "system.h"

#define MAX_SPRITES		1000
#define	SCREEN_WIDTH		640
#define	SCREEN_HEIGHT		448
#define GIF_PACKET_MAX		10


static short int ScreenOffsetX, ScreenOffsetY;

static GS_DRAWENV		draw_env;
static GS_DISPENV		disp_env;

static GS_GIF_PACKET		packets[GIF_PACKET_MAX];
static GS_PACKET_TABLE		giftable;


static void  InitGraphics(void)
{
	unsigned int fb_vram_addr;

	GsResetGraph(GS_INIT_RESET, GS_INTERLACED, GS_MODE_NTSC, GS_FFMD_FIELD);

	fb_vram_addr = GsVramAllocFrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, GS_PIXMODE_32);
	GsSetDefaultDrawEnv(&draw_env, GS_PIXMODE_32, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	/* Retrieve screen offset parameters. */
	ScreenOffsetX = draw_env.offset_x;
	ScreenOffsetY = draw_env.offset_y;
	GsSetDefaultDrawEnvAddress(&draw_env, fb_vram_addr);

	GsSetDefaultDisplayEnv(&disp_env, GS_PIXMODE_32, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	GsSetDefaultDisplayEnvAddress(&disp_env, fb_vram_addr);

	/* execute draw/display environment(s)  (context 1) */
	GsPutDrawEnv1(&draw_env);
	GsPutDisplayEnv1(&disp_env);

	/* Set common primitive-drawing settings (Refer to documentation on PRMODE and PRMODECONT registers). */
	GsOverridePrimAttributes(GS_DISABLE, 0, 0, 0, 0, 0, 0, 0, 0);

	/* Set transparency settings for context 1 (Refer to documentation on TEST and TEXA registers).
	 *Alpha test = enabled, pass if >= alpha reference, alpha reference = 1, fail method = no update
	 */
	GsEnableAlphaTransparency1(GS_ENABLE, GS_ALPHA_GEQUAL, 0x01, GS_ALPHA_NO_UPDATE);
	/* Enable global alpha blending */
	GsEnableAlphaBlending1(GS_ENABLE);

	/*set transparency settings for context 2 (Refer to documentation on TEST and TEXA registers).
	 *Alpha test = enabled, pass if >= alpha reference, alpha reference = 1, fail method = no update
	 */
	GsEnableAlphaTransparency2(GS_ENABLE, GS_ALPHA_GEQUAL, 0x01, GS_ALPHA_NO_UPDATE);
	/* Enable global alpha blending */
	GsEnableAlphaBlending2(GS_ENABLE);

}


void __attribute__((noreturn)) main(void)
{
	InitGraphics();
	
	giftable.packet_count = GIF_PACKET_MAX;
	giftable.packets = packets;

	for (;;) {
		/* clear the area that we are going to put the sprites/triangles/.... */
		GsGifPacketsClear(&giftable);




		GsDrawSync(0);
		GsVSync(0);
		/* clear the draw environment before we draw stuff on it */
		GsClearDrawEnv1(&draw_env);
		/* set to '1' becuse we want to wait for drawing to finish. if we dont wait we will write on packets that is currently writing to the gif */
		GsGifPacketsExecute(&giftable, 1);
	}
}

