#include "system.h"
#include "sprite.h"


static unsigned long ot[256];
static unsigned long tpage_id;
static RECT spritesheet_rect;

void load_sprite_sheet(const void* const spritesheet)
{
	spritesheet_rect = (RECT){
		.x = SCREEN_WIDTH,
		.y = 0,
		.w = ((uint16_t*)spritesheet)[0],
		.h = ((uint16_t*)spritesheet)[1]
	};

	tpage_id = LoadTPage((void*)(((uint32_t*)spritesheet) + 1),
	                     2, 0,
	                     spritesheet_rect.x, spritesheet_rect.y,
	                     spritesheet_rect.w, spritesheet_rect.h);
}


void draw_sprites(const Sprite* const sprites, short size)
{
	short i;
	SPRT sprt;
	DR_TPAGE tpage;
	ClearOTag(ot, sizeof(ot) / sizeof(ot[0]));
	for (i = 0; i < size; ++i) {
		if (sprites[i].scrpos.x > 0 && sprites[i].scrpos.y > 0) {
			SetSprt(&sprt);
			sprt.w = sprites[i].size.w;
			sprt.h = sprites[i].size.h;
			sprt.x0 = sprites[i].scrpos.x;
			sprt.y0 = sprites[i].scrpos.y;
			sprt.u0 = sprites[i].sheetpos.x;
			sprt.v0 = sprites[i].sheetpos.y;
			sprt.r0 = sprt.g0 = sprt.b0 = 127;
			AddPrim(ot, &sprt);
		}
	}
	SetDrawTPage(&tpage, 0, 0, tpage_id);
	AddPrim(ot, &tpage);
	DrawOTag(ot);
	DrawSync(0);
}



