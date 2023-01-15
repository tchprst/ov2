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

enum click_sound {
	CLICK_SOUND_CLICK,
	CLICK_SOUND_CLOSE_WINDOW,
	CLICK_SOUND_START_GAME,
};

/* region sprites */

enum load_type {
	LOAD_TYPE_INGAME,
	LOAD_TYPE_BACKEND,
	LOAD_TYPE_FRONTEND,
};

struct sprite_def {
	char* texture_file;
	char* effect_file;
	int64_t no_of_frames;
	bool always_transparent;
	bool transparency_check;
	bool no_refcount;
	enum click_sound click_sound;
	enum load_type load_type;
};

struct line_chart_def {
	struct vec2i size;
	int64_t line_width;
	bool always_transparent;
};

struct masked_shield_def {
	char* texture_file1;
	char* texture_file2;
	char* effect_file;
	bool always_transparent;
	bool flipv;
};

struct progress_bar_def {
	struct rgb color1;
	struct rgb color2;
	char* texture_file_1;
	char* texture_file_2;
	struct vec2i size;
	char* effect_file;
	bool always_transparent;
	bool horizontal;
	enum load_type load_type;
};

struct cornered_tile_sprite_def {
	struct vec2i size;
	char* texture_file;
	struct vec2i border_size;
	enum load_type load_type;
	bool always_transparent;
};

struct text_sprite_def {
	char* texture_file;
	int64_t no_of_frames;
	char* effect_file;
	bool no_refcount;
	enum load_type load_type;
	enum click_sound click_sound;
};

struct bar_chart_def {
	struct vec2i size;
};

struct pie_chart_def {
	int64_t size;
};

struct tile_sprite_def {
	char* texture_file;
	char* effect_file;
	enum load_type load_type;
	bool no_refcount;
	struct vec2i size;
};

struct scrolling_sprite_def {
	char* texture_file1;
	struct vec2i size;
	char* effect_file;
	int64_t step;
	bool always_transparent;
};

struct sprite_defs {
	struct sprite_defs* next;
	char* name;
	enum {
		TYPE_SPRITE,
		TYPE_LINE_CHART,
		TYPE_MASKED_SHIELD,
		TYPE_PROGRESS_BAR,
		TYPE_CORNERED_TILE_SPRITE,
		TYPE_TEXT_SPRITE,
		TYPE_BAR_CHART,
		TYPE_PIE_CHART,
		TYPE_TILE_SPRITE,
		TYPE_SCROLLING_SPRITE,
	} type;
	union {
		struct sprite_def sprite;
		struct line_chart_def line_chart;
		struct masked_shield_def masked_shield;
		struct progress_bar_def progress_bar;
		struct cornered_tile_sprite_def cornered_tile_sprite;
		struct text_sprite_def text_sprite;
		struct bar_chart_def bar_chart;
		struct pie_chart_def pie_chart;
		struct tile_sprite_def tile_sprite;
		struct scrolling_sprite_def scrolling_sprite;
	};
};

/* endregion */

/* region gui */

struct gui_defs;

enum gui_orientation {
	ORIENTATION_LOWER_LEFT,
	ORIENTATION_UPPER_LEFT,
	ORIENTATION_CENTER_UP,
	ORIENTATION_CENTER,
	ORIENTATION_CENTER_DOWN,
	ORIENTATION_UPPER_RIGHT,
	ORIENTATION_LOWER_RIGHT,
};

enum gui_format {
	GUI_FORMAT_LEFT,
	GUI_FORMAT_CENTER,
	GUI_FORMAT_RIGHT,
	GUI_FORMAT_JUSTIFIED,
};

struct window_def {
	char* background;
	bool movable;
	char* dont_render;
	char* horizontal_border;
	char* vertical_border;
	bool full_screen;
	struct gui_defs* children;
	enum gui_orientation orientation;
	char* up_sound;
	char* down_sound;
};

struct icon_def {
	char* sprite;
	enum gui_orientation orientation;
	int64_t frame;
	char* button_mesh;
	double rotation;
	double scale;
};

