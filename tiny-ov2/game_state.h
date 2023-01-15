#ifndef OV2_GAME_STATE_H
#define OV2_GAME_STATE_H

#include "parse.h"
#include <stdlib.h>
#include <GL/gl.h>

enum current_window {
	WINDOW_MAP,
	WINDOW_DIPLOMACY,
	WINDOW_MILITARY,
	WINDOW_POLITICS,
	WINDOW_POPULATION,
	WINDOW_PRODUCTION,
	WINDOW_TECHNOLOGY,
	WINDOW_TRADE,
};

/* TODO: Separate into actual game state and UI state*/
struct game_state {
	size_t province_definitions_count;
	struct province_definition* province_definitions;
	enum current_window current_window;

	float camera[3];
	float is_dragging;
	int32_t window_width;
	int32_t window_height;

	struct sprite* sprites;
	struct ui_widget* widgets;
	GLuint provinces_texture;
};

struct game_state* init_game_state(int32_t window_width, int32_t window_height);

void free_game_state(struct game_state* game_state);

#endif //OV2_GAME_STATE_H
