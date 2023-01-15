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
			src->buf = realloc(src->buf, src->bufcap); /* TODO: This can fail */
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

static bool try_parse_str(
	struct source* src,
	char const* str,
	bool ignore_leading_whitespace
) {
	size_t i;
	char c;

	if (ignore_leading_whitespace) consume_whitespace_and_comments(src);

	for (i = 0; i < strlen(str); i++) {
		if (i >= src->buflen) {
			if (!read_char(src, &c)) return false;
			if (src->buflen + 2 >= src->bufcap) {
				src->bufcap += 1;
				src->buf = realloc(src->buf, src->bufcap); /* TODO: This can fail */
			}
			src->buf[src->buflen] = c;
			src->buflen++;
			src->buf[src->buflen] = 0;
		} else {
			c = src->buf[i];
		}
		if (c != str[i]) return false;
	}

	return true;
}

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

static void parse_char(struct source* src, char target) {
	char c;
	if (!consume_char(src, &c, true)) {
		error(src, "Expected '%c', but got EOF", target);
	}
	if (c != target) {
		error(src, "Expected '%c', but got '%c'", target, c);
	}
}

static void parse_str(struct source* src, char const* target) {
	size_t len = strlen(target);
	size_t i;
	char c;
	for (i = 0; i < len; i++) {
		if (!consume_char(src, &c, true)) {
			error(src, "Expected '%s', but got EOF", target);
		}
		if (tolower(c) != tolower(target[i])) {
			error(src, "Expected '%s', but got '%c'", target, c);
		}
	}
}

static void parse_identifier(struct source* src, char** identifier) {
	char c;
	size_t size = 2;
	consume_char(src, &c, true);
	*identifier = calloc(2, size); /* TODO: This can fail */
	(*identifier)[0] = c;
	(*identifier)[1] = '\0';
	for (c = '\0'; peek_char(src, &c, false) && (isalnum(c) || c == '_');) {
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		*identifier = realloc(*identifier, size); /* TODO: This can fail */
		(*identifier)[size - 2] = (consume_char(src, &c, false), c);
		(*identifier)[size - 1] = '\0';
	}
}

static void parse_int_literal(struct source* src, int64_t* i) {
	char c;
	char* buf;
	char* endptr;
	size_t size = 1;
	buf = calloc(1, size); /* todo can fail */
	buf[0] = '\0';
	consume_whitespace_and_comments(src);
	for (c = '\0'; peek_char(src, &c, false) && (isdigit(c) || c == '_' || c == '-');) {
		if (c == '_') continue;
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		buf = realloc(buf, size); /* TODO: This can fail */
		buf[size - 2] = (consume_char(src, &c, false), c);
		buf[size - 1] = '\0';
	}

	*i = strtol(buf, &endptr, 10);

	if (*i == LONG_MIN || *i == LONG_MAX) {
		error(src, "Integer literal '%s' is out of range", buf);
	}

	if (*endptr != '\0') {
		error(src, "Invalid integer literal '%s'", buf);
	}

	free(buf);
}

static void parse_string_literal(struct source* src, char** str) {
	char c;
	size_t size = 1;

	if (peek_char(src, &c, true) && c == '"') {
		parse_char(src, '"');

		(*str) = calloc(1, size); /* todo can fail */
		(*str)[0] = '\0';
		for (c = '\0'; peek_char(src, &c, false) && c != '"';) {
			if (size == SIZE_MAX) {
				error(src, "String length exeeds %lu", SIZE_MAX);
			}
			size += 1;
			(*str) = realloc(*str, size); /* TODO: This can fail */
			/*if (c == '\\') {
				consume_char(src, &c, false);
				consume_char(src, &c, false);
				assert(c == 'n');
				(*str)[size - 2] = '\n';
			} else*/ {
				(*str)[size - 2] = (consume_char(src, &c, false), c);
			}
			(*str)[size - 1] = '\0';
		}

		parse_char(src, '"');
	} else {
		parse_identifier(src, str);
	}
}


static void parse_bool_literal(struct source* src, bool* value) {
	char c = '\0';
	if (peek_char(src, &c, true) && c == '1') {
		consume_char(src, &c, true);
		*value = true;
	} else if (peek_char(src, &c, true) && c == '0') {
		consume_char(src, &c, true);
		*value = false;
	} else if (peek_char(src, &c, true) && c == 'y') {
		parse_str(src, "yes");
		*value = true;
	} else if (peek_char(src, &c, true) && c == 'n') {
		parse_str(src, "no");
		*value = false;
	} else {
		error(src, "Expected bool, but got '%c'", c);
	}
}

static void parse_float_literal(struct source* src, double* f) {
	char c;
	char* buf;
	char* endptr;
	size_t size = 1;
	buf = calloc(1, size); /* todo can fail */
	consume_whitespace_and_comments(src);
	buf[0] = '\0';
	for (c = '\0'; peek_char(src, &c, false) && (isdigit(c) || c == '_' || c == '-' || c == '.');) {
		if (c == '_') continue;
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		buf = realloc(buf, size); /* TODO: This can fail */
		buf[size - 2] = (consume_char(src, &c, false), c);
		buf[size - 1] = '\0';
	}

	*f = strtod(buf, &endptr);

	if (*f == HUGE_VAL) {
		error(src, "Failed to convert to number: %s", strerror(errno));
	}

	if (*endptr != '\0') {
		error(src, "Invalid float literal '%s'", buf);
	}
}

