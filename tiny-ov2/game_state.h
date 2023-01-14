#ifndef OV2_GAME_STATE_H
#define OV2_GAME_STATE_H

#include "stdlib.h"

struct game_state {
    size_t province_definitions_count;
    struct province_definition* province_definitions;

    float camera[3];
    float is_dragging;
    int32_t window_width;
    int32_t window_height;

    unsigned int terrain_texture;
    unsigned int provinces_texture;

    unsigned int topbar_texture;
    unsigned int topbar_alert_building_texture;
    unsigned int topbar_alert_decision_texture;
    unsigned int topbar_alert_election_texture;
    unsigned int topbar_alert_factoryclosed_texture;
    unsigned int topbar_alert_greatpower_texture;
    unsigned int topbar_alert_rebels_texture;
    unsigned int topbar_alert_reform_texture;
    unsigned int topbar_alert_sphere_texture;
    unsigned int topbar_alert_unemployed_texture;
    unsigned int topbar_army_texture;
    unsigned int topbar_bg_texture;
    unsigned int topbar_budget_texture;
    unsigned int topbar_button_texture;
    unsigned int topbar_button_budget_texture;
    unsigned int topbar_button_diplo_texture;
    unsigned int topbar_button_military_texture;
    unsigned int topbar_button_politics_texture;
    unsigned int topbar_button_pops_texture;
    unsigned int topbar_button_production_texture;
    unsigned int topbar_button_tech_texture;
    unsigned int topbar_button_trade_texture;
    unsigned int topbar_con_texture;
    unsigned int topbar_diplomacy_texture;
    unsigned int topbar_diplopts_texture;
    unsigned int topbar_export_texture;
    unsigned int topbar_flag_overlay_texture;
    unsigned int topbar_graph_bg_texture;
    unsigned int topbar_import_texture;
    unsigned int topbar_leadership_texture;
    unsigned int topbar_literacy_texture;
    unsigned int topbar_manpower_texture;
    unsigned int topbar_mil_texture;
    unsigned int topbar_military_texture;
    unsigned int topbar_natfocus_texture;
    unsigned int topbar_navy_texture;
    unsigned int topbar_notech_texture;
    unsigned int topbar_paper_texture;
    unsigned int topbar_politics_texture;
    unsigned int topbar_pops_texture;
    unsigned int topbar_prestige_texture;
    unsigned int topbar_production_texture;
    unsigned int topbar_researchpoints_texture;
    unsigned int topbar_tech_texture;
    unsigned int topbar_tech_progress1_texture;
    unsigned int topbar_tech_progress2_texture;
    unsigned int topbar_trade_texture;
    unsigned int topbar_tradeframe_texture;
    unsigned int topbarflag_mask_texture;
    unsigned int topbarflag_shadow_texture;
    unsigned int resources_small_texture;
    unsigned int resources_texture;
    unsigned int resources_big_texture;
};

struct game_state* init_game_state(int32_t window_width, int32_t window_height);

void free_game_state(struct game_state* game_state);

#endif //OV2_GAME_STATE_H
