#include "game_state.h"
#include "province_definitions.h"
#include "parse.h"
#include "fs.h"
#include <GL/gl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SOIL/SOIL.h>

struct game_state* init_game_state(int32_t window_width, int32_t window_height) {
	bool success = true;
	struct game_state* state = malloc(sizeof(struct game_state));
	if (state == NULL) {
		fprintf(stderr, "Failed to allocate memory for game state.\n");
	} else if (load_province_definitions(
		&state->province_definitions,
		&state->province_definitions_count
	), state->province_definitions == NULL) {
		fprintf(stderr, "Failed to load province definitions.\n");
		success = false;
	} else if ((state->provinces_texture = SOIL_load_OGL_texture(
		"map/provinces.bmp",
		SOIL_LOAD_RGBA,
		SOIL_CREATE_NEW_ID,
		0
	)) == 0) {
		fprintf(stderr, "SOIL loading error while loading texture %s: "
				"%s\n", "map/provinces.bmp", SOIL_last_result());
		success = false;
	} else {
		state->current_window = WINDOW_MAP;
		state->is_paused = true;
		state->speed = 1;
		state->year = 1835;
		state->month = 0;
		state->day = 0;
		state->camera[0] = 0.0f;
		state->camera[1] = 0.0f;
		state->camera[2] = 1.0f;
		state->is_dragging = false;
		state->window_width = window_width;
		state->window_height = window_height;
		state->widgets = NULL;
		state->sprites = NULL;

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
							fprintf(stderr, "Failed to allocate memory for path.\n");
							success = false;
							break;
						}
						strcpy(path, "interface/");
						strcat(path, entry->d_name);
						if (has_ext(path, ".gfx") || has_ext(path, ".gui")) {
							parse(path, &state->sprites, &state->widgets);
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
	free_province_definitions(
		game_state->province_definitions,
		game_state->province_definitions_count
	);
	free_sprites(game_state->sprites);
	free_widgets(game_state->widgets);
	glDeleteTextures(1, &game_state->provinces_texture);

	free(game_state);
}