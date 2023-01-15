#include <SOIL/SOIL.h>
#include <stdio.h>
#include "load_textures.h"

static GLuint load_texture(char const* path) {
	GLuint id;
	if ((id = SOIL_load_OGL_texture(
		path,
		SOIL_LOAD_RGBA,
		SOIL_CREATE_NEW_ID,
		0
	)) == 0) {
		fprintf(stderr, "SOIL loading error while loading texture %s: %s\n", path, SOIL_last_result());
	}
	return id;
}

bool load_textures(struct game_state* state) {
	return (state->terrain_texture = load_texture("map/terrain.bmp"))
		&& (state->provinces_texture = load_texture("map/provinces.bmp"))
		&& (state->topbar_texture = load_texture("gfx/interface/topbar.dds")) != 0
		&& (state->topbar_alert_building_texture = load_texture("gfx/interface/topbar_alert_building.dds")) != 0
		&& (state->topbar_alert_decision_texture = load_texture("gfx/interface/topbar_alert_decision.dds")) != 0
		&& (state->topbar_alert_election_texture = load_texture("gfx/interface/topbar_alert_election.dds")) != 0
		&& (state->topbar_alert_factoryclosed_texture = load_texture("gfx/interface/topbar_alert_factoryclosed.dds")) != 0
		&& (state->topbar_alert_greatpower_texture = load_texture("gfx/interface/topbar_alert_greatpower.dds")) != 0
		&& (state->topbar_alert_rebels_texture = load_texture("gfx/interface/topbar_alert_rebels.dds")) != 0
		&& (state->topbar_alert_reform_texture = load_texture("gfx/interface/topbar_alert_reform.dds")) != 0
		&& (state->topbar_alert_sphere_texture = load_texture("gfx/interface/topbar_alert_sphere.dds")) != 0
		&& (state->topbar_alert_unemployed_texture = load_texture("gfx/interface/topbar_alert_unemployed.dds")) != 0
		&& (state->topbar_army_texture = load_texture("gfx/interface/topbar_army.dds")) != 0
		&& (state->topbar_bg_texture = load_texture("gfx/interface/topbar_bg.dds")) != 0
		&& (state->topbar_budget_texture = load_texture("gfx/interface/topbar_budget.dds")) != 0
		&& (state->topbar_button_texture = load_texture("gfx/interface/topbar_button.dds")) != 0
		&& (state->topbar_button_budget_texture = load_texture("gfx/interface/topbar_button_budget.dds")) != 0
		&& (state->topbar_button_diplo_texture = load_texture("gfx/interface/topbar_button_diplo.dds")) != 0
		&& (state->topbar_button_military_texture = load_texture("gfx/interface/topbar_button_military.dds")) != 0
		&& (state->topbar_button_politics_texture = load_texture("gfx/interface/topbar_button_politics.dds")) != 0
		&& (state->topbar_button_pops_texture = load_texture("gfx/interface/topbar_button_pops.dds")) != 0
		&& (state->topbar_button_production_texture = load_texture("gfx/interface/topbar_button_production.dds")) != 0
		&& (state->topbar_button_tech_texture = load_texture("gfx/interface/topbar_button_tech.dds")) != 0
		&& (state->topbar_button_trade_texture = load_texture("gfx/interface/topbar_button_trade.dds")) != 0
		&& (state->topbar_con_texture = load_texture("gfx/interface/topbar_con.dds")) != 0
		&& (state->topbar_diplomacy_texture = load_texture("gfx/interface/topbar_diplomacy.dds")) != 0
		&& (state->topbar_diplopts_texture = load_texture("gfx/interface/topbar_diplopts.dds")) != 0
		&& (state->topbar_export_texture = load_texture("gfx/interface/topbar_export.dds")) != 0
		&& (state->topbar_flag_overlay_texture = load_texture("gfx/interface/topbar_flag_overlay.dds")) != 0
		&& (state->topbar_graph_bg_texture = load_texture("gfx/interface/topbar_graph_bg.tga")) != 0
		&& (state->topbar_import_texture = load_texture("gfx/interface/topbar_import.dds")) != 0
		&& (state->topbar_leadership_texture = load_texture("gfx/interface/topbar_leadership.dds")) != 0
		&& (state->topbar_literacy_texture = load_texture("gfx/interface/topbar_literacy.dds")) != 0
		&& (state->topbar_manpower_texture = load_texture("gfx/interface/topbar_manpower.dds")) != 0
		&& (state->topbar_mil_texture = load_texture("gfx/interface/topbar_mil.dds")) != 0
		&& (state->topbar_military_texture = load_texture("gfx/interface/topbar_military.dds")) != 0
		&& (state->topbar_natfocus_texture = load_texture("gfx/interface/topbar_natfocus.dds")) != 0
		&& (state->topbar_navy_texture = load_texture("gfx/interface/topbar_navy.dds")) != 0
		&& (state->topbar_notech_texture = load_texture("gfx/interface/topbar_notech.dds")) != 0
		&& (state->topbar_paper_texture = load_texture("gfx/interface/topbar_paper.dds")) != 0
		&& (state->topbar_politics_texture = load_texture("gfx/interface/topbar_politics.dds")) != 0
		&& (state->topbar_pops_texture = load_texture("gfx/interface/topbar_pops.dds")) != 0
		&& (state->topbar_prestige_texture = load_texture("gfx/interface/topbar_prestige.dds")) != 0
		&& (state->topbar_production_texture = load_texture("gfx/interface/topbar_production.dds")) != 0
		&& (state->topbar_researchpoints_texture = load_texture("gfx/interface/topbar_researchpoints.dds")) != 0
		&& (state->topbar_tech_texture = load_texture("gfx/interface/topbar_tech.dds")) != 0
		&& (state->topbar_tech_progress1_texture = load_texture("gfx/interface/topbar_tech_progress1.dds")) != 0
		&& (state->topbar_tech_progress2_texture = load_texture("gfx/interface/topbar_tech_progress2.dds")) != 0
		&& (state->topbar_trade_texture = load_texture("gfx/interface/topbar_trade.dds")) != 0
		&& (state->topbar_tradeframe_texture = load_texture("gfx/interface/topbar_tradeframe.dds")) != 0
		&& (state->topbarflag_mask_texture = load_texture("gfx/interface/topbarflag_mask.tga")) != 0
		&& (state->topbarflag_shadow_texture = load_texture("gfx/interface/topbarflag_shadow.dds")) != 0
		&& (state->resources_small_texture = load_texture("gfx/interface/resources_small.dds")) != 0
		&& (state->resources_texture = load_texture("gfx/interface/resources.dds")) != 0
		&& (state->resources_big_texture = load_texture("gfx/interface/resources_big.dds")) != 0
		&& (state->background_map_texture = load_texture("gfx/interface/background_map.dds")) != 0
		&& (state->button_speeddown_texture = load_texture("gfx/interface/button_speeddown.dds")) != 0
		&& (state->button_speedup_texture = load_texture("gfx/interface/button_speedup.dds")) != 0
		&& (state->speed_indicator_texture = load_texture("gfx/interface/speed_indicator.dds")) != 0
		&& (state->flag_den_texture = load_texture("gfx/flags/DEN.tga")) != 0
		&& (state->flag_den_fascist_texture = load_texture("gfx/flags/DEN_fascist.tga")) != 0
		&& (state->flag_den_monarchy_texture = load_texture("gfx/flags/DEN_monarchy.tga")) != 0
		&& (state->flag_den_republic_texture = load_texture("gfx/flags/DEN_republic.tga")) != 0
		&& (state->topbar_flag_shadow_texture = load_texture("gfx/interface/topbarflag_shadow.dds")) != 0
		&& (state->topbar_flag_mask_texture = load_texture("gfx/interface/topbarflag_mask.tga")) != 0;
}