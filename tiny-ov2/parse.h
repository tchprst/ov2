#ifndef OV2_PARSE_H
#define OV2_PARSE_H

#include <stdbool.h>
#include <stdint.h>

struct rgb {
	double r;
	double g;
	double b;
};

struct vec2i {
	int64_t x;
	int64_t y;
};

/* region gfx */

struct sprite_definition {
	char* name;
	char* texture_file;
	char* effect_file;
	int64_t no_of_frames;
	bool always_transparent;
	bool transparency_check;
	bool no_refcount;
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

/* endregion */

/* region gui */

struct gui_defs;


struct window_def {
	char* name;
	char* background;
	struct vec2i position;
	struct vec2i size;
	int64_t moveable;
	char* dont_render;
	char* horizontal_border;
	char* vertical_border;
	bool full_screen;
	struct gui_defs* children;
};

struct icon_def {
	char* name;
	char* sprite;
	struct vec2i position;
	char* orientation;
	int64_t frame;
};

enum click_sound {
	CLICK_SOUND_CLICK,
	CLICK_SOUND_CLOSE_WINDOW,
};

struct gui_button_def {
	char* name;
	struct vec2i position;
	char* quad_texture_sprite;
	char* button_text;
	char* button_font;
	char* shortcut;
	enum click_sound click_sound;
	char* orientation;
	char* tooltip;
	char* tooltip_text;
	char* delayed_tooltip_text;
};

enum gui_format {
	GUI_FORMAT_LEFT,
	GUI_FORMAT_CENTER,
	GUI_FORMAT_RIGHT,
};

struct text_box_def {
	char* name;
	struct vec2i position;
	char* font;
	struct vec2i border_size;
	char* text;
	int64_t max_width;
	int64_t max_height;
	enum gui_format format;
	bool fixed_size;
};

struct instant_text_box_def {
	char* name;
	struct vec2i position;
	char* font;
	struct vec2i border_size;
	char* text;
	int64_t max_width;
	int64_t max_height;
	enum gui_format format;
	bool fixed_size;
	char* orientation;
	char* texture_file;
};

struct overlapping_elements_box_def {
	char* name;
	struct vec2i position;
	struct vec2i size;
	char* orientation;
	enum gui_format format;
	double spacing;
};

struct gui_defs {
	struct gui_defs* next;
	enum {
		TYPE_WINDOW,
		TYPE_ICON,
		TYPE_GUI_BUTTON,
		TYPE_TEXT_BOX,
		TYPE_INSTANT_TEXT_BOX,
		TYPE_OVERLAPPING_ELEMENTS_BOX
	} type;
	union {
		struct window_def window;
		struct icon_def icon;
		struct gui_button_def gui_button;
		struct text_box_def text_box;
		struct instant_text_box_def instant_text_box;
		struct overlapping_elements_box_def overlapping_elements_box;
	};
};

/* endregion */

struct sprite_defs* parse_gfx(char const* path);

struct gui_defs* parse_gui(char const* path);

#endif //OV2_PARSE_H
