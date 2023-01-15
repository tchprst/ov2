#include "game_state.h"
#include "province_definitions.h"
#include "load_textures.h"
#include "parse.h"
#include "fs.h"
#include <GL/gl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

struct game_state* init_game_state(int32_t window_width, int32_t window_height) {
	bool success = true;
	struct game_state* state = malloc(sizeof(struct game_state));
	if (state == NULL) {
		fprintf(stderr, "Failed to allocate memory for game state\n");
	} else if (load_province_definitions(
		&state->province_definitions,
		&state->province_definitions_count
	), state->province_definitions == NULL) {
		fprintf(stderr, "Failed to load province definitions\n");
		success = false;
	} else if (!load_textures(state)) {
		fprintf(stderr, "Failed to load textures\n");
		success = false;
	} else {
		state->current_window = WINDOW_MAP;
		state->camera[0] = 0.0f;
		state->camera[1] = 0.0f;
		state->camera[2] = 1.0f;
		state->is_dragging = false;
		state->window_width = window_width;
		state->window_height = window_height;
		state->gui_defs = NULL;
		state->sprite_defs = NULL;

		{
			DIR* dir;
			struct dirent* entry;
			if ((dir = opendir("interface")) == NULL) {
				fprintf(stderr, "Failed to open interface directory\n");
				success = false;
			} else {
				while ((entry = readdir(dir)) != NULL) {
					if (entry->d_type == DT_REG) {
						char* path = malloc(strlen("interface/") + strlen(entry->d_name) + 1);
						if (path == NULL) {
							fprintf(stderr, "Failed to allocate memory for path\n");
							success = false;
							break;
						}
						strcpy(path, "interface/");
						strcat(path, entry->d_name);
						if (has_ext(path, ".gfx") || has_ext(path, ".gui")) {
							parse(path, &state->sprite_defs, &state->gui_defs);
						}
						free(path);
					}
				}
				closedir(dir);
			}
		}
	}

	if (!success) {
		free_game_state(state);
		state = NULL;
	}

	return state;
}

void free_game_state(struct game_state* game_state) {
	glDeleteTextures(1, &game_state->terrain_texture);
	glDeleteTextures(1, &game_state->provinces_texture);

	glDeleteTextures(1, &game_state->topbar_texture);
	glDeleteTextures(1, &game_state->topbar_alert_building_texture);
	glDeleteTextures(1, &game_state->topbar_alert_decision_texture);
	glDeleteTextures(1, &game_state->topbar_alert_election_texture);
	glDeleteTextures(1, &game_state->topbar_alert_factoryclosed_texture);
	glDeleteTextures(1, &game_state->topbar_alert_greatpower_texture);
	glDeleteTextures(1, &game_state->topbar_alert_rebels_texture);
	glDeleteTextures(1, &game_state->topbar_alert_reform_texture);
	glDeleteTextures(1, &game_state->topbar_alert_sphere_texture);
	glDeleteTextures(1, &game_state->topbar_alert_unemployed_texture);
	glDeleteTextures(1, &game_state->topbar_army_texture);
	glDeleteTextures(1, &game_state->topbar_bg_texture);
	glDeleteTextures(1, &game_state->topbar_budget_texture);
	glDeleteTextures(1, &game_state->topbar_button_texture);
	glDeleteTextures(1, &game_state->topbar_button_budget_texture);
	glDeleteTextures(1, &game_state->topbar_button_diplo_texture);
	glDeleteTextures(1, &game_state->topbar_button_military_texture);
	glDeleteTextures(1, &game_state->topbar_button_politics_texture);
	glDeleteTextures(1, &game_state->topbar_button_pops_texture);
	glDeleteTextures(1, &game_state->topbar_button_production_texture);
	glDeleteTextures(1, &game_state->topbar_button_tech_texture);
	glDeleteTextures(1, &game_state->topbar_button_trade_texture);
	glDeleteTextures(1, &game_state->topbar_con_texture);
	glDeleteTextures(1, &game_state->topbar_diplomacy_texture);
	glDeleteTextures(1, &game_state->topbar_diplopts_texture);
	glDeleteTextures(1, &game_state->topbar_export_texture);
	glDeleteTextures(1, &game_state->topbar_flag_overlay_texture);
	glDeleteTextures(1, &game_state->topbar_graph_bg_texture);
	glDeleteTextures(1, &game_state->topbar_import_texture);
	glDeleteTextures(1, &game_state->topbar_leadership_texture);
	glDeleteTextures(1, &game_state->topbar_literacy_texture);
	glDeleteTextures(1, &game_state->topbar_manpower_texture);
	glDeleteTextures(1, &game_state->topbar_mil_texture);
	glDeleteTextures(1, &game_state->topbar_military_texture);
	glDeleteTextures(1, &game_state->topbar_natfocus_texture);
	glDeleteTextures(1, &game_state->topbar_navy_texture);
	glDeleteTextures(1, &game_state->topbar_notech_texture);
	glDeleteTextures(1, &game_state->topbar_paper_texture);
	glDeleteTextures(1, &game_state->topbar_politics_texture);
	glDeleteTextures(1, &game_state->topbar_pops_texture);
	glDeleteTextures(1, &game_state->topbar_prestige_texture);
	glDeleteTextures(1, &game_state->topbar_production_texture);
	glDeleteTextures(1, &game_state->topbar_researchpoints_texture);
	glDeleteTextures(1, &game_state->topbar_tech_texture);
	glDeleteTextures(1, &game_state->topbar_tech_progress1_texture);
	glDeleteTextures(1, &game_state->topbar_tech_progress2_texture);
	glDeleteTextures(1, &game_state->topbar_trade_texture);
	glDeleteTextures(1, &game_state->topbar_tradeframe_texture);
	glDeleteTextures(1, &game_state->topbarflag_mask_texture);
	glDeleteTextures(1, &game_state->topbarflag_shadow_texture);
	glDeleteTextures(1, &game_state->resources_small_texture);
	glDeleteTextures(1, &game_state->resources_texture);
	glDeleteTextures(1, &game_state->resources_big_texture);
	glDeleteTextures(1, &game_state->background_map_texture);
	glDeleteTextures(1, &game_state->button_speeddown_texture);
	glDeleteTextures(1, &game_state->button_speedup_texture);
	glDeleteTextures(1, &game_state->speed_indicator_texture);
	glDeleteTextures(1, &game_state->flag_den_texture);
	glDeleteTextures(1, &game_state->flag_den_fascist_texture);
	glDeleteTextures(1, &game_state->flag_den_monarchy_texture);
	glDeleteTextures(1, &game_state->flag_den_republic_texture);
	glDeleteTextures(1, &game_state->topbar_flag_shadow_texture);
	glDeleteTextures(1, &game_state->topbar_flag_mask_texture);

	free_sprites(game_state->sprite_defs);
	free_gui(game_state->gui_defs);

	free(game_state);
}