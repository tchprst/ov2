#ifndef OV2_GAME_STATE_H
#define OV2_GAME_STATE_H

#include "parse.h"
#include <stdlib.h>
#include <GL/gl.h>

enum current_window {
	WINDOW_MAP,
	WINDOW_DIPLOMACY,
	WINDOW_MILITARY,
	WINDOW_POLITICS,
	WINDOW_POPULATION,
	WINDOW_PRODUCTION,
	WINDOW_TECHNOLOGY,
	WINDOW_TRADE,
};

/* TODO: Separate into actual game state and UI state*/
struct game_state {
	size_t province_definitions_count;
	struct province_definition* province_definitions;
	enum current_window current_window;

	float camera[3];
	float is_dragging;
	int32_t window_width;
	int32_t window_height;

	struct sprite_defs* sprite_defs;
	struct gui_defs* gui_defs;

	/* region legacy gui */
	GLuint terrain_texture;
	GLuint provinces_texture;

	GLuint topbar_texture;
	GLuint topbar_alert_building_texture;
	GLuint topbar_alert_decision_texture;
	GLuint topbar_alert_election_texture;
	GLuint topbar_alert_factoryclosed_texture;
	GLuint topbar_alert_greatpower_texture;
	GLuint topbar_alert_rebels_texture;
	GLuint topbar_alert_reform_texture;
	GLuint topbar_alert_sphere_texture;
	GLuint topbar_alert_unemployed_texture;
	GLuint topbar_army_texture;
	GLuint topbar_bg_texture;
	GLuint topbar_budget_texture;
	GLuint topbar_button_texture;
	GLuint topbar_button_budget_texture;
	GLuint topbar_button_diplo_texture;
	GLuint topbar_button_military_texture;
	GLuint topbar_button_politics_texture;
	GLuint topbar_button_pops_texture;
	GLuint topbar_button_production_texture;
	GLuint topbar_button_tech_texture;
	GLuint topbar_button_trade_texture;
	GLuint topbar_con_texture;
	GLuint topbar_diplomacy_texture;
	GLuint topbar_diplopts_texture;
	GLuint topbar_export_texture;
	GLuint topbar_flag_overlay_texture;
	GLuint topbar_graph_bg_texture;
	GLuint topbar_import_texture;
	GLuint topbar_leadership_texture;
	GLuint topbar_literacy_texture;
	GLuint topbar_manpower_texture;
	GLuint topbar_mil_texture;
	GLuint topbar_military_texture;
	GLuint topbar_natfocus_texture;
	GLuint topbar_navy_texture;
	GLuint topbar_notech_texture;
	GLuint topbar_paper_texture;
	GLuint topbar_politics_texture;
	GLuint topbar_pops_texture;
	GLuint topbar_prestige_texture;
	GLuint topbar_production_texture;
	GLuint topbar_researchpoints_texture;
	GLuint topbar_tech_texture;
	GLuint topbar_tech_progress1_texture;
	GLuint topbar_tech_progress2_texture;
	GLuint topbar_trade_texture;
	GLuint topbar_tradeframe_texture;
	GLuint topbarflag_mask_texture;
	GLuint topbarflag_shadow_texture;
	GLuint resources_small_texture;
	GLuint resources_texture;
	GLuint resources_big_texture;
	GLuint background_map_texture;
	GLuint button_speeddown_texture;
	GLuint button_speedup_texture;
	GLuint speed_indicator_texture;
	GLuint flag_den_texture;
	GLuint flag_den_fascist_texture;
	GLuint flag_den_monarchy_texture;
	GLuint flag_den_republic_texture;
	GLuint topbar_flag_shadow_texture;
	GLuint topbar_flag_mask_texture;
	/* endregion */
};

struct game_state* init_game_state(int32_t window_width, int32_t window_height);

void free_game_state(struct game_state* game_state);

#endif //OV2_GAME_STATE_H
