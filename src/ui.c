#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <SOIL/SOIL.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_ttf.h>
#include "ui.h"

static char const* const month_names[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

struct frect {
	float x, y, w, h;
};

static struct ui_widget* find_widget(struct ui_widget* widgets, const char* name) {
	for (; widgets != NULL; widgets = widgets->next) {
		if (strcmp(widgets->name, name) == 0) {
			return widgets;
		}
	}
	return NULL;
}

static struct sprite* find_sprite(struct sprite* sprites, const char* name) {
	for (; sprites != NULL; sprites = sprites->next) {
		if (strcmp(sprites->name, name) == 0) {
			return sprites;
		}
	}
	return NULL;
}

/* region sprites */
static void replace_backslashes_with_forward_slashes(char* c) {
	for (; *c != '\0'; c++) {
		if (*c == '\\') {
			*c = '/';
		}
	}
}

static void replace_tga_extension_with_dds(char* c) {
	for (; *c != '\0'; c++) {
		if (*c == '.' && *(c + 1) == 't' && *(c + 2) == 'g' && *(c + 3) == 'a' && *(c + 4) == '\0') {
			*(c + 1) = 'd';
			*(c + 2) = 'd';
			*(c + 3) = 's';
		}
	}
}

static GLuint load_texture(char const* path) {
	GLuint id;
	char* corrected_path = strdup(path);
	replace_backslashes_with_forward_slashes(corrected_path);
	if ((id = SOIL_load_OGL_texture(
		corrected_path,
		SOIL_LOAD_RGBA,
		SOIL_CREATE_NEW_ID,
		0
	)) == 0) {
		/* We retry with dds, as there are a lot of references to
		 * non-existing tga files, with an existing dds counterpart. */
		replace_tga_extension_with_dds(corrected_path);
		if ((id = SOIL_load_OGL_texture(
			corrected_path,
			SOIL_LOAD_RGBA,
			SOIL_CREATE_NEW_ID,
			0
		)) == 0) {
			fprintf(stderr, "SOIL loading error while loading "
				        "texture %s: %s\n", corrected_path,
				SOIL_last_result());
		}
	}
	free(corrected_path);
	return id;
}

struct loaded_texture {
	char const* name;
	GLuint texture;
	struct loaded_texture* next;
};

/* TODO: Let's not keep a global texture buffer like this. */
static struct loaded_texture* loaded_textures = NULL;

static GLuint find_texture(const char* name) {
	struct loaded_texture* loaded_texture = loaded_textures;
	for (; loaded_texture != NULL; loaded_texture = loaded_texture->next) {
		if (strcmp(loaded_texture->name, name) == 0) {
			return loaded_texture->texture;
		}
	}
	return 0;
}

static GLuint find_or_load_texture(char const* name) {
	GLuint texture = find_texture(name);
	if (texture == 0 && (texture = load_texture(name), texture != 0)) {
		struct loaded_texture* new_texture =
			malloc(sizeof(struct loaded_texture));
		new_texture->name = name;
		new_texture->texture = texture;
		new_texture->next = loaded_textures;
		loaded_textures = new_texture;
	}
	return texture;
}

static void render_texture(
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

static void render_sprite(struct game_state const* state, struct sprite* sprite, uint64_t frame, struct frect* dstrect);
static void render_simple_sprite(struct game_state const* state, struct sprite* sprite, uint64_t frame, struct frect* dstrect);

static void render_sprite(struct game_state const* state, struct sprite* sprite, uint64_t frame, struct frect* dstrect) {
	switch (sprite->type) {
	case TYPE_SIMPLE_SPRITE:
		render_simple_sprite(state, sprite, frame, dstrect);
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

static void render_simple_sprite(struct game_state const* state, struct sprite* sprite, uint64_t frame, struct frect* dstrect) {
	GLint width, height;
	GLuint texture = find_or_load_texture(sprite->simple_sprite.texture_file);

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	dstrect->w = (float) width / (sprite->simple_sprite.no_of_frames > 1 ? (float) sprite->simple_sprite.no_of_frames : 1.0f);
	if (dstrect->h == 0) dstrect->h = (float) height;
	if (sprite->simple_sprite.no_of_frames > 1) {
		float frame_width = 1.0f / (float)sprite->simple_sprite.no_of_frames;
		float frame_x = frame_width * (float)frame;
		struct frect srcrect = { frame_x, 0.0f, frame_width, 1.0f };
		render_texture(texture, &srcrect, dstrect);
	} else {
		render_texture(texture, &(struct frect) { 0, 0, 1.0f, 1.0f }, dstrect);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* endregion */

/* region ui */

static void render_widget(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent);
static void render_window(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent);
static void render_icon(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent);
static void render_button(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent);
static void render_text_box(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent);

static void find_and_render_widget(struct game_state const* state, char const* name) {
	struct ui_widget* widget = find_widget(state->widgets, name);
	if (widget == NULL) {
		fprintf(stderr, "Could not find widget '%s'.\n", name);
	} else {
		render_widget(state, widget, NULL);
	}
}

static void render_widget(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent) {
	switch (widget->type) {
	case TYPE_WINDOW:
		render_window(state, widget, parent);
		break;
	case TYPE_ICON:
		render_icon(state, widget, parent);
		break;
	case TYPE_BUTTON:
		render_button(state, widget, parent);
		break;
	case TYPE_TEXT_BOX:
		render_text_box(state, widget, parent);
		break;
	case TYPE_INSTANT_TEXT_BOX:
		render_text_box(state, widget, parent);
		break;
	/*case TYPE_OVERLAPPING_ELEMENTS_BOX:
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

static void render_window(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent) {
	if (widget->window.dont_render != NULL && *widget->window.dont_render != '\0') return; /* TODO: I just use this an internal hack to disable ui, but this should be implemented properly instead */
	struct ui_widget* child = widget->window.children;
	for (; child != NULL; child = child->next) {
		render_widget(state, child, widget);
	}
}

static void render_icon(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent) {
	if (widget->icon.sprite == NULL) return;

	struct sprite* sprite = find_sprite(state->sprites, widget->icon.sprite);
	if (sprite == NULL) {
		fprintf(stderr, "Could not find sprite '%s'.\n", widget->icon.sprite);
	} else {
		struct frect dstrect = {
			.x = (float)widget->position.x,
			.y = (float)widget->position.y,
			.w = 0.0f,
			.h = 0.0f
		};

		/* TODO: Lift orientation to widget and implement this in `render_widget` instead. */
		switch (widget->icon.orientation) {
		case UI_ORIENTATION_LOWER_LEFT:
			break;
		case UI_ORIENTATION_UPPER_LEFT:
			break;
		case UI_ORIENTATION_CENTER_UP:
			break;
		case UI_ORIENTATION_CENTER:
			break;
		case UI_ORIENTATION_CENTER_DOWN:
			break;
		case UI_ORIENTATION_UPPER_RIGHT:
			break;
		case UI_ORIENTATION_LOWER_RIGHT:
			dstrect.x += (float)state->window_width;
			dstrect.y += (float)state->window_height;
			break;
		default:
			assert(0);
		}
		/* TODO: Handle position chain properly. */
		if(parent != NULL) {
			dstrect.x += (float)parent->position.x;
			dstrect.y += (float)parent->position.y;
		}
		render_sprite(state, sprite, widget->icon.frame, &dstrect);
	}
}

static void render_button(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent) {
	if (widget->button.quad_texture_sprite == NULL) return;
	struct sprite* sprite = find_sprite(state->sprites, widget->button.quad_texture_sprite);
	if (sprite == NULL) {
		fprintf(stderr, "Could not find sprite %s.\n", widget->button.quad_texture_sprite);
	} else {
		/* region TODO: Figure out a proper way of handling no_of_frames/sprite texture size. */
		if((widget->size.x == 0 || widget->size.y == 0) && sprite->type == TYPE_SIMPLE_SPRITE && sprite->simple_sprite.texture_file != NULL && *sprite->simple_sprite.texture_file != '\0') {
			GLint width, height;
			GLuint t = find_or_load_texture(sprite->simple_sprite.texture_file);
			glBindTexture(GL_TEXTURE_2D, t);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			glBindTexture(GL_TEXTURE_2D, 0);
			widget->size.x = width;
			widget->size.y = height;
			if (sprite->simple_sprite.no_of_frames > 1) {
				widget->size.x /= (float) sprite->simple_sprite.no_of_frames;
			}
		}
		/* endregion */

		struct frect dstrect = {
			.x = (float)widget->position.x,
			.y = (float)widget->position.y,
			.w = (float)widget->size.x,
			.h = (float)widget->size.y,
		};

		/* TODO: Lift orientation to widget and implement this in `render_widget` instead. */
		switch (widget->button.orientation) {
		case UI_ORIENTATION_LOWER_LEFT:
			break;
		case UI_ORIENTATION_UPPER_LEFT:
			break;
		case UI_ORIENTATION_CENTER_UP:
			break;
		case UI_ORIENTATION_CENTER:
			break;
		case UI_ORIENTATION_CENTER_DOWN:
			break;
		case UI_ORIENTATION_UPPER_RIGHT:
			break;
		case UI_ORIENTATION_LOWER_RIGHT:
			dstrect.x += (float)state->window_width;
			dstrect.y += (float)state->window_height;
			break;
		default:
			assert(0);
		}

		/* TODO: Handle position chain when rendering:
		if(parent != NULL) {
			dstrect.x += (float)parent->position.x;
			dstrect.y += (float)parent->position.y;
		}*/
		render_sprite(state, sprite, widget->button.frame, &dstrect);
		/* region TODO: Handle button hover/press properly. */
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

static void render_text_box(struct game_state const* state, struct ui_widget* widget, struct ui_widget* parent) {
	if (widget->text_box.text == NULL) return;
	struct frect dstrect = {
		.x = (float) widget->position.x,
		.y = (float) widget->position.y,
		.w = (float) widget->size.x,
		.h = (float) widget->size.y,
	};

	/* TODO: Fix this font rendering thing */

	TTF_Font* font;
	SDL_Color color = { 255, 255, 255, 255 };
	if (strncmp(widget->text_box.font, "vic_18", 6) == 0) {
		font = TTF_OpenFont("/usr/share/fonts/TTF/FiraMono-Regular.ttf", 16);
	} else if (strncmp(widget->text_box.font, "Arial12", 7) == 0) {
		font = TTF_OpenFont("/usr/share/fonts/TTF/FiraMono-Regular.ttf", 12);
	} else if (strncmp(widget->text_box.font, "vic_22", 6) == 0) {
		font = TTF_OpenFont("/usr/share/fonts/TTF/FiraMono-Regular.ttf", 19);
		color.r = 0; color.g = 0; color.b = 0;
	} else {
		fprintf(stderr, "Unknown font: %s\n", widget->text_box.font);
	}
	if (font == NULL) {
		fprintf(stderr, "Could not open font: %s\n", TTF_GetError());
		return;
	}
	SDL_Surface* surface = TTF_RenderUTF8_Blended(font, widget->text_box.text, color);
	if (surface == NULL) {
		fprintf(stderr, "Could not render text: %s\n", TTF_GetError());
	} else {
		GLuint texture = 0;
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch / surface->format->BytesPerPixel);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		SDL_FreeSurface(surface);
		struct frect src = {
			.x = 0.0f,
			.y = 0.0f,
			.w = 1.0f,
			.h = 1.0f,
		};
		if (dstrect.w == 0.0f) dstrect.w = (float) surface->w;
		if (dstrect.h == 0.0f) dstrect.h = (float) surface->h;
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_QUADS);
		glTexCoord2f(src.x, src.y);
		glVertex2f(dstrect.x, dstrect.y);
		glTexCoord2f(src.x + src.w, src.y);
		glVertex2f(dstrect.x + dstrect.w, dstrect.y);
		glTexCoord2f(src.x + src.w, src.y + src.h);
		glVertex2f(dstrect.x + dstrect.w, dstrect.y + dstrect.h);
		glTexCoord2f(src.x, src.y + src.h);
		glVertex2f(dstrect.x, dstrect.y + dstrect.h);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glDeleteTextures(1, &texture);
	}
	TTF_CloseFont(font);
}

/* endregion */

static struct ui_widget* find_window_child(struct ui_widget* widgets, const char* name) {
	/* TODO: We really need a good way of just iterating every single ui widget i think */
	for (; widgets != NULL; widgets = widgets->next) {
		if (strcmp(widgets->name, name) == 0) {
			return widgets;
		}
		if (widgets->type == TYPE_WINDOW && widgets->window.children != NULL) {
			struct ui_widget* child = find_widget(widgets->window.children, name);
			if (child != NULL) {
				return child;
			}
		}
	}
	return NULL;
}

static void update_ui(struct game_state const* state) {
	/*update ui*/
	if (state->is_paused) {
		find_window_child(state->widgets, "speed_indicator")->button.frame = 0;
	} else {
		find_window_child(state->widgets, "speed_indicator")->button.frame = state->speed;
	}
	/*print the date as Junary 24, 1836*/
	char* date = malloc(32);
	int32_t year = state->year;
	int32_t month = state->month;
	int32_t day = state->day;
	sprintf(date, "%s %d, %d", month_names[month], day + 1, year + 1);
	if (find_window_child(state->widgets, "DateText")->instant_text_box.text != NULL) {
		free(find_window_child(state->widgets, "DateText")->instant_text_box.text);
	}
	find_window_child(state->widgets, "DateText")->instant_text_box.text = date;
}

void render_ui(struct game_state const* state) {
	update_ui(state);

	/* Flat pixel-perfect rendering mode. */
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, state->window_width, state->window_height);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, state->window_width, state->window_height, 0, 1, -1);

	{
		/* TODO: DEBUG Hide part of the menubar widget*/
		struct ui_widget* widget = state->widgets;
		for (; widget != NULL; widget = widget->next) {
			if (strcmp(widget->name, "menubar") == 0) {
				widget = widget->window.children;
				for (; widget != NULL; widget = widget->next) {
					if (strcmp(widget->name, "chat_window") == 0) {
						widget->window.dont_render = "true";
					}
				}
				break;
			}
		}
	}
	find_and_render_widget(state, "topbar");
	find_and_render_widget(state, "FPS_Counter");
	find_and_render_widget(state, "menubar");
	find_and_render_widget(state, "minimap_pic");
	glPopMatrix();
}