static void parse_vec2i(struct source* src, struct vec2i* vec2) {
	char c = '\0';
	char* property = NULL;
	parse_char(src, '{');
	for (peek_char(src, &c, true); c != '}'; peek_char(src, &c, true)) {
		parse_identifier(src, &property);
		if (strcasecmp(property, "x") == 0) {
			parse_char(src, '=');
			parse_int_literal(src, &vec2->x);
		} else if (strcasecmp(property, "y") == 0) {
			parse_char(src, '=');
			parse_int_literal(src, &vec2->y);
		} else {
			error(src, "Unknown property '%s' for vec2", property);
		}
		free(property);
	}
	parse_char(src, '}');
}

static void parse_rgb(struct source* src, struct rgb* rgb) {
	parse_char(src, '{');
	parse_float_literal(src, &rgb->r);
	parse_float_literal(src, &rgb->g);
	parse_float_literal(src, &rgb->b);
	parse_char(src, '}');
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
		error(src, "Unknown click sound: %s", identifier);
	}
	free(identifier);
}

static void parse_load_type(struct source* src, enum load_type* load_type) {
	char* identifier = NULL;
	parse_string_literal(src, &identifier);
	if (strcasecmp(identifier, "ingame") == 0) {
		*load_type = LOAD_TYPE_INGAME;
	} else if (strcasecmp(identifier, "backend") == 0) {
		*load_type = LOAD_TYPE_BACKEND;
	} else if (strcasecmp(identifier, "frontend") == 0) {
		*load_type = LOAD_TYPE_FRONTEND;
	} else {
		error(src, "Unknown load type: %s", identifier);
	}
}

/* endregion */

