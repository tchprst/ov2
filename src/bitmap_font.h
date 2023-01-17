#ifndef OV2_BITMAP_FONT_H
#define OV2_BITMAP_FONT_H

#include "parse.h"

void render_bitmap_font(
	struct bitmap_font* bitmap_font,
	char const* text,
	float x,
	float y
);

#endif /*OV2_BITMAP_FONT_H*/
