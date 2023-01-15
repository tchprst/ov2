#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <SOIL/SOIL.h>
#include <SDL2/SDL_mouse.h>
#include "ui.h"

struct frect {
	float x, y, w, h;
};

static struct gui_defs* find_gui(struct gui_defs* defs, const char* name) {
	for (; defs != NULL; defs = defs->next) {
		if (strcmp(defs->name, name) == 0) {
			return defs;
		}
	}
	return NULL;
}

static struct sprite_defs* find_sprite(struct sprite_defs* defs, const char* name) {
	for (; defs != NULL; defs = defs->next) {
		if (strcmp(defs->name, name) == 0) {
			return defs;
		}
	}
	return NULL;
}

/* region sprites */
static void replace_slashes(char* c) {
	for (; *c != '\0'; c++) {
		if (*c == '\\') {
			*c = '/';
		}
	}
}

static void replace_tga_extensions_with_dds(char* c) {
	for (; *c != '\0'; c++) {
		if (*c == '.' && *(c + 1) == 't' && *(c + 2) == 'g' && *(c + 3) == 'a') {
			*(c + 1) = 'd';
			*(c + 2) = 'd';
			*(c + 3) = 's';
		}
	}
}

static GLuint load_texture(char const* path) {
	char* path_with_forward_slashes = strdup(path);
	replace_slashes(path_with_forward_slashes);
	GLuint id;
	if ((id = SOIL_load_OGL_texture(
		path_with_forward_slashes,
		SOIL_LOAD_RGBA,
		SOIL_CREATE_NEW_ID,
		0
	)) == 0) {
		replace_tga_extensions_with_dds(path_with_forward_slashes);
		if ((id = SOIL_load_OGL_texture(
			path_with_forward_slashes,
			SOIL_LOAD_RGBA,
			SOIL_CREATE_NEW_ID,
			0
		)) == 0) {
			fprintf(stderr, "SOIL loading error while loading texture %s: %s\n", path_with_forward_slashes,
				SOIL_last_result());
		}
	}
	free(path_with_forward_slashes);
	return id;
}

struct loaded_texture {
	char const* name;
	GLuint texture;
	struct loaded_texture* next;
};

static struct loaded_texture* loaded_textures = NULL;

static GLuint find_texture(const char* name) {
	for (; loaded_textures != NULL; loaded_textures = loaded_textures->next) {
		if (strcmp(loaded_textures->name, name) == 0) {
			return loaded_textures->texture;
		}
	}
	return 0;
}

static GLuint find_or_load_texture(char const* name) {
	GLuint texture = find_texture(name);
	if (texture == 0) {
		struct loaded_texture* loaded = malloc(sizeof(struct loaded_texture));
		loaded->name = name;
		loaded->texture = load_texture(name);
		loaded->next = loaded_textures;
		loaded_textures = loaded;
		texture = loaded->texture;
	}
}