struct button_def {
	char* quad_texture_sprite;
	char* button_text;
	char* button_font;
	char* shortcut;
	enum click_sound click_sound;
	enum gui_orientation orientation;
	char* tooltip;
	char* tooltip_text;
	char* delayed_tooltip_text;
	char* sprite_type;
	char* parent;
	double rotation;
	enum gui_format format;
};

struct text_box_def {
	struct vec2i position;
	char* font;
	struct vec2i border_size;
	char* text;
	int64_t max_width;
	int64_t max_height;
	enum gui_format format;
	bool fixed_size;
	char* texture_file;
	enum gui_orientation orientation;
};

struct instant_text_box_def {
	struct vec2i position;
	char* font;
	struct vec2i border_size;
	char* text;
	int64_t max_width;
	int64_t max_height;
	enum gui_format format;
	bool fixed_size;
	enum gui_orientation orientation;
	char* texture_file;
	bool always_transparent;
};

struct overlapping_elements_box_def {
	enum gui_orientation orientation;
	enum gui_format format;
	double spacing;
};

struct scrollbar_def {
	char* slider;
	char* track;
	char* left_button;
	char* right_button;
	int64_t priority;
	struct vec2i border_size;
	double max_value;
	double min_value;
	double step_size;
	double start_value;
	bool horizontal;
	bool use_range_limit;
	double range_limit_min;
	double range_limit_max;
	char* range_limit_min_icon;
	char* range_limit_max_icon;
	bool lockable;
	struct gui_defs* children;
};

struct checkbox_def {
	struct vec2i position;
	char* quad_texture_sprite;
	char* tooltip;
	char* tooltip_text;
	char* delayed_tooltip_text;
	char* button_text;
	char* button_font;
	enum gui_orientation orientation;
	char* shortcut;
};

struct edit_box_def {
	char* texture_file;
	char* font;
	struct vec2i border_size;
	char* text;
	enum gui_orientation orientation;
};

struct list_box_def {
	char* background;
	enum gui_orientation orientation;
	int64_t spacing;
	char* scrollbar_type;
	struct vec2i border_size;
	int64_t priority;
	int64_t step;
	bool horizontal;
	struct vec2i offset;
	bool always_transparent;
};

struct eu3_dialog_def {
	char* background;
	bool movable;
	char* dont_render;
	char* horizontal_border;
	char* vertical_border;
	bool full_screen;
	enum gui_orientation orientation;
	struct gui_defs* children;
};

struct shield_def {
	char* sprite_type;
	double rotation;
};

struct gui_defs {
	struct gui_defs* next;
	char* name;
	struct vec2i position;
	struct vec2i size;
	enum {
		TYPE_WINDOW,
		TYPE_ICON,
		TYPE_BUTTON,
		TYPE_TEXT_BOX,
		TYPE_INSTANT_TEXT_BOX,
		TYPE_OVERLAPPING_ELEMENTS_BOX,
		TYPE_SCROLLBAR,
		TYPE_CHECKBOX,
		TYPE_EDIT_BOX,
		TYPE_LIST_BOX,
		TYPE_EU3_DIALOG,
		TYPE_SHIELD,
		TYPE_POSITION,
	} type;
	union {
		struct window_def window;
		struct icon_def icon;
		struct button_def button;
		struct text_box_def text_box;
		struct instant_text_box_def instant_text_box;
		struct overlapping_elements_box_def overlapping_elements_box;
		struct scrollbar_def scrollbar;
		struct checkbox_def checkbox;
		struct edit_box_def edit_box;
		struct list_box_def list_box;
		struct eu3_dialog_def eu3_dialog;
		struct shield_def shield;
	};
};

/* endregion */

void parse(char const* path, struct sprite_defs** gfx_defs, struct gui_defs** gui_defs);

void free_sprites(struct sprite_defs* gfx_defs);

void free_gui(struct gui_defs* gui_defs);

#endif //OV2_PARSE_H
