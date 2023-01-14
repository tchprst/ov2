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

/* region generic parsing */
struct loc {
	uint64_t lineno;
	uint64_t colno;
};

struct source {
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
			fprintf(stderr, "An error ocurred while reading file:"
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
		}
		if (is_whitespace(c)) consume_char(src, &c, false);
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

	fprintf(stderr, "Syntax error at %lu:%lu: ", src->loc.lineno,
		src->loc.colno);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

static void parse_char(struct source* src, char target) {
	char c;
	if (!consume_char(src, &c, true)) {
		error(src, "Expected '%c', but got EOF\n", target);
	}
	if (c != target) {
		error(src, "Expected '%c', but got '%c'\n", target, c);
	}
}

static void parse_str(struct source* src, char const* target) {
	size_t len = strlen(target);
	size_t i;
	char c;
	for (i = 0; i < len; i++) {
		if (!consume_char(src, &c, true)) {
			error(src, "Expected '%s', but got EOF\n", target);
		}
		if (c != target[i]) {
			error(src, "Expected '%s', but got '%c'\n", target, c);
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
			error(src, "Identifier length exeeds %lu\n", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		*identifier = realloc(*identifier, size); /* TODO: This can fail */
		(*identifier)[size - 2] = (consume_char(src, &c, false), c);
		(*identifier)[size - 1] = '\0';
	}
}

static void parse_constant_integer(struct source* src, int64_t* i) {
	char c;
	char* buf;
	char* endptr;
	size_t size = 1;
	buf = calloc(1, size); /* todo can fail */
	buf[0] = '\0';
	for (c = '\0'; peek_char(src, &c, true) && (isdigit(c) || c == '_');) {
		if (c == '_') continue;
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu\n", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		buf = realloc(buf, size); /* TODO: This can fail */
		buf[size - 2] = (consume_char(src, &c, true), c);
		buf[size - 1] = '\0';
	}

	errno = 0; /* To distinguish success/failure after call */ /* TODO */
	*i = strtol(buf, &endptr, 10);

	if (errno != 0) {
		error(src, "Failed to convert to number: %s", strerror(errno));
	}
	if (endptr == buf) {
		error(src, "No digits were found in %s\n", buf);
	}

	free(buf);
}

static void parse_constant_string(struct source* src, char** str) {
	char c;
	size_t size = 1;

	parse_char(src, '"');

	(*str) = calloc(1, size); /* todo can fail */
	(*str)[0] = '\0';
	for (c = '\0'; peek_char(src, &c, false) && c != '"';) {
		if (size == SIZE_MAX) {
			error(src, "String length exeeds %lu\n", SIZE_MAX);
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
}


void parse_constant_bool(struct source* src, bool* value) {
	char* text = NULL;
	parse_identifier(src, &text);
	if (strcmp(text, "yes") == 0) {
		*value = true;
	} else if (strcmp(text, "no") == 0) {
		*value = false;
	} else {
		error(src, "Unknown value '%s' for bool\n", value);
	}
}

static void parse_constant_float(struct source* src, double* f) {
	/*char c;
	char* buf;
	char* endptr;
	size_t size = 1;
	buf = calloc(1, size); *//* todo can fail *//*
	buf[0] = '\0';
	for (c = '\0'; peek_char(src, &c, true) && (isdigit(c) || c == '_');) {
		if (c == '_') continue;
		if (size == SIZE_MAX) {
			error(src, "Identifier length exeeds %lu\n", SIZE_MAX);
			exit(EXIT_FAILURE);
		}
		size += 1;
		buf = realloc(buf, size); *//* TODO: This can fail *//*
		buf[size - 2] = (consume_char(src, &c, true), c);
		buf[size - 1] = '\0';
	}

	errno = 0; *//* To distinguish success/failure after call *//* *//* TODO *//*
	*i = strtol(buf, &endptr, 10);

	if (errno != 0) {
		error(src, "Failed to convert to number: %s", strerror(errno));
	}
	if (endptr == buf) {
		error(src, "No digits were found in %s\n", buf);
	}

	free(buf);*/
	/* TODO */
	char c;
	while (peek_char(src, &c, true) && (isdigit(c) || c == '.')) {
		consume_char(src, &c, true);
	}
	*f = 1337.0;
}

void parse_vec2(struct source* src, struct vec2i* vec2) {
	char c = '\0';
	char* property = NULL;
	parse_char(src, '{');
	for (peek_char(src, &c, true); c != '}'; peek_char(src, &c, true)) {
		parse_identifier(src, &property);
		if (strcmp(property, "x") == 0) {
			parse_char(src, '=');
			parse_constant_integer(src, &vec2->x);
		} else if (strcmp(property, "y") == 0) {
			parse_char(src, '=');
			parse_constant_integer(src, &vec2->y);
		} else {
			error(src, "Unknown property '%s' for vec2\n", property);
		}
		free(property);
	}
	parse_char(src, '}');
}

void parse_rgb(struct source* src, struct rgb* rgb) {
	parse_char(src, '{');
	parse_constant_float(src, &rgb->r);
	parse_constant_float(src, &rgb->g);
	parse_constant_float(src, &rgb->b);
	parse_char(src, '}');
}

/* endregion */

/* region parse_gfx */
void parse_line_chart(struct source* src, struct sprite_defs* def) {
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
		if (strcmp(property, "name") == 0) {
			parse_constant_string(src, &def->line_chart.name);
		} else if (strcmp(property, "size") == 0) {
			parse_vec2(src, &def->line_chart.size);
		} else if (strcmp(property, "linewidth") == 0) {
			parse_constant_integer(src, &def->line_chart.line_width);
		} else if (strcmp(property, "allwaystransparent") == 0) {
			parse_constant_bool(src, &def->line_chart.always_transparent);
		} else {
			error(src, "Unknown property '%s' for line_chart_type\n", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

void parse_sprite(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_SPRITE;
	def->sprite.name = NULL;
	def->sprite.texture_file = NULL;
	def->sprite.effect_file = NULL;
	def->sprite.no_of_frames = 1;
	def->sprite.always_transparent = false;
	def->sprite.transparency_check = false;
	def->sprite.no_refcount = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcmp(property, "name") == 0) {
			parse_constant_string(src, &def->sprite.name);
		} else if (strcmp(property, "texturefile") == 0) {
			parse_constant_string(src, &def->sprite.texture_file);
		} else if (strcmp(property, "noOfFrames") == 0) {
			parse_constant_integer(src, &def->sprite.no_of_frames);
		} else if (strcmp(property, "allwaystransparent") == 0) {
			parse_constant_bool(src, &def->sprite.always_transparent);
		} else if (strcmp(property, "transparencecheck") == 0) {
			parse_constant_bool(src, &def->sprite.transparency_check);
		} else if (strcmp(property, "norefcount") == 0) {
			parse_constant_bool(src, &def->sprite.no_refcount);
		} else if (strcmp(property, "effectFile") == 0) {
			parse_constant_string(src, &def->sprite.effect_file);
		} else {
			error(src, "Unknown property '%s' for sprite\n", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

void parse_masked_shield(struct source* src, struct sprite_defs* def) {
	char c = '\0';
	def->type = TYPE_MASKED_SHIELD;
	def->masked_shield.name = NULL;
	def->masked_shield.texture_file1 = NULL;
	def->masked_shield.texture_file2 = NULL;
	def->masked_shield.effect_file = false;
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcmp(property, "name") == 0) {
			parse_constant_string(src, &def->masked_shield.name);
		} else if (strcmp(property, "textureFile1") == 0) {
			parse_constant_string(src, &def->masked_shield.texture_file1);
		} else if (strcmp(property, "textureFile2") == 0) {
			parse_constant_string(src, &def->masked_shield.texture_file2);
		} else if (strcmp(property, "effectFile") == 0) {
			parse_constant_string(src, &def->masked_shield.effect_file);
		} else {
			error(src, "Unknown property '%s' for masked shield\n", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

void parse_progress_bar(struct source* src, struct sprite_defs* def) {
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
	parse_str(src, "=");
	parse_str(src, "{");

	peek_char(src, &c, true);
	while (c != '}') {
		char* property = NULL;
		parse_identifier(src, &property);
		parse_str(src, "=");
		if (strcmp(property, "name") == 0) {
			parse_constant_string(src, &def->progress_bar.name);
		} else if (strcmp(property, "color") == 0) {
			parse_rgb(src, &def->progress_bar.color1);
		} else if (strcmp(property, "colortwo") == 0) {
			parse_rgb(src, &def->progress_bar.color2);
		} else if (strcmp(property, "textureFile1") == 0) {
			parse_constant_string(src, &def->progress_bar.texture_file_1);
		} else if (strcmp(property, "textureFile2") == 0) {
			parse_constant_string(src, &def->progress_bar.texture_file_2);
		} else if (strcmp(property, "size") == 0) {
			parse_vec2(src, &def->progress_bar.size);
		} else if (strcmp(property, "effectFile") == 0) {
			parse_constant_string(src, &def->progress_bar.effect_file);
		} else if (strcmp(property, "allwaystransparent") == 0) {
			parse_constant_bool(src, &def->progress_bar.always_transparent);
		} else {
			error(src, "Unknown property '%s' for progress bar\n", property);
		}
		peek_char(src, &c, true);
	}

	parse_str(src, "}");
}

struct sprite_defs* parse_gfx(char const* path) {
	struct source src = {
		.loc = { .lineno = 1, .colno = 1 },
		.bufcap = 1,
		.buf = calloc(1, 1), /* TODO: this can fail */
		.buflen = 0,
		.file = fopen(path, "r"),  /* TODO: this can fail */
	};
	struct sprite_defs* sprite_defs = NULL;
	char c = '\0';

	parse_str(&src, "spriteTypes");
	parse_str(&src, "=");
	parse_str(&src, "{");

	for (peek_char(&src, &c, true); c != '}'; peek_char(&src, &c, true)) {
		struct sprite_defs* sprite_def = calloc(1, sizeof(struct sprite_defs)); /* TODO: this can fail */
		char* type = NULL;
		parse_identifier(&src, &type);
		if (strcmp(type, "LineChartType") == 0) {
			parse_line_chart(&src, sprite_def);
		} else if (strcmp(type, "spriteType") == 0) {
			parse_sprite(&src, sprite_def);
		} else if (strcmp(type, "maskedShieldType") == 0) {
			parse_masked_shield(&src, sprite_def);
		} else if (strcmp(type, "progressbartype") == 0) {
			parse_progress_bar(&src, sprite_def);
		} else  {
			error(&src, "Unknown sprite type: %s\n", type);
		}
		sprite_def->next = sprite_defs;
		sprite_defs = sprite_def;

		free(type);
	}

	parse_str(&src, "}");

	fclose(src.file);
	free(src.buf);
	return sprite_defs;
}
/* endregion */

/* region parse_gui */

struct gui_types* parse_gui(char const* path) {
	struct source src = {
		.loc = { .lineno = 1, .colno = 1 },
		.bufcap = 1,
		.buf = calloc(1, 1), /* TODO: this can fail */
		.buflen = 0,
		.file = fopen(path, "r"),  /* TODO: this can fail */
	};
	struct gui_types* gui_types = NULL;
	char c = '\0';

	parse_str(&src, "guiTypes");
	parse_str(&src, "=");
	parse_str(&src, "{");

	for (peek_char(&src, &c, true); c != '}'; peek_char(&src, &c, true)) {
		struct gui_types* gui_type = calloc(1, sizeof(struct gui_types)); /* TODO: this can fail */
		char* type = NULL;
		parse_identifier(&src, &type);
		if (strcmp(type, "windowType") == 0) {
			/*parse_window(&src, gui_type);*/error(&src, "windowType not implemented yet\n");
		} if (strcmp(type, "iconType") == 0) {
			/*parse_icon(&src, gui_type);*/error(&src, "iconType not implemented yet\n");
		} if (strcmp(type, "guiButtonType") == 0) {
			/*parse_gui_button(&src, gui_type);*/error(&src, "guiButtonType not implemented yet\n");
		} if (strcmp(type, "textBoxType") == 0) {
			/*parse_text_box(&src, gui_type);*/error(&src, "textBoxType not implemented yet\n");
		} if (strcmp(type, "instantTextBoxType") == 0) {
			/*parse_instant_text_box(&src, gui_type);*/error(&src, "instantTextBoxType not implemented yet\n");
		} else {
			error(&src, "Unknown gui type: %s\n", type);
		}
		gui_type->next = gui_types;
		gui_types = gui_type;

		free(type);
	}

	parse_str(&src, "}");

	fclose(src.file);
	free(src.buf);
	return gui_types;
}
/* endregion */