static void render_texture(
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

static void render_sprite(struct game_state const* state, struct sprite_defs* sprite, uint64_t frame, struct frect* dstrect);
static void render_default_sprite(struct game_state const* state, struct sprite_defs* sprite, uint64_t frame, struct frect* dstrect);
static void render_line_chart(struct game_state const* state, struct sprite_defs* sprite, uint64_t frame, struct frect* dstrect);

static void render_sprite(struct game_state const* state, struct sprite_defs* sprite, uint64_t frame, struct frect* dstrect) {
	switch (sprite->type) {
	case TYPE_SPRITE:
		render_default_sprite(state, sprite, frame, dstrect);
		break;
	case TYPE_LINE_CHART:
		break;
	case TYPE_MASKED_SHIELD:
		break;
	case TYPE_PROGRESS_BAR:
		break;
	case TYPE_CORNERED_TILE_SPRITE:
		break;
	case TYPE_TEXT_SPRITE:
		break;
	case TYPE_BAR_CHART:
		break;
	case TYPE_PIE_CHART:
		break;
	case TYPE_TILE_SPRITE:
		break;
	case TYPE_SCROLLING_SPRITE:
		break;
	default:
		assert(0);
	}
}

static void render_default_sprite(struct game_state const* state, struct sprite_defs* sprite, uint64_t frame, struct frect* dstrect) {
	GLint width, height;
	GLuint texture = find_texture(sprite->sprite.texture_file);
	if (texture == 0) {
		struct loaded_texture* loaded = malloc(sizeof(struct loaded_texture));
		loaded->name = sprite->sprite.texture_file;
		loaded->texture = load_texture(sprite->sprite.texture_file);
		loaded->next = loaded_textures;
		loaded_textures = loaded;
		texture = loaded->texture;
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	dstrect->w = (float) width / (sprite->sprite.no_of_frames > 1 ? (float) sprite->sprite.no_of_frames : 1.0f);
	if (dstrect->h == 0) dstrect->h = (float) height;
	if (sprite->sprite.no_of_frames > 1) {
		float frame_width = 1.0f / (float)sprite->sprite.no_of_frames;
		float frame_x = frame_width * (float)frame;
		struct frect srcrect = { frame_x, 0.0f, frame_width, 1.0f };
		render_texture(state, texture, &srcrect, dstrect);
	} else {
		render_texture(state, texture, &(struct frect) { 0, 0, 1.0f, 1.0f }, dstrect);
	}
}

static void render_line_chart(struct game_state const* state, struct sprite_defs* sprite, uint64_t frame, struct frect* dstrect) {
	fprintf(stderr, "render_line_chart not implemented\n");
}
/* endregion */

/* region gui */

static void render_gui(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_window(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_icon(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_button(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_text_box(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_instant_text_box(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void
render_overlapping_elements_box(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_scrollbar(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_checkbox(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_edit_box(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_list_box(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_eu3_dialog(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_shield(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

static void render_position(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent);

void ui_render(struct game_state const* state, char const* name) {
	struct gui_defs* gui = find_gui(state->gui_defs, name);
	if (gui == NULL) {
		fprintf(stderr, "Could not find gui %s\n", name);
	} else {
		render_gui(state, gui, NULL);
	}
}

static void render_gui(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent) {
	switch (gui->type) {
	case TYPE_WINDOW:
		render_window(state, gui, parent);
		break;
	case TYPE_ICON:
		render_icon(state, gui, parent);
		break;
	case TYPE_BUTTON:
		render_button(state, gui, parent);
		break;
	/*case TYPE_TEXT_BOX:
		fprintf(stderr, "TODO: render text box\n");
		break;
	case TYPE_INSTANT_TEXT_BOX:
		fprintf(stderr, "TODO: render instant text box\n");
		break;
	case TYPE_OVERLAPPING_ELEMENTS_BOX:
		fprintf(stderr, "TODO: render overlapping elements box\n");
		break;
	case TYPE_SCROLLBAR:
		fprintf(stderr, "TODO: render scrollbar\n");
		break;
	case TYPE_CHECKBOX:
		fprintf(stderr, "TODO: render checkbox\n");
		break;
	case TYPE_EDIT_BOX:
		fprintf(stderr, "TODO: render edit box\n");
		break;
	case TYPE_LIST_BOX:
		fprintf(stderr, "TODO: render list box\n");
		break;
	case TYPE_EU3_DIALOG:
		fprintf(stderr, "TODO: render eu3 dialog\n");
		break;
	case TYPE_SHIELD:
		fprintf(stderr, "TODO: render shield\n");
		break;
	case TYPE_POSITION:
		fprintf(stderr, "TODO: render position\n");
		break;
	default:
		assert(0);*/
	}
}

static void render_window(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent) {
	if (gui->window.dont_render != NULL && *gui->window.dont_render != '\0') return; /* TODO: I just use this an internal hack to disable ui, but this should be implemented properly instead */
	struct gui_defs* child = gui->window.children;
	for (; child != NULL; child = child->next) {
		render_gui(state, child, gui);
	}
}

static void render_icon(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent) {
	if (gui->icon.sprite == NULL) return;

	struct sprite_defs* sprite = find_sprite(state->sprite_defs, gui->icon.sprite);
	if (sprite == NULL) {
		fprintf(stderr, "Could not find sprite %s\n", gui->icon.sprite);
	} else {
		struct frect dstrect = {
			.x = (float)gui->position.x,
			.y = (float)gui->position.y,
			.w = 0.0f,
			.h = 0.0f
		};

		/* TODO: Implement this in general */
		switch (gui->icon.orientation) {
		case ORIENTATION_LOWER_LEFT:
			break;
		case ORIENTATION_UPPER_LEFT:
			break;
		case ORIENTATION_CENTER_UP:
			break;
		case ORIENTATION_CENTER:
			break;
		case ORIENTATION_CENTER_DOWN:
			break;
		case ORIENTATION_UPPER_RIGHT:
			break;
		case ORIENTATION_LOWER_RIGHT:
			dstrect.x += (float)state->window_width;
			dstrect.y += (float)state->window_height;
			break;
		default:
			assert(0);
		}
		if(parent != NULL) {
			dstrect.x += (float)parent->position.x;
			dstrect.y += (float)parent->position.y;
		}
		render_sprite(state, sprite, gui->icon.frame, &dstrect);
	}
}

static void render_button(struct game_state const* state, struct gui_defs* gui, struct gui_defs* parent) {
	if (gui->button.quad_texture_sprite == NULL) return;
	struct sprite_defs* sprite = find_sprite(state->sprite_defs, gui->button.quad_texture_sprite);
	if (sprite == NULL) {
		fprintf(stderr, "Could not find sprite %s\n", gui->button.quad_texture_sprite);
	} else {
		/* region TODO */
		if((gui->size.x == 0 || gui->size.y == 0) && sprite->type == TYPE_SPRITE && sprite->sprite.texture_file != NULL && *sprite->sprite.texture_file != '\0') {
			GLint width, height;
			GLuint t = find_or_load_texture(sprite->sprite.texture_file);
			glBindTexture(GL_TEXTURE_2D, t);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			glBindTexture(GL_TEXTURE_2D, 0);
			if (sprite->sprite.no_of_frames > 1) {
				width /= (float) sprite->sprite.no_of_frames;
			}
			gui->size.x = width;
			gui->size.y = height;
		}
		/* endregion */

		struct frect dstrect = {
			.x = (float)gui->position.x,
			.y = (float)gui->position.y,
			.w = (float)gui->size.x,
			.h = (float)gui->size.y,
		};

		/* TODO: Implement this in general */
		switch (gui->button.orientation) {
		case ORIENTATION_LOWER_LEFT:
			break;
		case ORIENTATION_UPPER_LEFT:
			break;
		case ORIENTATION_CENTER_UP:
			break;
		case ORIENTATION_CENTER:
			break;
		case ORIENTATION_CENTER_DOWN:
			break;
		case ORIENTATION_UPPER_RIGHT:
			break;
		case ORIENTATION_LOWER_RIGHT:
			dstrect.x += (float)state->window_width;
			dstrect.y += (float)state->window_height;
			break;
		default:
			assert(0);
		}

		/*if(parent != NULL) {
			dstrect.x += (float)parent->position.x;
			dstrect.y += (float)parent->position.y;
		}*/
		render_sprite(state, sprite, 0, &dstrect);
		/* region TODO */
		int mouse_x, mouse_y;
		uint32_t mouseButtons = SDL_GetMouseState(&mouse_x, &mouse_y);
		if ((float)mouse_x >= dstrect.x
		    && (float)mouse_x < dstrect.x + dstrect.w
		    && (float)mouse_y >= dstrect.y
		    && (float)mouse_y < dstrect.y + dstrect.h) {
			/* TODO: This is not accurate */
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBegin(GL_QUADS);
			if (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				glColor4f(0.0f, 0.0f, 0.0f, 0.10f);
			} else {
				glColor4f(1.0f, 1.0f, 1.0f, 0.10f);
			}
			glVertex2f(dstrect.x, dstrect.y);
			glVertex2f(dstrect.x + dstrect.w, dstrect.y);
			glVertex2f(dstrect.x + dstrect.w, dstrect.y + dstrect.h);
			glVertex2f(dstrect.x, dstrect.y + dstrect.h);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glEnd();
			glDisable(GL_BLEND);
		}
		/* endregion */
	}
}

/* endregion */