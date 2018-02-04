#include <stdio.h>
#include <kernel.h>
#include <sifrpc.h>
#include <tamtypes.h>
#include <gif_tags.h>
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma.h>
#include <dma_tags.h>
#include <draw.h>
#include <graph.h>
#include <packet.h>
#include "system.h"


/* The minimum buffers needed for single buffered rendering. */
static framebuffer_t framebuffer;
static zbuffer_t zbuffer;
static packet_t* gpu_packet;
static qword_t* gpu_packet_offset;




void init_system(void)
{
	SifInitRpc(0);
	LOGINFO("Init System");
	/* graphics */
	/* The data packet. */
	gpu_packet = packet_init(50, PACKET_NORMAL);
	/* Init GIF dma channel. */
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	/* Define a 32-bit 512x512 framebuffer. */
	framebuffer.width  = SCREEN_WIDTH;
	framebuffer.height = SCREEN_HEIGHT;
	framebuffer.mask   = 0;
	framebuffer.psm    = GS_PSM_32;

	/* Switch the mask on for some fun. */
	/*framebuffer.mask = 0xFFFF0000; */

	/* Allocate some vram for our framebuffer */
	framebuffer.address = graph_vram_allocate(framebuffer.width, framebuffer.height, framebuffer.psm, GRAPH_ALIGN_PAGE);

	/* Disable the zbuffer. */
	zbuffer.enable = 0;
	zbuffer.address = 0;
	zbuffer.mask = 0;
	zbuffer.zsm = 0;

	/* Initialize the screen and tie the framebuffer to the read circuits. */
	graph_initialize(framebuffer.address, framebuffer.width, framebuffer.height, framebuffer.psm, 0, 0);

	/* This is how you would define a custom mode */
	/*graph_set_mode(GRAPH_MODE_NONINTERLACED,GRAPH_MODE_VGA_1024_60,GRAPH_MODE_FRAME,GRAPH_DISABLE); */
	/*graph_set_screen(0,0,512,768); */
	/*graph_set_bgcolor(32,32,128);*/
	/*graph_set_framebuffer_filtered(framebuffer.address,framebuffer.width,framebuffer.psm,0,0); */
	/*graph_enable_output(); */

	/* This will setup a default drawing environment. */
	gpu_packet_offset = draw_setup_environment(gpu_packet->data, 0, &framebuffer, &zbuffer);

	/* This is where you could add various other drawing environment settings, */
	/* or keep on adding onto the packet, but I'll stop with the default setup */
	/* by appending a finish tag. */
	gpu_packet_offset = draw_finish(gpu_packet_offset);


	/* Free graphics
	 Free the vram. 
	graph_vram_free(framebuffer.address);

	Free the packet. 
	packet_free(gpu_packet);

	 Disable output and reset the GS. 
	graph_shutdown();

	 Shutdown our currently used dma channel. 
	dma_channel_shutdown(DMA_CHANNEL_GIF,0);
	*/
}

void update_display(const bool vsync)
{
	dma_channel_send_normal(DMA_CHANNEL_GIF, gpu_packet->data, gpu_packet_offset - gpu_packet->data, 0, 0);

	/* Wait until the screen is cleared. */
	draw_wait_finish();

	/* Update the screen. */
	if (vsync)
		graph_wait_vsync();

	/* Since we only have one packet, we need to wait until the dmac is done */
	/* before reusing our pointer; */
	dma_wait_fast();
	gpu_packet_offset = draw_clear(gpu_packet->data, 0, 0, 0, framebuffer.width, framebuffer.height, 32, 32, 128);
	gpu_packet_offset = draw_finish(gpu_packet_offset);
}






