#include <SDL2/SDL.h>
#include "game_tick.h"

void game_tick(struct game_state* state) {
	int32_t wait = (state->speed == 5) ? 0 : (1000 / state->speed);
	if (state->is_paused) return;
	if (SDL_GetTicks() - state->last_game_tick_time < wait) return;
	state->last_game_tick_time = SDL_GetTicks();

	state->day++;
	if (state->day > 30) {
		state->day = 0;
		state->month++;
	}
	if (state->month > 11) {
		state->month = 0;
		state->year++;
	}
}