/* region parse_sprites */
static void parse_line_chart(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_LINE_CHART;
	def->line_chart.name = NULL;
	def->line_chart.always_transparent = false;
	def->line_chart.line_width = 1;
	def->line_chart.size = (struct vec2i){0, 0};
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->line_chart.name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->line_chart.size);
		} else if (strcasecmp(property, "linewidth") == 0) {
			parse_int_literal(src, &def->line_chart.line_width);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->line_chart.always_transparent);
		} else {
			error(src, "Unknown property '%s' for line_chart_type", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_sprite(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_SPRITE;
	def->sprite.name = NULL;
	def->sprite.texture_file = NULL;
	def->sprite.effect_file = NULL;
	def->sprite.no_of_frames = 1;
	def->sprite.always_transparent = false;
	def->sprite.transparency_check = false;
	def->sprite.no_refcount = false;
	def->sprite.click_sound = CLICK_SOUND_CLICK;
	def->sprite.load_type = LOAD_TYPE_INGAME;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->sprite.name);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &def->sprite.texture_file);
		} else if (strcasecmp(property, "noofframes") == 0) {
			parse_int_literal(src, &def->sprite.no_of_frames);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->sprite.always_transparent);
		} else if (strcasecmp(property, "transparencecheck") == 0) {
			parse_bool_literal(src, &def->sprite.transparency_check);
		} else if (strcasecmp(property, "norefcount") == 0) {
			parse_bool_literal(src, &def->sprite.no_refcount);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &def->sprite.effect_file);
		} else if (strcasecmp(property, "clicksound") == 0) {
			parse_click_sound(src, &def->sprite.click_sound);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &def->sprite.load_type);
		} else {
			error(src, "Unknown property '%s' for sprite", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_masked_shield(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_MASKED_SHIELD;
	def->masked_shield.name = NULL;
	def->masked_shield.texture_file1 = NULL;
	def->masked_shield.texture_file2 = NULL;
	def->masked_shield.effect_file = false;
	def->masked_shield.always_transparent = false;
	def->masked_shield.flipv = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->masked_shield.name);
		} else if (strcasecmp(property, "texturefile1") == 0) {
			parse_string_literal(src, &def->masked_shield.texture_file1);
		} else if (strcasecmp(property, "texturefile2") == 0) {
			parse_string_literal(src, &def->masked_shield.texture_file2);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &def->masked_shield.effect_file);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->masked_shield.always_transparent);
		} else if (strcasecmp(property, "flipv") == 0) {
			parse_bool_literal(src, &def->masked_shield.flipv);
		} else {
			error(src, "Unknown property '%s' for masked shield", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_progress_bar(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_PROGRESS_BAR;
	def->progress_bar.name = NULL;
	def->progress_bar.color1 = (struct rgb){0, 0, 0};
	def->progress_bar.color2 = (struct rgb){0, 0, 0};
	def->progress_bar.texture_file_1 = NULL;
	def->progress_bar.texture_file_2 = NULL;
	def->progress_bar.size = (struct vec2i){0, 0};
	def->progress_bar.effect_file = NULL;
	def->progress_bar.always_transparent = false;
	def->progress_bar.horizontal = false;
	def->progress_bar.load_type = LOAD_TYPE_INGAME;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->progress_bar.name);
		} else if (strcasecmp(property, "color") == 0) {
			parse_rgb(src, &def->progress_bar.color1);
		} else if (strcasecmp(property, "colortwo") == 0) {
			parse_rgb(src, &def->progress_bar.color2);
		} else if (strcasecmp(property, "texturefile1") == 0) {
			parse_string_literal(src, &def->progress_bar.texture_file_1);
		} else if (strcasecmp(property, "texturefile2") == 0) {
			parse_string_literal(src, &def->progress_bar.texture_file_2);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->progress_bar.size);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &def->progress_bar.effect_file);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->progress_bar.always_transparent);
		} else if (strcasecmp(property, "horizontal") == 0) {
			parse_bool_literal(src, &def->progress_bar.horizontal);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &def->progress_bar.load_type);
		} else {
			error(src, "Unknown property '%s' for progress bar", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_cornered_tile_sprite(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_CORNERED_TILE_SPRITE;
	def->cornered_tile_sprite.name = NULL;
	def->cornered_tile_sprite.size = (struct vec2i){0, 0};
	def->cornered_tile_sprite.texture_file = NULL;
	def->cornered_tile_sprite.border_size = (struct vec2i){0, 0};
	def->cornered_tile_sprite.load_type = LOAD_TYPE_INGAME;
	def->cornered_tile_sprite.always_transparent = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->cornered_tile_sprite.name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->cornered_tile_sprite.size);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &def->cornered_tile_sprite.texture_file);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &def->cornered_tile_sprite.border_size);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &def->cornered_tile_sprite.load_type);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->cornered_tile_sprite.always_transparent);
		} else {
			error(src, "Unknown property '%s' for cornered tile sprite", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_text_sprite(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_TEXT_SPRITE;
	def->text_sprite.name = NULL;
	def->text_sprite.texture_file = NULL;
	def->text_sprite.no_of_frames = 0;
	def->text_sprite.effect_file = NULL;
	def->text_sprite.no_refcount = false;
	def->text_sprite.load_type = LOAD_TYPE_INGAME;
	def->text_sprite.click_sound = CLICK_SOUND_CLICK;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->text_sprite.name);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &def->text_sprite.texture_file);
		} else if (strcasecmp(property, "noofframes") == 0) {
			parse_int_literal(src, &def->text_sprite.no_of_frames);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &def->text_sprite.effect_file);
		} else if (strcasecmp(property, "norefcount") == 0) {
			parse_bool_literal(src, &def->text_sprite.no_refcount);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &def->text_sprite.load_type);
		} else if (strcasecmp(property, "clicksound") == 0) {
			parse_click_sound(src, &def->text_sprite.click_sound);
		} else {
			error(src, "Unknown property '%s' for text sprite", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_bar_chart(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_BAR_CHART;
	def->bar_chart.name = NULL;
	def->bar_chart.size = (struct vec2i){0, 0};
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->bar_chart.name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->bar_chart.size);
		} else {
			error(src, "Unknown property '%s' for bar chart", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_pie_chart(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_PIE_CHART;
	def->pie_chart.name = NULL;
	def->pie_chart.size = 0;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->pie_chart.name);
		} else if (strcasecmp(property, "size") == 0) {
			parse_int_literal(src, &def->pie_chart.size);
		} else {
			error(src, "Unknown property '%s' for pie chart", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_tile_sprite(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_TILE_SPRITE;
	def->tile_sprite.name = NULL;
	def->tile_sprite.texture_file = NULL;
	def->tile_sprite.effect_file = NULL;
	def->tile_sprite.load_type = LOAD_TYPE_INGAME;
	def->tile_sprite.no_refcount = false;
	def->tile_sprite.size = (struct vec2i){0, 0};
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->tile_sprite.name);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &def->tile_sprite.texture_file);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &def->tile_sprite.effect_file);
		} else if (strcasecmp(property, "loadtype") == 0) {
			parse_load_type(src, &def->tile_sprite.load_type);
		} else if (strcasecmp(property, "norefcount") == 0) {
			parse_bool_literal(src, &def->tile_sprite.no_refcount);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->tile_sprite.size);
		} else {
			error(src, "Unknown property '%s' for tile sprite", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_scrolling_sprite(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_SCROLLING_SPRITE;
	def->scrolling_sprite.name = NULL;
	def->scrolling_sprite.texture_file1 = NULL;
	def->scrolling_sprite.size = (struct vec2i){0, 0};
	def->scrolling_sprite.effect_file = NULL;
	def->scrolling_sprite.step = 0;
	def->scrolling_sprite.always_transparent = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->scrolling_sprite.name);
		} else if (strcasecmp(property, "texturefile1") == 0) {
			parse_string_literal(src, &def->scrolling_sprite.texture_file1);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->scrolling_sprite.size);
		} else if (strcasecmp(property, "effectfile") == 0) {
			parse_string_literal(src, &def->scrolling_sprite.effect_file);
		} else if (strcasecmp(property, "step") == 0) {
			parse_int_literal(src, &def->scrolling_sprite.step);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->scrolling_sprite.always_transparent);
		} else {
			error(src, "Unknown property '%s' for scrolling sprite", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_sprites(struct source* src, struct sprite_defs** defs) {
	char c = '\0';
	parse_str(src, "{");

	for (peek_char(src, &c, true); c != '}'; peek_char(src, &c, true)) {
		struct sprite_defs* def = calloc(1, sizeof(struct sprite_defs)); /* TODO: this can fail */
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
			error(src, "Unknown sprite type: %s", type);
		}
		def->next = *defs;
		*defs = def;

		free(type);
	}

	parse_str(src, "}");
}
/* endregion */

/* region parse_gui */

static void parse_gui_type(struct source* src, char const* type_name, struct gui_defs** defs);

static void parse_gui_format(struct source* src, enum gui_format* format) {
	char* identifier = NULL;
	parse_identifier(src, &identifier);
	if (strcasecmp(identifier, "left") == 0) {
		*format = GUI_FORMAT_LEFT;
	} else if (strcasecmp(identifier, "centre") == 0
		   || strcasecmp(identifier, "center") == 0) {
		*format = GUI_FORMAT_CENTER;
	} else if (strcasecmp(identifier, "right") == 0) {
		*format = GUI_FORMAT_RIGHT;
	} else if (strcasecmp(identifier, "justified") == 0) {
		*format = GUI_FORMAT_JUSTIFIED;
	} else {
		error(src, "Unknown text box format: %s", identifier);
	}
}

static void parse_gui_orientation(struct source* src, enum gui_orientation* orientation) {
	char* str = NULL;
	parse_string_literal(src, &str);
	if (strcasecmp(str, "lower_left") == 0) {
		*orientation = ORIENTATION_LOWER_LEFT;
	} else if (strcasecmp(str, "upper_left") == 0) {
		*orientation = ORIENTATION_UPPER_LEFT;
	} else if (strcasecmp(str, "center") == 0) {
		*orientation = ORIENTATION_CENTER;
	} else if (strcasecmp(str, "center_down") == 0) {
		*orientation = ORIENTATION_CENTER_DOWN;
	} else if (strcasecmp(str, "center_up") == 0) {
		*orientation = ORIENTATION_CENTER_UP;
	} else if (strcasecmp(str, "upper_right") == 0) {
		*orientation = ORIENTATION_UPPER_RIGHT;
	} else if (strcasecmp(str, "lower_right") == 0) {
		*orientation = ORIENTATION_LOWER_RIGHT;
	} else if (strcasecmp(str, "upperl_left") == 0) {
		warning(src, "Ignoring misspelled orientation '%s'", str);
	} else {
		error(src, "Unknown orientation: %s", str);
	}
	free(str);
}

static void parse_window(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_WINDOW;
	def->window.name = NULL;
	def->window.background = NULL;
	def->window.position = (struct vec2i){0, 0};
	def->window.size = (struct vec2i){0, 0};
	def->window.movable = 0;
	def->window.dont_render = NULL;
	def->window.horizontal_border = NULL;
	def->window.vertical_border = NULL;
	def->window.full_screen = false;
	def->window.children = NULL;
	def->window.orientation = ORIENTATION_LOWER_LEFT;
	def->window.up_sound = NULL;
	def->window.down_sound = NULL;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->window.name);
		} else if (strcasecmp(property, "background") == 0) {
			parse_string_literal(src, &def->window.background);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->window.position);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->window.size);
		} else if (strcasecmp(property, "moveable") == 0) {
			parse_bool_literal(src, &def->window.movable);
		} else if (strcasecmp(property, "dontrender") == 0) {
			parse_string_literal(src, &def->window.dont_render);
		} else if (strcasecmp(property, "horizontalborder") == 0) {
			parse_string_literal(src, &def->window.horizontal_border);
		} else if (strcasecmp(property, "verticalborder") == 0) {
			parse_string_literal(src, &def->window.vertical_border);
		} else if (strcasecmp(property, "fullscreen") == 0) {
			parse_bool_literal(src, &def->window.full_screen);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->window.orientation);
		} else if (strcasecmp(property, "upsound") == 0) {
			parse_string_literal(src, &def->window.up_sound);
		} else if (strcasecmp(property, "downsound") == 0) {
			parse_string_literal(src, &def->window.down_sound);
		} else {
			parse_gui_type(src, property, &def->window.children);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_icon(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_ICON;
	def->icon.name = NULL;
	def->icon.sprite = NULL;
	def->icon.position = (struct vec2i){0, 0};
	def->icon.orientation = ORIENTATION_LOWER_LEFT;
	def->icon.frame = 0;
	def->icon.button_mesh = NULL;
	def->icon.rotation = 0.0;
	def->icon.scale = 0.0;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->icon.name);
		} else if (strcasecmp(property, "spritetype") == 0) {
			parse_string_literal(src, &def->icon.sprite);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->icon.position);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->icon.orientation);
		} else if (strcasecmp(property, "frame") == 0) {
			parse_int_literal(src, &def->icon.frame);
		} else if (strcasecmp(property, "buttonmesh") == 0) {
			parse_string_literal(src, &def->icon.button_mesh);
		} else if (strcasecmp(property, "rotation") == 0) {
			parse_float_literal(src, &def->icon.rotation);
		} else if (strcasecmp(property, "scale") == 0) {
			parse_float_literal(src, &def->icon.scale);
		} else {
			error(src, "Unknown property '%s' for icon", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_gui_button(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_GUI_BUTTON;
	def->gui_button.name = NULL;
	def->gui_button.position = (struct vec2i){0, 0};
	def->gui_button.quad_texture_sprite = NULL;
	def->gui_button.button_text = NULL;
	def->gui_button.button_font = NULL;
	def->gui_button.click_sound = CLICK_SOUND_CLICK;
	def->gui_button.orientation = ORIENTATION_LOWER_LEFT;
	def->gui_button.tooltip = NULL;
	def->gui_button.tooltip_text = NULL;
	def->gui_button.delayed_tooltip_text = NULL;
	def->gui_button.sprite_type = NULL;
	def->gui_button.parent = NULL;
	def->gui_button.size = (struct vec2i){0, 0};
	def->gui_button.rotation = 0.0;
	def->gui_button.format = GUI_FORMAT_LEFT;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->window.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->gui_button.position);
		} else if (strcasecmp(property, "quadtexturesprite") == 0) {
			parse_string_literal(src, &def->gui_button.quad_texture_sprite);
		} else if (strcasecmp(property, "buttontext") == 0) {
			parse_string_literal(src, &def->gui_button.button_text);
		} else if (strcasecmp(property, "buttonfont") == 0) {
			parse_string_literal(src, &def->gui_button.button_font);
		} else if (strcasecmp(property, "shortcut") == 0) {
			parse_string_literal(src, &def->gui_button.shortcut);
		} else if (strcasecmp(property, "clicksound") == 0) {
			parse_click_sound(src, &def->gui_button.click_sound);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->gui_button.orientation);
		} else if (strcasecmp(property, "tooltip") == 0) {
			parse_string_literal(src, &def->gui_button.tooltip);
		} else if (strcasecmp(property, "tooltiptext") == 0) {
			parse_string_literal(src, &def->gui_button.tooltip_text);
		} else if (strcasecmp(property, "delayedtooltiptext") == 0) {
			parse_string_literal(src, &def->gui_button.delayed_tooltip_text);
		} else if (strcasecmp(property, "spritetype") == 0) {
			parse_string_literal(src, &def->gui_button.sprite_type);
		} else if (strcasecmp(property, "parent") == 0) {
			parse_string_literal(src, &def->gui_button.parent);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->gui_button.size);
		} else if (strcasecmp(property, "rotation") == 0) {
			parse_float_literal(src, &def->gui_button.rotation);
		} else if (strcasecmp(property, "format") == 0) {
			parse_gui_format(src, &def->gui_button.format);
		} else {
			error(src, "Unknown property '%s' for button", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_text_box(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_TEXT_BOX;
	def->text_box.name = NULL;
	def->text_box.position = (struct vec2i){0, 0};
	def->text_box.font = NULL;
	def->text_box.border_size = (struct vec2i){0, 0};
	def->text_box.text = NULL;
	def->text_box.max_width = 0;
	def->text_box.max_height = 0;
	def->text_box.format = GUI_FORMAT_LEFT;
	def->text_box.fixed_size = false;
	def->text_box.texture_file = NULL;
	def->text_box.orientation = ORIENTATION_LOWER_LEFT;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->text_box.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->text_box.position);
		} else if (strcasecmp(property, "font") == 0) {
			parse_string_literal(src, &def->text_box.font);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &def->text_box.border_size);
		} else if (strcasecmp(property, "text") == 0) {
			parse_string_literal(src, &def->text_box.text);
		} else if (strcasecmp(property, "maxwidth") == 0) {
			parse_int_literal(src, &def->text_box.max_width);
		} else if (strcasecmp(property, "maxheight") == 0) {
			parse_int_literal(src, &def->text_box.max_height);
		} else if (strcasecmp(property, "format") == 0) {
			parse_gui_format(src, &def->text_box.format);
		} else if (strcasecmp(property, "fixedsize") == 0) {
			parse_bool_literal(src, &def->text_box.fixed_size);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &def->text_box.texture_file);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->text_box.orientation);
		} else {
			error(src, "Unknown property '%s' for text box", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_instant_text_box(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_INSTANT_TEXT_BOX;
	def->instant_text_box.name = NULL;
	def->instant_text_box.position = (struct vec2i){0, 0};
	def->instant_text_box.font = NULL;
	def->instant_text_box.border_size = (struct vec2i){0, 0};
	def->instant_text_box.text = NULL;
	def->instant_text_box.max_width = 0;
	def->instant_text_box.max_height = 0;
	def->instant_text_box.format = GUI_FORMAT_LEFT;
	def->instant_text_box.fixed_size = false;
	def->instant_text_box.orientation = ORIENTATION_LOWER_LEFT;
	def->instant_text_box.texture_file = NULL;
	def->instant_text_box.always_transparent = false;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->instant_text_box.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->instant_text_box.position);
		} else if (strcasecmp(property, "font") == 0) {
			parse_string_literal(src, &def->instant_text_box.font);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &def->instant_text_box.border_size);
		} else if (strcasecmp(property, "text") == 0) {
			parse_string_literal(src, &def->instant_text_box.text);
		} else if (strcasecmp(property, "maxwidth") == 0) {
			parse_int_literal(src, &def->instant_text_box.max_width);
		} else if (strcasecmp(property, "maxheight") == 0) {
			parse_int_literal(src, &def->instant_text_box.max_height);
		} else if (strcasecmp(property, "format") == 0) {
			parse_gui_format(src, &def->instant_text_box.format);
		} else if (strcasecmp(property, "fixedsize") == 0) {
			parse_bool_literal(src, &def->instant_text_box.fixed_size);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->instant_text_box.orientation);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &def->instant_text_box.texture_file);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->instant_text_box.always_transparent);
		} else {
			error(src, "Unknown property '%s' for instant text box", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_overlapping_elements_box(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_OVERLAPPING_ELEMENTS_BOX;
	def->overlapping_elements_box.name = NULL;
	def->overlapping_elements_box.position = (struct vec2i){0, 0};
	def->overlapping_elements_box.size = (struct vec2i){0, 0};
	def->overlapping_elements_box.orientation = ORIENTATION_LOWER_LEFT;
	def->overlapping_elements_box.format = GUI_FORMAT_LEFT;
	def->overlapping_elements_box.spacing = 0;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->overlapping_elements_box.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->overlapping_elements_box.position);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->overlapping_elements_box.size);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->overlapping_elements_box.orientation);
		} else if (strcasecmp(property, "format") == 0) {
			parse_gui_format(src, &def->overlapping_elements_box.format);
		} else if (strcasecmp(property, "spacing") == 0) {
			parse_float_literal(src, &def->overlapping_elements_box.spacing);
		} else {
			error(src, "Unknown property '%s' for overlapping elements box", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_scrollbar(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_SCROLLBAR;
	def->scrollbar.name = NULL;
	def->scrollbar.slider = NULL;
	def->scrollbar.track = NULL;
	def->scrollbar.left_button = NULL;
	def->scrollbar.right_button = NULL;
	def->scrollbar.size = (struct vec2i){0, 0};
	def->scrollbar.position = (struct vec2i){0, 0};
	def->scrollbar.priority = 0;
	def->scrollbar.border_size = (struct vec2i){0, 0};
	def->scrollbar.max_value = 0.0;
	def->scrollbar.min_value = 0.0;
	def->scrollbar.step_size = 0.0;
	def->scrollbar.start_value = 0.0;
	def->scrollbar.horizontal = false;
	def->scrollbar.range_limit_min = 0.0;
	def->scrollbar.range_limit_max = 0.0;
	def->scrollbar.range_limit_min_icon = NULL;
	def->scrollbar.range_limit_max_icon = NULL;
	def->scrollbar.lockable = false;
	def->scrollbar.children = NULL;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->scrollbar.name);
		} else if (strcasecmp(property, "slider") == 0) {
			parse_string_literal(src, &def->scrollbar.slider);
		} else if (strcasecmp(property, "track") == 0) {
			parse_string_literal(src, &def->scrollbar.track);
		} else if (strcasecmp(property, "leftbutton") == 0) {
			parse_string_literal(src, &def->scrollbar.left_button);
		} else if (strcasecmp(property, "rightbutton") == 0) {
			parse_string_literal(src, &def->scrollbar.right_button);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->scrollbar.size);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->scrollbar.position);
		} else if (strcasecmp(property, "priority") == 0) {
			parse_int_literal(src, &def->scrollbar.priority);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &def->scrollbar.border_size);
		} else if (strcasecmp(property, "maxvalue") == 0) {
			parse_float_literal(src, &def->scrollbar.max_value);
		} else if (strcasecmp(property, "minvalue") == 0) {
			parse_float_literal(src, &def->scrollbar.min_value);
		} else if (strcasecmp(property, "stepsize") == 0) {
			parse_float_literal(src, &def->scrollbar.step_size);
		} else if (strcasecmp(property, "startvalue") == 0) {
			parse_float_literal(src, &def->scrollbar.start_value);
		} else if (strcasecmp(property, "horizontal") == 0) {
			parse_bool_literal(src, &def->scrollbar.horizontal);
		} else if (strcasecmp(property, "userangelimit") == 0) {
			parse_bool_literal(src, &def->scrollbar.use_range_limit);
		} else if (strcasecmp(property, "rangelimitmin") == 0) {
			parse_float_literal(src, &def->scrollbar.range_limit_min);
		} else if (strcasecmp(property, "rangelimitmax") == 0) {
			parse_float_literal(src, &def->scrollbar.range_limit_max);
		} else if (strcasecmp(property, "rangelimitminicon") == 0) {
			parse_string_literal(src, &def->scrollbar.range_limit_min_icon);
		} else if (strcasecmp(property, "rangelimitmaxicon") == 0) {
			parse_string_literal(src, &def->scrollbar.range_limit_max_icon);
		} else if (strcasecmp(property, "lockable") == 0) {
			parse_bool_literal(src, &def->scrollbar.lockable);
		} else {
			parse_gui_type(src, property, &def->scrollbar.children);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_checkbox(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_CHECKBOX;
	def->checkbox.name = NULL;
	def->checkbox.position = (struct vec2i){0, 0};
	def->checkbox.quad_texture_sprite = NULL;
	def->checkbox.tooltip = NULL;
	def->checkbox.tooltip_text = NULL;
	def->checkbox.delayed_tooltip_text = NULL;
	def->checkbox.button_text = NULL;
	def->checkbox.button_font = NULL;
	def->checkbox.orientation = ORIENTATION_LOWER_LEFT;
	def->checkbox.shortcut = NULL;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->checkbox.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->checkbox.position);
		} else if (strcasecmp(property, "quadtexturesprite") == 0) {
			parse_string_literal(src, &def->checkbox.quad_texture_sprite);
		} else if (strcasecmp(property, "tooltip") == 0) {
			parse_string_literal(src, &def->checkbox.tooltip);
		} else if (strcasecmp(property, "tooltiptext") == 0) {
			parse_string_literal(src, &def->checkbox.tooltip_text);
		} else if (strcasecmp(property, "delayedtooltiptext") == 0) {
			parse_string_literal(src, &def->checkbox.delayed_tooltip_text);
		} else if (strcasecmp(property, "buttontext") == 0) {
			parse_string_literal(src, &def->checkbox.button_text);
		} else if (strcasecmp(property, "buttonfont") == 0) {
			parse_string_literal(src, &def->checkbox.button_font);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->checkbox.orientation);
		} else if (strcasecmp(property, "shortcut") == 0) {
			parse_string_literal(src, &def->checkbox.shortcut);
		} else {
			error(src, "Unknown property '%s' for checkbox", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_edit_box(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_EDIT_BOX;
	def->edit_box.name = NULL;
	def->edit_box.position = (struct vec2i){0, 0};
	def->edit_box.texture_file = NULL;
	def->edit_box.font = NULL;
	def->edit_box.border_size = (struct vec2i){0, 0};
	def->edit_box.size = (struct vec2i){0, 0};
	def->edit_box.text = NULL;
	def->edit_box.orientation = ORIENTATION_LOWER_LEFT;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->edit_box.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->edit_box.position);
		} else if (strcasecmp(property, "texturefile") == 0) {
			parse_string_literal(src, &def->edit_box.texture_file);
		} else if (strcasecmp(property, "font") == 0) {
			parse_string_literal(src, &def->edit_box.font);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &def->edit_box.border_size);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->edit_box.size);
		} else if (strcasecmp(property, "text") == 0) {
			parse_string_literal(src, &def->edit_box.text);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->edit_box.orientation);
		} else {
			error(src, "Unknown property '%s' for edit box", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_list_box(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_LIST_BOX;
	def->list_box.name = NULL;
	def->list_box.position = (struct vec2i){0, 0};
	def->list_box.background = NULL;
	def->list_box.size = (struct vec2i){0, 0};
	def->list_box.orientation = ORIENTATION_LOWER_LEFT;
	def->list_box.spacing = 0;
	def->list_box.scrollbar_type = NULL;
	def->list_box.border_size = (struct vec2i){0, 0};
	def->list_box.priority = 0;
	def->list_box.step = 0;
	def->list_box.horizontal = false;
	def->list_box.offset = (struct vec2i){0, 0};
	def->list_box.always_transparent = false;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->list_box.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->list_box.position);
		} else if (strcasecmp(property, "background") == 0) {
			parse_string_literal(src, &def->list_box.background);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->list_box.size);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->list_box.orientation);
		} else if (strcasecmp(property, "spacing") == 0) {
			parse_int_literal(src, &def->list_box.spacing);
		} else if (strcasecmp(property, "scrollbartype") == 0) {
			parse_string_literal(src, &def->list_box.scrollbar_type);
		} else if (strcasecmp(property, "bordersize") == 0) {
			parse_vec2i(src, &def->list_box.border_size);
		} else if (strcasecmp(property, "priority") == 0) {
			parse_int_literal(src, &def->list_box.priority);
		} else if (strcasecmp(property, "step") == 0) {
			parse_int_literal(src, &def->list_box.step);
		} else if (strcasecmp(property, "horizontal") == 0) {
			parse_bool_literal(src, &def->list_box.horizontal);
		} else if (strcasecmp(property, "offset") == 0) {
			parse_vec2i(src, &def->list_box.offset);
		} else if (strcasecmp(property, "allwaystransparent") == 0) {
			parse_bool_literal(src, &def->list_box.always_transparent);
		} else {
			error(src, "Unknown property '%s' for list box", property);
		}
		peek_char(src, &c, true);
		free(property);
	}

	parse_str(src, "}");
}

