#ifndef OV2_UI_H
#define OV2_UI_H

#include "game_state.h"

struct frect {
    float x, y, w, h;
};

void render_texture(struct game_state const* state, GLuint texture, struct frect* dstrect);

void render_topbar(struct game_state const* game_state);

void load_ui(char const* path);

#endif //OV2_UI_H
