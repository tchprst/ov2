#include <GL/gl.h>
#include <SOIL/SOIL.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "bitmap_font.h"

struct frect {
	float x, y, w, h;
};

/* region textures */

static GLuint load_texture(char const* path) {
	GLuint id;
	char* full_path = malloc(strlen("gfx/fonts/") + strlen(path) + strlen(".tga") + 1);
	if (full_path == NULL) {
		fprintf(stderr, "Failed to allocate memory for full path.\n");
		return 0;
	}
	strcpy(full_path, "gfx/fonts/");
	strcat(full_path, path);
	strcat(full_path, ".tga");
	if ((id = SOIL_load_OGL_texture(
		full_path,
		SOIL_LOAD_RGBA,
		SOIL_CREATE_NEW_ID,
		0
	)) == 0) {
		fprintf(stderr, "SOIL loading error while loading "
				"texture %s: %s\n", full_path,
			SOIL_last_result());
	}
	free(full_path);
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

/* endregion */

/* region font description */

static struct font_desc* load_font_desc(char const* path) {
	struct font_desc* font_desc = NULL;
	char* full_path = NULL;
	font_desc = malloc(sizeof(struct font_desc));
	if (font_desc == NULL) {
		fprintf(stderr, "Failed to allocate memory for font description.\n");
		return NULL;
	}
	full_path = malloc(strlen("gfx/fonts/") + strlen(path) + strlen(".fnt") + 1);
	if (full_path == NULL) {
		fprintf(stderr, "Failed to allocate memory for full path.\n");
		return NULL;
	}
	strcpy(full_path, "gfx/fonts/");
	strcat(full_path, path);
	strcat(full_path, ".fnt");
	parse_font_desc(full_path, font_desc);
	free(full_path);
	return font_desc;
}

/* TODO: Again let's avoid these global buffers. */
struct loaded_font_desc {
	char const* name;
	struct font_desc* font_desc;
	struct loaded_font_desc* next;
};

static struct loaded_font_desc* loaded_font_descs = NULL;

static struct font_desc* find_font_desc(char const* name) {
	struct loaded_font_desc* loaded_font_desc = loaded_font_descs;
	for (; loaded_font_desc != NULL; loaded_font_desc = loaded_font_desc->next) {
		if (strcmp(loaded_font_desc->name, name) == 0) {
			return loaded_font_desc->font_desc;
		}
	}
	return NULL;
}

static struct font_desc* find_or_load_font_desc(char const* name) {
	struct font_desc* font_desc = find_font_desc(name);
	if (font_desc == NULL && (font_desc = load_font_desc(name), font_desc != NULL)) {
		struct loaded_font_desc* new_font_desc =
			malloc(sizeof(struct loaded_font_desc));
		new_font_desc->name = name;
		new_font_desc->font_desc = font_desc;
		new_font_desc->next = loaded_font_descs;
		loaded_font_descs = new_font_desc;
	}
	return font_desc;
}

/* endregion */

void render_bitmap_font(
	struct bitmap_font* bitmap_font,
	char const* text,
	float x,
	float y
) {
	struct font_desc* font_desc = find_or_load_font_desc(bitmap_font->font_name);
	GLuint texture = find_or_load_texture(bitmap_font->font_name);
	size_t i = 0;
	GLint texture_width, texture_height = 0;
	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texture_width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texture_height);
	glBindTexture(GL_TEXTURE_2D, 0);

	glColor4f((GLfloat) bitmap_font->color.r,
		  (GLfloat) bitmap_font->color.g,
		  (GLfloat) bitmap_font->color.b,
		  (GLfloat) bitmap_font->color.a);

	for (i = 0; i < strlen(text); i++) {
		struct frect srcrect;
		struct frect dstrect;
		unsigned char c = text[i];
		if (c == '\n') {
			x = 0;
			y += (float) font_desc->line_height;
			continue;
		}
		if (font_desc->chars[c].id == 0) return;
		assert(font_desc->chars[c].id == c);
		srcrect.x = (float) font_desc->chars[c].x / (float) texture_width;
		srcrect.y = (float) font_desc->chars[c].y / (float) texture_height;
		srcrect.w = (float) font_desc->chars[c].width / (float) texture_width;
		srcrect.h = (float) font_desc->chars[c].height / (float) texture_height;
		dstrect.x = x + (float) font_desc->chars[c].xoffset;
		dstrect.y = y + (float) font_desc->chars[c].yoffset;
		dstrect.w = (float) font_desc->chars[c].width;
		dstrect.h = (float) font_desc->chars[c].height;
		render_texture(texture, &srcrect, &dstrect);
		x += (float) font_desc->chars[c].xadvance;
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}