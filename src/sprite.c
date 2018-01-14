#include "system.h"
#include "sprite.h"


void load_sprite_sheet(const void* const spritesheet)
{
	RECT vram_rect = {
		.x = SCREEN_WIDTH,
		.y = 0,
		.w = ((uint16_t*)spritesheet)[0],
		.h = ((uint16_t*)spritesheet)[1]
	};

	LoadImage(&vram_rect, (void*) (((uint32_t*)spritesheet) + 1));
}


void draw_sprites(const Sprite* const sprites, short size)
{
	extern const DRAWENV* sys_curr_drawenv;

	short i;
	RECT src_rect;
	for (i = 0; i < size; ++i) {
		if (sprites[i].scrpos.x > 0 && sprites[i].scrpos.y > 0) {
			src_rect.x = SCREEN_WIDTH + sprites[i].sheetpos.x;
			src_rect.y = sprites[i].sheetpos.y;
			src_rect.w = sprites[i].size.w;
			src_rect.h = sprites[i].size.h;
			MoveImage(&src_rect,
			          sprites[i].scrpos.x + sys_curr_drawenv->clip.x,
			          sprites[i].scrpos.y + sys_curr_drawenv->clip.y);
		}
	}
}



