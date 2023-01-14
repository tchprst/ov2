#ifndef OV2_PARSE_H
#define OV2_PARSE_H

#include <stdbool.h>
#include <stdint.h>

struct window_type {

};

struct icon_type {

};

struct position_type {

};

struct gui_types {
	struct gui_types* next;
	enum {
		GUI_TYPE_WINDOW,
		GUI_TYPE_POSITION,
		GUI_TYPE_ICON
	} type;
	union {
		struct window_type window;
		struct position_type position;
		struct icon_type icon;
	};
};

struct sprite_definition {
	char* name;
	char* texture_file;
	char* effect_file;
	int64_t no_of_frames;
	bool always_transparent;
	bool transparency_check;
	bool no_refcount;
};

struct vec2i {
	int64_t x;
	int64_t y;
};

struct line_chart_definition {
	char* name;
	struct vec2i size;
	int64_t line_width;
	bool always_transparent;
};

struct masked_shield_definition {
	char* name;
	char* texture_file1;
	char* texture_file2;
	char* effect_file;
};

struct rgb {
	double r;
	double g;
	double b;
};

struct progress_bar_definition {
	char* name;
	struct rgb color1;
	struct rgb color2;
	char* texture_file_1;
	char* texture_file_2;
	struct vec2i size;
	char* effect_file;
	bool always_transparent;
};

struct sprite_defs {
	struct sprite_defs* next;
	enum {
		TYPE_SPRITE,
		TYPE_LINE_CHART,
		TYPE_MASKED_SHIELD,
		TYPE_PROGRESS_BAR,
	} type;
	union {
		struct sprite_definition sprite;
		struct line_chart_definition line_chart;
		struct masked_shield_definition masked_shield;
		struct progress_bar_definition progress_bar;
	};
};

struct gui_types* parse_gui(char const* path);

struct sprite_defs* parse_gfx(char const* path);

#endif //OV2_PARSE_H
