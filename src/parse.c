#include "parse.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

/* region generic parsing */
struct loc {
	uint64_t lineno;
	uint64_t colno;
};

struct source {
	char const* name;
	FILE* file;
	struct loc loc;
	char* buf;
	size_t bufcap;
	size_t buflen;
};

static void error(struct source* src, char* fmt, ...) {
	va_list ap;

	fprintf(stderr, "Syntax error in %s:%lu:%lu: ",
		src->name, src->loc.lineno,src->loc.colno);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

static void warning(struct source* src, char* fmt, ...) {
	va_list ap;

	fprintf(stderr, "Warning in %s:%lu:%lu: ",
		src->name, src->loc.lineno,src->loc.colno);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
}

static bool is_whitespace(char c) {
	return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}

/* Returns false if EOF */
static bool read_char(struct source* src, char* c) {
	size_t read = fread(c, 1, 1, src->file);
	if (read != 1) {
		if (feof(src->file)) return false;
		if (ferror(src->file)) {
			fprintf(stderr, "An error occurred while reading file:"
					"%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		assert(0);
	}
	return true;
}


static bool peek_char(struct source* src, char* c, bool ignore_whitespace);
static bool consume_char(struct source* src, char* c, bool ignore_whitespace);

static void consume_whitespace_and_comments(struct source* src) {
	char c;
	do {
		if (!peek_char(src, &c, false)) return;
		if (c == '#') {
			do {
				if (!consume_char(src, &c, false)) return;
			} while (c != '\n');
		} else if (is_whitespace(c) || c == ';') {
			consume_char(src, &c, false);
		}
	} while (is_whitespace(c));
}

/* Returns false if EOF */
static bool peek_char(struct source* src, char* c, bool ignore_whitespace) {
	if (ignore_whitespace) consume_whitespace_and_comments(src);

	if (src->buflen == 0) {
		if (!read_char(src, c)) return false;
		if (src->buflen + 2 >= src->bufcap) {
			src->bufcap += 1;
			src->buf = realloc(src->buf, src->bufcap);
			if (!src->buf) {
				error(src, "Failed to allocate memory: %s",
				      strerror(errno));
			}
		}
		src->buf[src->buflen] = *c;
		src->buflen++;
		src->buf[src->buflen] = 0;
	}
	*c = src->buf[0];
	while (ignore_whitespace && is_whitespace(*c)) {
		if (!consume_char(src, c, false)) return false;
		if (!peek_char(src, c, false)) return false;
	}
	return true;
}

/* Returns false if EOF */
static bool consume_char(struct source* src, char* c, bool ignore_whitespace) {
	if (ignore_whitespace) consume_whitespace_and_comments(src);

	/* Grab char from buffer or from file */
	if (src->buflen > 0) {
		*c = src->buf[0];
		/* Move buf + '\0' one character backwards, except first char */
		src->buf = memmove(src->buf, src->buf + 1, src->buflen);
		src->buflen--;
	} else {
		if (!read_char(src, c)) return false;
	}

	if (*c == '\n') {
		src->loc.lineno += 1; /* TODO overflow */
		src->loc.colno = 0;
	} else {
		src->loc.colno += 1; /* TODO overflow */
	}
	return true;
}

static void* calloc_or_die(size_t nmemb, size_t size) {
	void *result = calloc(nmemb, size);
	if (result == NULL) {
		fprintf(stderr, "Failed to allocate: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	return result;
}

static void parse_str(struct source* src, char const* target) {
	size_t len = strlen(target);
	size_t i;
	char c;
	for (i = 0; i < len; i++) {
		if (!consume_char(src, &c, true)) {
			error(src, "Expected '%s', but got EOF.", target);
		}
		if (tolower(c) != tolower(target[i])) {
			error(src, "Expected '%s', but got '%c'.", target, c);
		}
	}
}

static void parse_identifier(struct source* src, char** identifier) {
	char c;
	size_t size = 2;
	consume_char(src, &c, true);
	*identifier = calloc_or_die(2, size);
	(*identifier)[0] = c;
	(*identifier)[1] = '\0';
	for (c = '\0'; peek_char(src, &c, false) && (isalnum(c) || c == '_');) {
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu.", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		*identifier = realloc(*identifier, size);
		if (*identifier == NULL) {
			error(src,"Failed to allocate memory for identifier: "
				  "%s", strerror(errno));
		}
		(*identifier)[size - 2] = (consume_char(src, &c, false), c);
		(*identifier)[size - 1] = '\0';
	}
}

static void parse_string_literal(struct source* src, char** str) {
	char c;
	size_t size = 1;

	if (peek_char(src, &c, true) && c == '"') {
		parse_str(src, "\"");

		(*str) = calloc_or_die(1, size);
		(*str)[0] = '\0';
		for (c = '\0'; peek_char(src, &c, false) && c != '"';) {
			if (size == SIZE_MAX) {
				error(src, "String length exeeds "
				      "%lu.", SIZE_MAX);
			}
			size += 1;
			(*str) = realloc(*str, size);
			if (*str == NULL) {
				error(src, "Failed to allocate memory for "
				      "string: %s", strerror(errno));
			}
			(*str)[size - 2] = (consume_char(src, &c, false), c);
			(*str)[size - 1] = '\0';
		}

		parse_str(src, "\"");
	} else {
		parse_identifier(src, str);
	}
}

static void parse_int_literal(struct source* src, int64_t* i) {
	char c;
	char* buf;
	char* endptr;
	size_t size = 1;
	buf = calloc_or_die(1, size);
	buf[0] = '\0';
	consume_whitespace_and_comments(src);
	for (c = '\0'; peek_char(src, &c, false) && (isdigit(c) || c == '-');) {
		if (c == '_') continue;
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu.", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		buf = realloc(buf, size);
		if (buf == NULL) {
			error(src,"Failed to allocate memory when parsing "
			      "int: %s", strerror(errno));
		}
		buf[size - 2] = (consume_char(src, &c, false), c);
		buf[size - 1] = '\0';
	}

	*i = strtol(buf, &endptr, 10);

	if (*i == LONG_MIN || *i == LONG_MAX) {
		error(src, "Integer literal '%s' is out of range.", buf);
	}

	if (*endptr != '\0') {
		error(src, "Invalid integer literal '%s'.", buf);
	}

	free(buf);
}


static void parse_float_literal(struct source* src, double* f) {
	char c;
	char* buf;
	char* endptr;
	size_t size = 1;
	buf = calloc_or_die(1, size);
	consume_whitespace_and_comments(src);
	buf[0] = '\0';
	for (c = '\0'; peek_char(src, &c, false)
	               && (isdigit(c) || c == '-' || c == '.');) {
		if (c == '_') continue;
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu.", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		buf = realloc(buf, size);
		if (buf == NULL) {
			error(src, "Failed to allocate memory when "
				   "parsing float: %s", strerror(errno));
		}
		buf[size - 2] = (consume_char(src, &c, false), c);
		buf[size - 1] = '\0';
	}

	*f = strtod(buf, &endptr);

	if (*f == HUGE_VAL) {
		error(src, "Failed to convert to number: %s", strerror(errno));
	}

	if (*endptr != '\0') {
		error(src, "Invalid float literal '%s'.", buf);
	}
}

static void parse_bool_literal(struct source* src, bool* value) {
	char c = '\0';
	if (peek_char(src, &c, true) && c == '1') {
		parse_str(src, "1");
		*value = true;
	} else if (peek_char(src, &c, true) && c == '0') {
		parse_str(src, "0");
		*value = false;
	} else if (peek_char(src, &c, true) && c == 'y') {
		parse_str(src, "yes");
		*value = true;
	} else if (peek_char(src, &c, true) && c == 'n') {
		parse_str(src, "no");
		*value = false;
	} else {
		error(src, "Expected '0', '1', 'yes' or 'no', "
		           "but got '%c'.", c);
	}
}

static void parse_vec2i(struct source* src, struct vec2i* vec2) {
	char c = '\0';
	char* property = NULL;
	parse_str(src, "{");
	for (peek_char(src, &c, true); c != '}'; peek_char(src, &c, true)) {
		parse_identifier(src, &property);
		if (strcasecmp(property, "x") == 0) {
			parse_str(src, "=");
			parse_int_literal(src, &vec2->x);
		} else if (strcasecmp(property, "y") == 0) {
			parse_str(src, "=");
			parse_int_literal(src, &vec2->y);
		} else {
			error(src, "Unknown property '%s' for vec2.", property);
		}
		free(property);
	}
	parse_str(src, "}");
}

static void parse_rgb(struct source* src, struct rgb* rgb) {
	parse_str(src, "{");
	parse_float_literal(src, &rgb->r);
	parse_float_literal(src, &rgb->g);
	parse_float_literal(src, &rgb->b);
	parse_str(src, "}");
}

static void parse_click_sound(struct source* src, enum click_sound* click_sound) {
	char* identifier = NULL;
	parse_identifier(src, &identifier);
	if (strcasecmp(identifier, "click") == 0) {
		*click_sound = CLICK_SOUND_CLICK;
	} else if (strcasecmp(identifier, "close_window") == 0) {
		*click_sound = CLICK_SOUND_CLOSE_WINDOW;
	} else if (strcasecmp(identifier, "start_game") == 0) {
		*click_sound = CLICK_SOUND_START_GAME;
	} else {
		error(src, "Unknown click sound '%s'.", identifier);
	}
	free(identifier);
}

static void parse_load_type(struct source* src, enum sprite_load_type* load_type) {
	char* identifier = NULL;
	parse_string_literal(src, &identifier);
	if (strcasecmp(identifier, "ingame") == 0) {
		*load_type = SPRITE_LOAD_TYPE_INGAME;
	} else if (strcasecmp(identifier, "backend") == 0) {
		*load_type = SPRITE_LOAD_TYPE_BACKEND;
	} else if (strcasecmp(identifier, "frontend") == 0) {
		*load_type = SPRITE_LOAD_TYPE_FRONTEND;
	} else {
		error(src, "Unknown load type '%s'.", identifier);
	}
}

/* endregion */

/* region parse_sprites */
static void parse_line_chart(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_LINE_CHART;
	sprite->name = NULL;
	sprite->line_chart.always_transparent = false;
	sprite->line_chart.line_width = 1;
	sprite->line_chart.size = (struct vec2i){0, 0};
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &sprite->line_chart.size);
		} else if (strcasecmp(property, "linewidth") == 0) {
			parse_int_literal(src, &sprite->line_chart.line_width);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &sprite->line_chart.always_transparent);
		} else {
			error(src, "Unknown property '%s' for line_chart_type.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_sprite(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_SIMPLE_SPRITE;
	sprite->name = NULL;
	sprite->simple_sprite.texture_file = NULL;
	sprite->simple_sprite.effect_file = NULL;
	sprite->simple_sprite.no_of_frames = 1;
	sprite->simple_sprite.always_transparent = false;
	sprite->simple_sprite.transparency_check = false;
	sprite->simple_sprite.no_refcount = false;
	sprite->simple_sprite.click_sound = CLICK_SOUND_CLICK;
	sprite->simple_sprite.load_type = SPRITE_LOAD_TYPE_INGAME;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &sprite->simple_sprite.texture_file);
		} else if (strcasecmp(property, "noofframes") == 0) {
			parse_int_literal(src, &sprite->simple_sprite.no_of_frames);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &sprite->simple_sprite.always_transparent);
		} else if (strcasecmp(property, "transparencecheck") == 0) {
			parse_bool_literal(src, &sprite->simple_sprite.transparency_check);
		} else if (strcasecmp(property, "norefcount") == 0) {
			parse_bool_literal(src, &sprite->simple_sprite.no_refcount);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &sprite->simple_sprite.effect_file);
		} else if (strcasecmp(property, "clicksound") == 0) {
			parse_click_sound(src, &sprite->simple_sprite.click_sound);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &sprite->simple_sprite.load_type);
		} else {
			error(src, "Unknown property '%s' for simple_sprite.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_masked_shield(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_MASKED_SHIELD;
	sprite->name = NULL;
	sprite->masked_shield.texture_file1 = NULL;
	sprite->masked_shield.texture_file2 = NULL;
	sprite->masked_shield.effect_file = false;
	sprite->masked_shield.always_transparent = false;
	sprite->masked_shield.flipv = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "texturefile1") == 0) {
			parse_string_literal(src, &sprite->masked_shield.texture_file1);
		} else if (strcasecmp(property, "texturefile2") == 0) {
			parse_string_literal(src, &sprite->masked_shield.texture_file2);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &sprite->masked_shield.effect_file);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &sprite->masked_shield.always_transparent);
		} else if (strcasecmp(property, "flipv") == 0) {
			parse_bool_literal(src, &sprite->masked_shield.flipv);
		} else {
			error(src, "Unknown property '%s' for masked shield.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_progress_bar(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_PROGRESS_BAR;
	sprite->name = NULL;
	sprite->progress_bar.color1 = (struct rgb){0, 0, 0};
	sprite->progress_bar.color2 = (struct rgb){0, 0, 0};
	sprite->progress_bar.texture_file_1 = NULL;
	sprite->progress_bar.texture_file_2 = NULL;
	sprite->progress_bar.size = (struct vec2i){0, 0};
	sprite->progress_bar.effect_file = NULL;
	sprite->progress_bar.always_transparent = false;
	sprite->progress_bar.horizontal = false;
	sprite->progress_bar.load_type = SPRITE_LOAD_TYPE_INGAME;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "color") == 0) {
			parse_rgb(src, &sprite->progress_bar.color1);
		} else if (strcasecmp(property, "colortwo") == 0) {
			parse_rgb(src, &sprite->progress_bar.color2);
		} else if (strcasecmp(property, "texturefile1") == 0) {
			parse_string_literal(src, &sprite->progress_bar.texture_file_1);
		} else if (strcasecmp(property, "texturefile2") == 0) {
			parse_string_literal(src, &sprite->progress_bar.texture_file_2);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &sprite->progress_bar.size);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &sprite->progress_bar.effect_file);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &sprite->progress_bar.always_transparent);
		} else if (strcasecmp(property, "horizontal") == 0) {
			parse_bool_literal(src, &sprite->progress_bar.horizontal);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &sprite->progress_bar.load_type);
		} else {
			error(src, "Unknown property '%s' for progress bar.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_cornered_tile_sprite(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_CORNERED_TILE_SPRITE;
	sprite->name = NULL;
	sprite->cornered_tile_sprite.size = (struct vec2i){0, 0};
	sprite->cornered_tile_sprite.texture_file = NULL;
	sprite->cornered_tile_sprite.border_size = (struct vec2i){0, 0};
	sprite->cornered_tile_sprite.load_type = SPRITE_LOAD_TYPE_INGAME;
	sprite->cornered_tile_sprite.always_transparent = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &sprite->cornered_tile_sprite.size);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &sprite->cornered_tile_sprite.texture_file);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &sprite->cornered_tile_sprite.border_size);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &sprite->cornered_tile_sprite.load_type);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &sprite->cornered_tile_sprite.always_transparent);
		} else {
			error(src, "Unknown property '%s' for cornered tile simple_sprite.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_text_sprite(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_TEXT_SPRITE;
	sprite->name = NULL;
	sprite->text_sprite.texture_file = NULL;
	sprite->text_sprite.no_of_frames = 0;
	sprite->text_sprite.effect_file = NULL;
	sprite->text_sprite.no_refcount = false;
	sprite->text_sprite.load_type = SPRITE_LOAD_TYPE_INGAME;
	sprite->text_sprite.click_sound = CLICK_SOUND_CLICK;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &sprite->text_sprite.texture_file);
		} else if (strcasecmp(property, "noofframes") == 0) {
			parse_int_literal(src, &sprite->text_sprite.no_of_frames);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &sprite->text_sprite.effect_file);
		} else if (strcasecmp(property, "norefcount") == 0) {
			parse_bool_literal(src, &sprite->text_sprite.no_refcount);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &sprite->text_sprite.load_type);
		} else if (strcasecmp(property, "clicksound") == 0) {
			parse_click_sound(src, &sprite->text_sprite.click_sound);
		} else {
			error(src, "Unknown property '%s' for text simple_sprite.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_bar_chart(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_BAR_CHART;
	sprite->name = NULL;
	sprite->bar_chart.size = (struct vec2i){0, 0};
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &sprite->bar_chart.size);
		} else {
			error(src, "Unknown property '%s' for bar chart.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_pie_chart(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_PIE_CHART;
	sprite->name = NULL;
	sprite->pie_chart.size = 0;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_int_literal(src, &sprite->pie_chart.size);
		} else {
			error(src, "Unknown property '%s' for pie chart.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_tile_sprite(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_TILE_SPRITE;
	sprite->name = NULL;
	sprite->tile_sprite.texture_file = NULL;
	sprite->tile_sprite.effect_file = NULL;
	sprite->tile_sprite.load_type = SPRITE_LOAD_TYPE_INGAME;
	sprite->tile_sprite.no_refcount = false;
	sprite->tile_sprite.size = (struct vec2i){0, 0};
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &sprite->tile_sprite.texture_file);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &sprite->tile_sprite.effect_file);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &sprite->tile_sprite.load_type);
		} else if (strcasecmp(property, "norefcount") == 0) {
			parse_bool_literal(src, &sprite->tile_sprite.no_refcount);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &sprite->tile_sprite.size);
		} else {
			error(src, "Unknown property '%s' for tile simple_sprite.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_scrolling_sprite(struct source* src, struct sprite* sprite) {
	char c = '\0';
	sprite->type = TYPE_SCROLLING_SPRITE;
	sprite->name = NULL;
	sprite->scrolling_sprite.texture_file1 = NULL;
	sprite->scrolling_sprite.size = (struct vec2i){0, 0};
	sprite->scrolling_sprite.effect_file = NULL;
	sprite->scrolling_sprite.step = 0;
	sprite->scrolling_sprite.always_transparent = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &sprite->name);
		} else if (strcasecmp(property, "texturefile1") == 0) {
			parse_string_literal(src, &sprite->scrolling_sprite.texture_file1);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &sprite->scrolling_sprite.size);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &sprite->scrolling_sprite.effect_file);
		} else if (strcasecmp(property, "step") == 0) {
			parse_int_literal(src, &sprite->scrolling_sprite.step);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &sprite->scrolling_sprite.always_transparent);
		} else {
			error(src, "Unknown property '%s' for scrolling simple_sprite.", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_sprites(struct source* src, struct sprite** sprites) {
	char c = '\0';
	parse_str(src, "{");

	for (peek_char(src, &c, true); c != '}'; peek_char(src, &c, true)) {
		struct sprite* def = calloc_or_die(1, sizeof(struct sprite));
		char* type = NULL;
		parse_identifier(src, &type);
		if (strcasecmp(type, "linecharttype") == 0) {
			parse_line_chart(src, def);
		} else if (strcasecmp(type, "spritetype") == 0) {
			parse_sprite(src, def);
		} else if (strcasecmp(type, "maskedshieldtype") == 0) {
			parse_masked_shield(src, def);
		} else if (strcasecmp(type, "progressbartype") == 0) {
			parse_progress_bar(src, def);
		} else if (strcasecmp(type, "corneredtilespritetype") == 0) {
			parse_cornered_tile_sprite(src, def);
		} else if (strcasecmp(type, "textspritetype") == 0) {
			parse_text_sprite(src, def);
		} else if (strcasecmp(type, "barcharttype") == 0) {
			parse_bar_chart(src, def);
		} else if (strcasecmp(type, "piecharttype") == 0) {
			parse_pie_chart(src, def);
		} else if (strcasecmp(type, "tilespritetype") == 0) {
			parse_tile_sprite(src, def);
		} else if (strcasecmp(type, "scrollingsprite") == 0) {
			parse_scrolling_sprite(src, def);
		} else  {
			error(src, "Unknown simple_sprite type '%s'.", type);
		}
		def->next = NULL;
		if (*sprites == NULL) {
			*sprites = def;
		} else {
			struct sprite* last = *sprites;
			while (last->next != NULL) {
				last = last->next;
			}
			last->next = def;
		}

		free(type);
	}

	parse_str(src, "}");
}
/* endregion */

/* region parse_gui */

static void parse_widget(struct source* src, char const* name, struct ui_widget** widgets);

static void parse_format(struct source* src, enum ui_format* format) {
	char* identifier = NULL;
	parse_identifier(src, &identifier);
	if (strcasecmp(identifier, "left") == 0) {
		*format = UI_FORMAT_LEFT;
	} else if (strcasecmp(identifier, "centre") == 0
		   || strcasecmp(identifier, "center") == 0) {
		*format = UI_FORMAT_CENTER;
	} else if (strcasecmp(identifier, "right") == 0) {
		*format = UI_FORMAT_RIGHT;
	} else if (strcasecmp(identifier, "justified") == 0) {
		*format = UI_FORMAT_JUSTIFIED;
	} else {
		error(src, "Unknown text box format '%s'.", identifier);
	}
}

static void parse_orientation(struct source* src, enum ui_orientation* orientation) {
	char* str = NULL;
	parse_string_literal(src, &str);
	if (strcasecmp(str, "lower_left") == 0) {
		*orientation = UI_ORIENTATION_LOWER_LEFT;
	} else if (strcasecmp(str, "upper_left") == 0) {
		*orientation = UI_ORIENTATION_UPPER_LEFT;
	} else if (strcasecmp(str, "center") == 0) {
		*orientation = UI_ORIENTATION_CENTER;
	} else if (strcasecmp(str, "center_down") == 0) {
		*orientation = UI_ORIENTATION_CENTER_DOWN;
	} else if (strcasecmp(str, "center_up") == 0) {
		*orientation = UI_ORIENTATION_CENTER_UP;
	} else if (strcasecmp(str, "upper_right") == 0) {
		*orientation = UI_ORIENTATION_UPPER_RIGHT;
	} else if (strcasecmp(str, "lower_right") == 0) {
		*orientation = UI_ORIENTATION_LOWER_RIGHT;
	} else if (strcasecmp(str, "upperl_left") == 0) {
		warning(src, "Ignoring misspelled orientation '%s'.", str);
	} else {
		error(src, "Unknown orientation '%s'.", str);
	}
	free(str);
}

static void parse_window(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_WINDOW;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->window.background = NULL;
	widget->window.movable = 0;
	widget->window.dont_render = NULL;
	widget->window.horizontal_border = NULL;
	widget->window.vertical_border = NULL;
	widget->window.full_screen = false;
	widget->window.children = NULL;
	widget->window.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->window.up_sound = NULL;
	widget->window.down_sound = NULL;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "background") == 0) {
			parse_string_literal(src, &widget->window.background);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &widget->size);
		} else if (strcasecmp(property, "moveable") == 0) {
			parse_bool_literal(src, &widget->window.movable);
		} else if (strcasecmp(property, "dontrender") == 0) {
			parse_string_literal(src, &widget->window.dont_render);
		} else if (strcasecmp(property, "horizontalborder") == 0) {
			parse_string_literal(src, &widget->window.horizontal_border);
		} else if (strcasecmp(property, "verticalborder") == 0) {
			parse_string_literal(src, &widget->window.vertical_border);
		} else if (strcasecmp(property, "fullscreen") == 0) {
			parse_bool_literal(src, &widget->window.full_screen);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->window.orientation);
		} else if (strcasecmp(property, "upsound") == 0) {
			parse_string_literal(src, &widget->window.up_sound);
		} else if (strcasecmp(property, "downsound") == 0) {
			parse_string_literal(src, &widget->window.down_sound);
		} else {
			parse_widget(src, property, &widget->window.children);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_icon(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_ICON;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->icon.sprite = NULL;
	widget->icon.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->icon.frame = 0;
	widget->icon.button_mesh = NULL;
	widget->icon.rotation = 0.0;
	widget->icon.scale = 0.0;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "spritetype") == 0) {
			parse_string_literal(src, &widget->icon.sprite);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->icon.orientation);
		} else if (strcasecmp(property, "frame") == 0) {
			parse_int_literal(src, &widget->icon.frame);
		} else if (strcasecmp(property, "buttonmesh") == 0) {
			parse_string_literal(src, &widget->icon.button_mesh);
		} else if (strcasecmp(property, "rotation") == 0) {
			parse_float_literal(src, &widget->icon.rotation);
		} else if (strcasecmp(property, "scale") == 0) {
			parse_float_literal(src, &widget->icon.scale);
		} else {
			error(src, "Unknown property '%s' for icon.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_button(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_BUTTON;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->button.quad_texture_sprite = NULL;
	widget->button.button_text = NULL;
	widget->button.button_font = NULL;
	widget->button.click_sound = CLICK_SOUND_CLICK;
	widget->button.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->button.tooltip = NULL;
	widget->button.tooltip_text = NULL;
	widget->button.delayed_tooltip_text = NULL;
	widget->button.sprite_type = NULL;
	widget->button.parent = NULL;
	widget->button.rotation = 0.0;
	widget->button.format = UI_FORMAT_LEFT;
	widget->button.frame = 0;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "quadtexturesprite") == 0) {
			parse_string_literal(src, &widget->button.quad_texture_sprite);
		} else if (strcasecmp(property, "buttontext") == 0) {
			parse_string_literal(src, &widget->button.button_text);
		} else if (strcasecmp(property, "buttonfont") == 0) {
			parse_string_literal(src, &widget->button.button_font);
		} else if (strcasecmp(property, "shortcut") == 0) {
			parse_string_literal(src, &widget->button.shortcut);
		} else if (strcasecmp(property, "clicksound") == 0) {
			parse_click_sound(src, &widget->button.click_sound);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->button.orientation);
		} else if (strcasecmp(property, "tooltip") == 0) {
			parse_string_literal(src, &widget->button.tooltip);
		} else if (strcasecmp(property, "tooltiptext") == 0) {
			parse_string_literal(src, &widget->button.tooltip_text);
		} else if (strcasecmp(property, "delayedtooltiptext") == 0) {
			parse_string_literal(src, &widget->button.delayed_tooltip_text);
		} else if (strcasecmp(property, "spritetype") == 0) {
			parse_string_literal(src, &widget->button.sprite_type);
		} else if (strcasecmp(property, "parent") == 0) {
			parse_string_literal(src, &widget->button.parent);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &widget->size);
		} else if (strcasecmp(property, "rotation") == 0) {
			parse_float_literal(src, &widget->button.rotation);
		} else if (strcasecmp(property, "format") == 0) {
			parse_format(src, &widget->button.format);
		} else {
			error(src, "Unknown property '%s' for button.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_text_box(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_TEXT_BOX;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->text_box.font = NULL;
	widget->text_box.border_size = (struct vec2i){0, 0};
	widget->text_box.text = NULL;
	widget->text_box.max_width = 0;
	widget->text_box.max_height = 0;
	widget->text_box.format = UI_FORMAT_LEFT;
	widget->text_box.fixed_size = false;
	widget->text_box.texture_file = NULL;
	widget->text_box.orientation = UI_ORIENTATION_LOWER_LEFT;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "font") == 0) {
			parse_string_literal(src, &widget->text_box.font);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &widget->text_box.border_size);
		} else if (strcasecmp(property, "text") == 0) {
			parse_string_literal(src, &widget->text_box.text);
		} else if (strcasecmp(property, "maxwidth") == 0) {
			parse_int_literal(src, &widget->text_box.max_width);
		} else if (strcasecmp(property, "maxheight") == 0) {
			parse_int_literal(src, &widget->text_box.max_height);
		} else if (strcasecmp(property, "format") == 0) {
			parse_format(src, &widget->text_box.format);
		} else if (strcasecmp(property, "fixedsize") == 0) {
			parse_bool_literal(src, &widget->text_box.fixed_size);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &widget->text_box.texture_file);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->text_box.orientation);
		} else {
			error(src, "Unknown property '%s' for text box.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_instant_text_box(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_INSTANT_TEXT_BOX;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->instant_text_box.font = NULL;
	widget->instant_text_box.border_size = (struct vec2i){0, 0};
	widget->instant_text_box.text = NULL;
	widget->instant_text_box.max_width = 0;
	widget->instant_text_box.max_height = 0;
	widget->instant_text_box.format = UI_FORMAT_LEFT;
	widget->instant_text_box.fixed_size = false;
	widget->instant_text_box.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->instant_text_box.texture_file = NULL;
	widget->instant_text_box.always_transparent = false;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "font") == 0) {
			parse_string_literal(src, &widget->instant_text_box.font);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &widget->instant_text_box.border_size);
		} else if (strcasecmp(property, "text") == 0) {
			parse_string_literal(src, &widget->instant_text_box.text);
		} else if (strcasecmp(property, "maxwidth") == 0) {
			parse_int_literal(src, &widget->instant_text_box.max_width);
		} else if (strcasecmp(property, "maxheight") == 0) {
			parse_int_literal(src, &widget->instant_text_box.max_height);
		} else if (strcasecmp(property, "format") == 0) {
			parse_format(src, &widget->instant_text_box.format);
		} else if (strcasecmp(property, "fixedsize") == 0) {
			parse_bool_literal(src, &widget->instant_text_box.fixed_size);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->instant_text_box.orientation);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &widget->instant_text_box.texture_file);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &widget->instant_text_box.always_transparent);
		} else {
			error(src, "Unknown property '%s' for instant text box.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_overlapping_elements_box(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_OVERLAPPING_ELEMENTS_BOX;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->overlapping_elements_box.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->overlapping_elements_box.format = UI_FORMAT_LEFT;
	widget->overlapping_elements_box.spacing = 0;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &widget->size);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->overlapping_elements_box.orientation);
		} else if (strcasecmp(property, "format") == 0) {
			parse_format(src, &widget->overlapping_elements_box.format);
		} else if (strcasecmp(property, "spacing") == 0) {
			parse_float_literal(src, &widget->overlapping_elements_box.spacing);
		} else {
			error(src, "Unknown property '%s' for overlapping elements box.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_scrollbar(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_SCROLLBAR;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->scrollbar.slider = NULL;
	widget->scrollbar.track = NULL;
	widget->scrollbar.left_button = NULL;
	widget->scrollbar.right_button = NULL;
	widget->scrollbar.priority = 0;
	widget->scrollbar.border_size = (struct vec2i){0, 0};
	widget->scrollbar.max_value = 0.0;
	widget->scrollbar.min_value = 0.0;
	widget->scrollbar.step_size = 0.0;
	widget->scrollbar.start_value = 0.0;
	widget->scrollbar.horizontal = false;
	widget->scrollbar.range_limit_min = 0.0;
	widget->scrollbar.range_limit_max = 0.0;
	widget->scrollbar.range_limit_min_icon = NULL;
	widget->scrollbar.range_limit_max_icon = NULL;
	widget->scrollbar.lockable = false;
	widget->scrollbar.children = NULL;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "slider") == 0) {
			parse_string_literal(src, &widget->scrollbar.slider);
		} else if (strcasecmp(property, "track") == 0) {
			parse_string_literal(src, &widget->scrollbar.track);
		} else if (strcasecmp(property, "leftbutton") == 0) {
			parse_string_literal(src, &widget->scrollbar.left_button);
		} else if (strcasecmp(property, "rightbutton") == 0) {
			parse_string_literal(src, &widget->scrollbar.right_button);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &widget->size);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "priority") == 0) {
			parse_int_literal(src, &widget->scrollbar.priority);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &widget->scrollbar.border_size);
		} else if (strcasecmp(property, "maxvalue") == 0) {
			parse_float_literal(src, &widget->scrollbar.max_value);
		} else if (strcasecmp(property, "minvalue") == 0) {
			parse_float_literal(src, &widget->scrollbar.min_value);
		} else if (strcasecmp(property, "stepsize") == 0) {
			parse_float_literal(src, &widget->scrollbar.step_size);
		} else if (strcasecmp(property, "startvalue") == 0) {
			parse_float_literal(src, &widget->scrollbar.start_value);
		} else if (strcasecmp(property, "horizontal") == 0) {
			parse_bool_literal(src, &widget->scrollbar.horizontal);
		} else if (strcasecmp(property, "userangelimit") == 0) {
			parse_bool_literal(src, &widget->scrollbar.use_range_limit);
		} else if (strcasecmp(property, "rangelimitmin") == 0) {
			parse_float_literal(src, &widget->scrollbar.range_limit_min);
		} else if (strcasecmp(property, "rangelimitmax") == 0) {
			parse_float_literal(src, &widget->scrollbar.range_limit_max);
		} else if (strcasecmp(property, "rangelimitminicon") == 0) {
			parse_string_literal(src, &widget->scrollbar.range_limit_min_icon);
		} else if (strcasecmp(property, "rangelimitmaxicon") == 0) {
			parse_string_literal(src, &widget->scrollbar.range_limit_max_icon);
		} else if (strcasecmp(property, "lockable") == 0) {
			parse_bool_literal(src, &widget->scrollbar.lockable);
		} else {
			parse_widget(src, property, &widget->scrollbar.children);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_checkbox(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_CHECKBOX;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->checkbox.quad_texture_sprite = NULL;
	widget->checkbox.tooltip = NULL;
	widget->checkbox.tooltip_text = NULL;
	widget->checkbox.delayed_tooltip_text = NULL;
	widget->checkbox.button_text = NULL;
	widget->checkbox.button_font = NULL;
	widget->checkbox.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->checkbox.shortcut = NULL;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "quadtexturesprite") == 0) {
			parse_string_literal(src, &widget->checkbox.quad_texture_sprite);
		} else if (strcasecmp(property, "tooltip") == 0) {
			parse_string_literal(src, &widget->checkbox.tooltip);
		} else if (strcasecmp(property, "tooltiptext") == 0) {
			parse_string_literal(src, &widget->checkbox.tooltip_text);
		} else if (strcasecmp(property, "delayedtooltiptext") == 0) {
			parse_string_literal(src, &widget->checkbox.delayed_tooltip_text);
		} else if (strcasecmp(property, "buttontext") == 0) {
			parse_string_literal(src, &widget->checkbox.button_text);
		} else if (strcasecmp(property, "buttonfont") == 0) {
			parse_string_literal(src, &widget->checkbox.button_font);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->checkbox.orientation);
		} else if (strcasecmp(property, "shortcut") == 0) {
			parse_string_literal(src, &widget->checkbox.shortcut);
		} else {
			error(src, "Unknown property '%s' for checkbox.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_edit_box(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_EDIT_BOX;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->edit_box.texture_file = NULL;
	widget->edit_box.font = NULL;
	widget->edit_box.border_size = (struct vec2i){0, 0};
	widget->edit_box.text = NULL;
	widget->edit_box.orientation = UI_ORIENTATION_LOWER_LEFT;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &widget->edit_box.texture_file);
		} else if (strcasecmp(property, "font") == 0) {
			parse_string_literal(src, &widget->edit_box.font);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &widget->edit_box.border_size);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &widget->size);
		} else if (strcasecmp(property, "text") == 0) {
			parse_string_literal(src, &widget->edit_box.text);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->edit_box.orientation);
		} else {
			error(src, "Unknown property '%s' for edit box.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_list_box(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_LIST_BOX;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->list_box.background = NULL;
	widget->list_box.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->list_box.spacing = 0;
	widget->list_box.scrollbar_type = NULL;
	widget->list_box.border_size = (struct vec2i){0, 0};
	widget->list_box.priority = 0;
	widget->list_box.step = 0;
	widget->list_box.horizontal = false;
	widget->list_box.offset = (struct vec2i){0, 0};
	widget->list_box.always_transparent = false;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "background") == 0) {
			parse_string_literal(src, &widget->list_box.background);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &widget->size);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->list_box.orientation);
		} else if (strcasecmp(property, "spacing") == 0) {
			parse_int_literal(src, &widget->list_box.spacing);
		} else if (strcasecmp(property, "scrollbartype") == 0) {
			parse_string_literal(src, &widget->list_box.scrollbar_type);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &widget->list_box.border_size);
		} else if (strcasecmp(property, "priority") == 0) {
			parse_int_literal(src, &widget->list_box.priority);
		} else if (strcasecmp(property, "step") == 0) {
			parse_int_literal(src, &widget->list_box.step);
		} else if (strcasecmp(property, "horizontal") == 0) {
			parse_bool_literal(src, &widget->list_box.horizontal);
		} else if (strcasecmp(property, "offset") == 0) {
			parse_vec2i(src, &widget->list_box.offset);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &widget->list_box.always_transparent);
		} else {
			error(src, "Unknown property '%s' for list box.", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_eu3_dialog(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_EU3_DIALOG;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->eu3_dialog.background = NULL;
	widget->eu3_dialog.movable = 0;
	widget->eu3_dialog.dont_render = NULL;
	widget->eu3_dialog.horizontal_border = NULL;
	widget->eu3_dialog.vertical_border = NULL;
	widget->eu3_dialog.full_screen = false;
	widget->eu3_dialog.orientation = UI_ORIENTATION_LOWER_LEFT;
	widget->eu3_dialog.children = NULL;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "background") == 0) {
			parse_string_literal(src, &widget->eu3_dialog.background);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &widget->size);
		} else if (strcasecmp(property, "moveable") == 0) {
			parse_bool_literal(src, &widget->eu3_dialog.movable);
		} else if (strcasecmp(property, "dontrender") == 0) {
			parse_string_literal(src, &widget->eu3_dialog.dont_render);
		} else if (strcasecmp(property, "horizontalborder") == 0) {
			parse_string_literal(src, &widget->eu3_dialog.horizontal_border);
		} else if (strcasecmp(property, "verticalborder") == 0) {
			parse_string_literal(src, &widget->eu3_dialog.vertical_border);
		} else if (strcasecmp(property, "fullscreen") == 0) {
			parse_bool_literal(src, &widget->eu3_dialog.full_screen);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_orientation(src, &widget->eu3_dialog.orientation);
		} else {
			parse_widget(src, property, &widget->eu3_dialog.children);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_shield(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_SHIELD;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	widget->shield.sprite_type = NULL;
	widget->shield.rotation = 0;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "spritetype") == 0) {
			parse_string_literal(src, &widget->shield.sprite_type);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else if (strcasecmp(property, "rotation") == 0) {
			parse_float_literal(src, &widget->shield.rotation);
		} else {
			error(src, "Unknown property '%s' for shield.", property);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_position(struct source* src, struct ui_widget* widget) {
	char c = '\0';

	widget->type = TYPE_POSITION;
	widget->name = NULL;
	widget->position = (struct vec2i){0, 0};
	widget->size = (struct vec2i){0, 0};
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &widget->name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &widget->position);
		} else {
			error(src, "Unknown property '%s' for position.", property);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_widget(struct source* src, char const* name, struct ui_widget** widgets) {
	struct ui_widget* widget = calloc_or_die(1, sizeof(struct ui_widget));
	if (strcasecmp(name, "windowtype") == 0) {
		parse_window(src, widget);
	} else if (strcasecmp(name, "icontype") == 0) {
		parse_icon(src, widget);
	} else if (strcasecmp(name, "guibuttontype") == 0) {
		parse_button(src, widget);
	} else if (strcasecmp(name, "textboxtype") == 0) {
		parse_text_box(src, widget);
	} else if (strcasecmp(name, "instanttextboxtype") == 0) {
		parse_instant_text_box(src, widget);
	} else if (strcasecmp(name, "overlappingelementsboxtype") == 0) {
		parse_overlapping_elements_box(src, widget);
	} else if (strcasecmp(name, "scrollbartype") == 0) {
		parse_scrollbar(src, widget);
	} else if (strcasecmp(name, "checkboxtype") == 0) {
		parse_checkbox(src, widget);
	} else if (strcasecmp(name, "editboxtype") == 0) {
		parse_edit_box(src, widget);
	} else if (strcasecmp(name, "listboxtype") == 0) {
		parse_list_box(src, widget);
	} else if (strcasecmp(name, "eu3dialogtype") == 0) {
		parse_eu3_dialog(src, widget);
	} else if (strcasecmp(name, "shieldtype") == 0) {
		parse_shield(src, widget);
	} else if (strcasecmp(name, "positiontype") == 0) {
		parse_position(src, widget);
	} else {
		error(src, "Unknown gui type_name '%s'.", name);
	}
	/* Add to the end of the list */
	widget->next = NULL;
	if (*widgets == NULL) {
		*widgets = widget;
	} else {
		struct ui_widget* last = *widgets;
		while (last->next != NULL) {
			last = last->next;
		}
		last->next = widget;
	}
}

static void parse_widgets(struct source* src, struct ui_widget** widgets) {
	char c = '\0';
	parse_str(src, "{");

	for (peek_char(src, &c, true); c != '}'; peek_char(src, &c, true)) {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		parse_widget(src, property, widgets);
		free(property);
	}

	parse_str(src, "}");
}
/* endregion */

void parse(
	char const* path,
	struct sprite** sprites,
	struct ui_widget** widgets
) {
	struct source src;
	char c = '\0';
	char* identifier = NULL;
	src.loc.lineno = 1;
	src.loc.colno = 1;
	src.bufcap = 1;
	src.buf = calloc_or_die(1, 1);
	src.buflen = 0;
	src.file = fopen(path, "r");
	src.name = path;
	if (src.file == NULL) {
		fprintf(stderr, "Failed to open file '%s': %s\n", path,
		        strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (peek_char(&src, &c , true)) {
		parse_identifier(&src, &identifier);
		parse_str(&src, "=");
		if (strcasecmp(identifier, "spritetypes") == 0) {
			parse_sprites(&src, sprites);
		} else if (strcasecmp(identifier, "guitypes") == 0) {
			parse_widgets(&src, widgets);
		} else {
			warning(&src, "Ignoring unknown file type "
			        "'%s'.", identifier);
		}
	} else {
		/* Ignoring empty files */
	}
	fclose(src.file);
	free(src.buf);
}

void free_sprites(struct sprite* sprites) {
	fprintf(stderr, "WARNING: free_sprites is not implemented.\n");
}

void free_widgets(struct ui_widget* widgets) {
	fprintf(stderr, "WARNING: free_widgets is not implemented.\n");
}