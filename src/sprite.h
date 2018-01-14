#ifndef PSCHIP8_SPRITE_H_
#define PSCHIP8_SPRITE_H_


typedef struct Sprite {
	struct { short x, y; } sheetpos;
	struct { short x, y; } scrpos; // -1 means off
	struct { short w, h; } size;
} Sprite;


void load_sprite_sheet(const void* spritesheet);
void draw_sprites(const Sprite* sprites, short size);


#endif
