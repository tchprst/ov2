#include <GL/gl.h>
#include <SDL2/SDL.h>
#include "ui.h"

struct frect {
    float x, y, w, h;
};

static void render_texture(struct game_state const* state, GLuint texture, struct frect* dstrect) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(dstrect->x, dstrect->y);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(dstrect->x + dstrect->w, dstrect->y);

	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(dstrect->x + dstrect->w, dstrect->y + dstrect->h);

	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(dstrect->x, dstrect->y + dstrect->h);

	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

static void render_texture_slice(
	struct game_state const* state,
	GLuint texture,
	struct frect* srcrect,
	struct frect* dstrect
) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);

	glTexCoord2f(srcrect->x, srcrect->y);
	glVertex2f(dstrect->x, dstrect->y);

	glTexCoord2f(srcrect->x + srcrect->w, srcrect->y);
	glVertex2f(dstrect->x + dstrect->w, dstrect->y);

	glTexCoord2f(srcrect->x + srcrect->w, srcrect->y + srcrect->h);
	glVertex2f(dstrect->x + dstrect->w, dstrect->y + dstrect->h);

	glTexCoord2f(srcrect->x, srcrect->y + srcrect->h);
	glVertex2f(dstrect->x, dstrect->y + dstrect->h);

	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void render_topbar(struct game_state const* state) {
	render_texture(state, state->topbar_bg_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 1088.0f, .h = 100.0f
	});

	/* region production */
	/* Button */
	render_texture_slice(state, state->topbar_button_production_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 6.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* region TODO */
	int mouseX, mouseY;
	uint32_t mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
	if (mouseX >= 6 && mouseX < 6 + 124 && mouseY >= 6 && mouseY < 6 + 72) {
		/* TODO: This is not accurate */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_QUADS);
		if (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			glColor4f(0.0f, 0.0f, 0.0f, 0.10f);
		} else {
			glColor4f(1.0f, 1.0f, 1.0f, 0.10f);
		}
		glVertex2f(6.0f, 6.0f);
		glVertex2f(6.0f + 124.0f, 6.0f);
		glVertex2f(6.0f + 124.0f, 6.0f + 72.0f);
		glVertex2f(6.0f, 6.0f + 72.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnd();
		glDisable(GL_BLEND);
	}
	/* render_texture_slice(state, state->topbar_button_production_texture, &(struct frect) {
		.x = 0.5f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 6.0f, .y = 6.0f, .w = 124, .h = 72
	});*/
	/* endregion */
	/* Button icon */
	render_texture(state, state->topbar_production_texture, &(struct frect) {
		.x = 7.0f, .y = 2.0f, .w = 24.0f, .h = 24.0f
	});
	/* Button title */
	/* TODO: Render text */
	/* Top 5 */
	/* TODO: The resource images are really blurry */
	render_texture_slice(state, state->resources_texture, &(struct frect) {
		.x = 35.0f / 49.0f, .y = 0.0f, .w = 1.0f / 49.0f, .h = 1.0f
	}, &(struct frect) {
		.x = 17.0f, .y = 29.0f, .w = 20.0f, .h = 16.0f
	});
	render_texture_slice(state, state->resources_texture, &(struct frect) {
		.x = 32.0f / 49.0f, .y = 0.0f, .w = 1.0f / 49.0f, .h = 1.0f
	}, &(struct frect) {
		.x = 37.0f, .y = 29.0f, .w = 20.0f, .h = 16.0f
	});
	render_texture_slice(state, state->resources_texture, &(struct frect) {
		.x = 33.0f / 49.0f, .y = 0.0f, .w = 1.0f / 49.0f, .h = 1.0f
	}, &(struct frect) {
		.x = 57.0f, .y = 29.0f, .w = 20.0f, .h = 16.0f
	});
	render_texture_slice(state, state->resources_texture, &(struct frect) {
		.x = 36.0f / 49.0f, .y = 0.0f, .w = 1.0f / 49.0f, .h = 1.0f
	}, &(struct frect) {
		.x = 77.0f, .y = 29.0f, .w = 20.0f, .h = 16.0f
	});
	/* Building factories */
	render_texture_slice(state, state->topbar_alert_building_texture, &(struct frect) {
		.x = 0.5f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 16.0f, .y = 49.0f, .w = 28.0f, .h = 28.0f
	});
	/* Closed factories */
	render_texture_slice(state, state->topbar_alert_factoryclosed_texture, &(struct frect) {
		.x = 0.5f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 52.0f, .y = 49.0f, .w = 28.0f, .h = 28.0f
	});
	/* Unemployed workers */
	render_texture_slice(state, state->topbar_alert_unemployed_texture, &(struct frect) {
		.x = 0.5f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 89.0f, .y = 49.0f, .w = 28.0f, .h = 28.0f
	});
	/* endregion */

	/* region budget */
	/* Button */
	render_texture_slice(state, state->topbar_button_budget_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 130.0f + 4.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* Button icon */
	render_texture(state, state->topbar_budget_texture, &(struct frect) {
		.x = 134.0f, .y = 3.0f, .w = 24.0f, .h = 24.0f
	});
	/* Button title */
	/* TODO */
	/* Graph */
	render_texture(state, state->topbar_graph_bg_texture, &(struct frect) {
		.x = 147.0f, .y = 25.0f, .w = 99.0f, .h = 29.0f
	});
	/* Treasury */
	/* TODO: Text + icons */
	/* endregion */

	/* region technology */
	/* Button */
	render_texture_slice(state, state->topbar_button_tech_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 261.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* Button icon */
	render_texture(state, state->topbar_tech_texture, &(struct frect) {
		.x = 261.0f, .y = 4.0f, .w = 24.0f, .h = 24.0f
	});
	/* Button title */
	/* TODO */
	/* Research status */
	/* TODO */
	/* Literacy stats */
	/* TODO */
	/* endregion */

	/* region politics */
	/* Button */
	render_texture_slice(state, state->topbar_button_politics_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 388.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* Button icon */
	/* TODO: Correct icon placement */
	render_texture(state, state->topbar_politics_texture, &(struct frect) {
		.x = 388.0f, .y = 4.0f, .w = 24.0f, .h = 24.0f
	});
	/* endregion */

	/* region population */
	/* Button */
	render_texture_slice(state, state->topbar_button_pops_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 515.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* Button icon */
	/* TODO: Correct icon placement */
	render_texture(state, state->topbar_pops_texture, &(struct frect) {
		.x = 515.0f, .y = 4.0f, .w = 24.0f, .h = 24.0f
	});
	/* endregion */

	/* region trade */
	/* Button */
	render_texture_slice(state, state->topbar_button_trade_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 642.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* Button icon */
	/* TODO: Correct icon placement */
	render_texture(state, state->topbar_trade_texture, &(struct frect) {
		.x = 642.0f, .y = 4.0f, .w = 24.0f, .h = 24.0f
	});
	/* endregion */

	/* region diplomacy */
	/* Button */
	render_texture_slice(state, state->topbar_button_diplo_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 769.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* Button icon */
	/* TODO: Correct icon placement */
	render_texture(state, state->topbar_diplomacy_texture, &(struct frect) {
		.x = 769.0f, .y = 4.0f, .w = 24.0f, .h = 24.0f
	});
	/* endregion */

	/* region military */
	/* Button */
	render_texture_slice(state, state->topbar_button_military_texture, &(struct frect) {
		.x = 0.0f, .y = 0.0f, .w = 0.5f, .h = 1.0f
	}, &(struct frect) {
		.x = 896.0f, .y = 6.0f, .w = 124, .h = 72
	});
	/* Button icon */
	/* TODO: Correct icon placement */
	render_texture(state, state->topbar_military_texture, &(struct frect) {
		.x = 896.0f, .y = 4.0f, .w = 24.0f, .h = 24.0f
	});
	/* endregion */
}