static void parse_eu3_dialog(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_EU3_DIALOG;
	def->eu3_dialog.name = NULL;
	def->eu3_dialog.background = NULL;
	def->eu3_dialog.position = (struct vec2i){0, 0};
	def->eu3_dialog.size = (struct vec2i){0, 0};
	def->eu3_dialog.movable = 0;
	def->eu3_dialog.dont_render = NULL;
	def->eu3_dialog.horizontal_border = NULL;
	def->eu3_dialog.vertical_border = NULL;
	def->eu3_dialog.full_screen = false;
	def->eu3_dialog.orientation = ORIENTATION_LOWER_LEFT;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->eu3_dialog.name);
		} else if (strcasecmp(property, "background") == 0) {
			parse_string_literal(src, &def->eu3_dialog.background);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->eu3_dialog.position);
		} else if (strcasecmp(property, "size") == 0) {
			parse_vec2i(src, &def->eu3_dialog.size);
		} else if (strcasecmp(property, "moveable") == 0) {
			parse_bool_literal(src, &def->eu3_dialog.movable);
		} else if (strcasecmp(property, "dontrender") == 0) {
			parse_string_literal(src, &def->eu3_dialog.dont_render);
		} else if (strcasecmp(property, "horizontalborder") == 0) {
			parse_string_literal(src, &def->eu3_dialog.horizontal_border);
		} else if (strcasecmp(property, "verticalborder") == 0) {
			parse_string_literal(src, &def->eu3_dialog.vertical_border);
		} else if (strcasecmp(property, "fullscreen") == 0) {
			parse_bool_literal(src, &def->eu3_dialog.full_screen);
		} else if (strcasecmp(property, "orientation") == 0) {
			parse_gui_orientation(src, &def->eu3_dialog.orientation);
		} else {
			parse_gui_type(src, property, &def->scrollbar.children);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_shield(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_SHIELD;
	def->shield.name = NULL;
	def->shield.sprite_type = NULL;
	def->shield.position = (struct vec2i){0, 0};
	def->shield.rotation = 0;
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->shield.name);
		} else if (strcasecmp(property, "spritetype") == 0) {
			parse_string_literal(src, &def->shield.sprite_type);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->shield.position);
		} else if (strcasecmp(property, "rotation") == 0) {
			parse_float_literal(src, &def->shield.rotation);
		} else {
			error(src, "Unknown property '%s' for shield", property);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_position(struct source* src, struct gui_defs* def) {
	char c = '\0';

	def->type = TYPE_POSITION;
	def->position.name = NULL;
	def->position.position = (struct vec2i){0, 0};
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcasecmp(property, "name") == 0) {
			parse_string_literal(src, &def->position.name);
		} else if (strcasecmp(property, "position") == 0) {
			parse_vec2i(src, &def->position.position);
		} else {
			error(src, "Unknown property '%s' for position", property);
		}
		free(property);
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

static void parse_gui_type(struct source* src, char const* type_name, struct gui_defs** defs) {
	struct gui_defs* def = calloc(1, sizeof(struct gui_defs)); /* TODO: this can fail */
	if (strcasecmp(type_name, "windowtype") == 0) {
		parse_window(src, def);
	} else if (strcasecmp(type_name, "icontype") == 0) {
		parse_icon(src, def);
	} else if (strcasecmp(type_name, "guibuttontype") == 0) {
		parse_gui_button(src, def);
	} else if (strcasecmp(type_name, "textboxtype") == 0) {
		parse_text_box(src, def);
	} else if (strcasecmp(type_name, "instanttextboxtype") == 0) {
		parse_instant_text_box(src, def);
	} else if (strcasecmp(type_name, "overlappingelementsboxtype") == 0) {
		parse_overlapping_elements_box(src, def);
	} else if (strcasecmp(type_name, "scrollbartype") == 0) {
		parse_scrollbar(src, def);
	} else if (strcasecmp(type_name, "checkboxtype") == 0) {
		parse_checkbox(src, def);
	} else if (strcasecmp(type_name, "editboxtype") == 0) {
		parse_edit_box(src, def);
	} else if (strcasecmp(type_name, "listboxtype") == 0) {
		parse_list_box(src, def);
	} else if (strcasecmp(type_name, "eu3dialogtype") == 0) {
		parse_eu3_dialog(src, def);
	} else if (strcasecmp(type_name, "shieldtype") == 0) {
		parse_shield(src, def);
	} else if (strcasecmp(type_name, "positiontype") == 0) {
		parse_position(src, def);
	} else {
		error(src, "Unknown gui type_name: %s", type_name);
	}
	def->next = *defs;
	*defs = def;
}

static void parse_gui(struct source* src, struct gui_defs** defs) {
	char c = '\0';
	parse_str(src, "{");

	for (peek_char(src, &c, true); c != '}'; peek_char(src, &c, true)) {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		parse_gui_type(src, property, defs);
		free(property);
	}

	parse_str(src, "}");
}
/* endregion */

void parse(char const* path, struct sprite_defs** gfx_defs, struct gui_defs** gui_defs) {
	struct source src = {
		.loc = { .lineno = 1, .colno = 1 },
		.bufcap = 1,
		.buf = calloc(1, 1), /* TODO: this can fail */
		.buflen = 0,
		.file = fopen(path, "r"),  /* TODO: this can fail */
		.name = path,
	};
	char c = '\0';
	char* identifier = NULL;
	if (peek_char(&src, &c , true)) {
		parse_identifier(&src, &identifier);
		parse_char(&src, '=');
		if (strcasecmp(identifier, "spritetypes") == 0) {
			parse_sprites(&src, gfx_defs);
		} else if (strcasecmp(identifier, "guitypes") == 0) {
			parse_gui(&src, gui_defs);
		} else {
			warning(&src, "Ignoring unknown file type: %s", identifier);
		}
	} else {
		/* Ignoring empty files */
	}
	fclose(src.file);
	free(src.buf);
}