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
	CLICK_SOUND_START_GAME
};

/* region sprites */

enum sprite_load_type {
	SPRITE_LOAD_TYPE_INGAME,
	SPRITE_LOAD_TYPE_BACKEND,
	SPRITE_LOAD_TYPE_FRONTEND
};

struct simple_sprite {
	char* texture_file;
	char* effect_file;
	int64_t no_of_frames;
	bool always_transparent;
	bool transparency_check;
	bool no_refcount;
	enum click_sound click_sound;
	enum sprite_load_type load_type;
};

struct line_chart {
	struct vec2i size;
	int64_t line_width;
	bool always_transparent;
};

struct masked_shield {
	char* texture_file1;
	char* texture_file2;
	char* effect_file;
	bool always_transparent;
	bool flipv;
};

struct progress_bar {
	struct rgb color1;
	struct rgb color2;
	char* texture_file_1;
	char* texture_file_2;
	struct vec2i size;
	char* effect_file;
	bool always_transparent;
	bool horizontal;
	enum sprite_load_type load_type;
};

struct cornered_tile_sprite {
	struct vec2i size;
	char* texture_file;
	struct vec2i border_size;
	enum sprite_load_type load_type;
	bool always_transparent;
};

struct text_sprite {
	char* texture_file;
	int64_t no_of_frames;
	char* effect_file;
	bool no_refcount;
	enum sprite_load_type load_type;
	enum click_sound click_sound;
};

struct bar_chart {
	struct vec2i size;
};

struct pie_chart {
	int64_t size;
};

struct tile_sprite {
	char* texture_file;
	char* effect_file;
	enum sprite_load_type load_type;
	bool no_refcount;
	struct vec2i size;
};

struct scrolling_sprite {
	char* texture_file1;
	struct vec2i size;
	char* effect_file;
	int64_t step;
	bool always_transparent;
};

struct sprite {
	char* name;
	enum {
		TYPE_SIMPLE_SPRITE,
		TYPE_LINE_CHART,
		TYPE_MASKED_SHIELD,
		TYPE_PROGRESS_BAR,
		TYPE_CORNERED_TILE_SPRITE,
		TYPE_TEXT_SPRITE,
		TYPE_BAR_CHART,
		TYPE_PIE_CHART,
		TYPE_TILE_SPRITE,
		TYPE_SCROLLING_SPRITE
	} type;
	union {
		struct simple_sprite sprite;
		struct line_chart line_chart;
		struct masked_shield masked_shield;
		struct progress_bar progress_bar;
		struct cornered_tile_sprite cornered_tile_sprite;
		struct text_sprite text_sprite;
		struct bar_chart bar_chart;
		struct pie_chart pie_chart;
		struct tile_sprite tile_sprite;
		struct scrolling_sprite scrolling_sprite;
	};
	struct sprite* next;
};

/* endregion */

/* region ui */

struct ui_widget;

enum ui_orientation {
	UI_ORIENTATION_LOWER_LEFT,
	UI_ORIENTATION_UPPER_LEFT,
	UI_ORIENTATION_CENTER_UP,
	UI_ORIENTATION_CENTER,
	UI_ORIENTATION_CENTER_DOWN,
	UI_ORIENTATION_UPPER_RIGHT,
	UI_ORIENTATION_LOWER_RIGHT
};

enum ui_format {
	UI_FORMAT_LEFT,
	UI_FORMAT_CENTER,
	UI_FORMAT_RIGHT,
	UI_FORMAT_JUSTIFIED
};

struct ui_window {
	char* background;
	bool movable;
	char* dont_render;
	char* horizontal_border;
	char* vertical_border;
	bool full_screen;
	struct ui_widget* children;
	enum ui_orientation orientation;
	char* up_sound;
	char* down_sound;
};

struct ui_icon {
	char* sprite;
	enum ui_orientation orientation;
	int64_t frame;
	char* button_mesh;
	double rotation;
	double scale;
};

struct ui_button {
	char* quad_texture_sprite;
	char* button_text;
	char* button_font;
	char* shortcut;
	enum click_sound click_sound;
	enum ui_orientation orientation;
	char* tooltip;
	char* tooltip_text;
	char* delayed_tooltip_text;
	char* sprite_type;
	char* parent;
	double rotation;
	enum ui_format format;
};

struct ui_text_box {
	char* font;
	struct vec2i border_size;
	char* text;
	int64_t max_width;
	int64_t max_height;
	enum ui_format format;
	bool fixed_size;
	char* texture_file;
	enum ui_orientation orientation;
};

struct ui_instant_text_box {
	char* font;
	struct vec2i border_size;
	char* text;
	int64_t max_width;
	int64_t max_height;
	enum ui_format format;
	bool fixed_size;
	char* texture_file;
	enum ui_orientation orientation;
	bool always_transparent;
};

struct ui_overlapping_elements_box {
	enum ui_orientation orientation;
	enum ui_format format;
	double spacing;
};

struct ui_scrollbar {
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
	struct ui_widget* children;
};

struct ui_checkbox {
	char* quad_texture_sprite;
	char* tooltip;
	char* tooltip_text;
	char* delayed_tooltip_text;
	char* button_text;
	char* button_font;
	enum ui_orientation orientation;
	char* shortcut;
};

struct ui_edit_box {
	char* texture_file;
	char* font;
	struct vec2i border_size;
	char* text;
	enum ui_orientation orientation;
};

struct ui_list_box {
	char* background;
	enum ui_orientation orientation;
	int64_t spacing;
	char* scrollbar_type;
	struct vec2i border_size;
	int64_t priority;
	int64_t step;
	bool horizontal;
	struct vec2i offset;
	bool always_transparent;
};

struct ui_eu3_dialog {
	char* background;
	bool movable;
	char* dont_render;
	char* horizontal_border;
	char* vertical_border;
	bool full_screen;
	enum ui_orientation orientation;
	struct ui_widget* children;
};

struct ui_shield {
	char* sprite_type;
	double rotation;
};

struct ui_widget {
	struct ui_widget* next;
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
		TYPE_POSITION
	} type;
	union {
		struct ui_window window;
		struct ui_icon icon;
		struct ui_button button;
		struct ui_text_box text_box;
		struct ui_instant_text_box instant_text_box;
		struct ui_overlapping_elements_box overlapping_elements_box;
		struct ui_scrollbar scrollbar;
		struct ui_checkbox checkbox;
		struct ui_edit_box edit_box;
		struct ui_list_box list_box;
		struct ui_eu3_dialog eu3_dialog;
		struct ui_shield shield;
	};
};

/* endregion */

void parse(char const* path, struct sprite** sprites, struct ui_widget** widgets);

void free_sprites(struct sprite* sprites);

void free_widgets(struct ui_widget* widgets);

#endif /*OV2_PARSE_H*/
