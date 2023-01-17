#include "ui_event.h"
#include <SDL2/SDL.h>

static void speed_up(struct game_state* state) {
	if (state->speed < 5) state->speed++;
}

static void speed_down(struct game_state* state) {
	if (state->speed > 1) state->speed--;
}

static void pause(struct game_state* state) {
	state->is_paused = !state->is_paused;
}

static char const* button_pressed = NULL;

/* returns false if a quit has been requested */
static bool handle_key_down(struct game_state* state, SDL_Keysym* keysym) {
	bool should_quit = false;

	switch (keysym->sym) {
	case SDLK_ESCAPE:
	case SDLK_q:
		should_quit = true;
		break;
	case SDLK_SPACE:
		state->is_paused = !state->is_paused;
		break;
	case SDLK_KP_PLUS:
		speed_up(state);
		break;
	case SDLK_KP_MINUS:
		speed_down(state);
		break;
	default:
		break;
	}

	return should_quit;
}

static void handle_mouse_button_down(struct game_state* state, SDL_MouseButtonEvent* button) {
	/* TODO: Make this actually recursive. */
	struct ui_widget* widget = NULL;
	if (button->button != SDL_BUTTON_LEFT) return;
	for (widget = state->widgets; widget != NULL; widget = widget->next) {
		if (widget->type == TYPE_WINDOW) {
			struct ui_widget* child = NULL;
			for (child = widget->window.children; child != NULL; child = child->next) {
				if (child->type == TYPE_BUTTON) {
					if (button->x >= child->position.x && button->x <= child->position.x + child->size.x &&
					    button->y >= child->position.y && button->y <= child->position.y + child->size.y) {
						button_pressed = child->name;
					}
				}
			}
		} else if (widget->type == TYPE_BUTTON) {
			if (button->x >= widget->position.x && button->x <= widget->position.x + widget->size.x &&
			    button->y >= widget->position.y && button->y <= widget->position.y + widget->size.y) {
				button_pressed = widget->name;
			}
		}
	}
}

static void handle_mouse_button_up(struct game_state* state, SDL_MouseButtonEvent* button) {
	/* TODO: Make this actually recursive. */
	struct ui_widget* widget = NULL;
	if (button->button != SDL_BUTTON_LEFT) return;
	for (widget = state->widgets; widget != NULL; widget = widget->next) {
		if (widget->type == TYPE_WINDOW) {
			struct ui_widget* child = NULL;
			for (child = widget->window.children; child != NULL; child = child->next) {
				if (child->type == TYPE_BUTTON) {
					if (button->x >= child->position.x && button->x <= child->position.x + child->size.x &&
					    button->y >= child->position.y && button->y <= child->position.y + child->size.y) {
						if (button_pressed == child->name) {
							if (strcmp(button_pressed, "button_speedup") == 0) {
								speed_up(state);
							} else if (strcmp(button_pressed, "button_speeddown") == 0) {
								speed_down(state);
							} else if (strcmp(button_pressed, "pause_bg") == 0 || strcmp(button_pressed, "speed_indicator") == 0) {
								pause(state);
							} else if (strcmp(button_pressed, "topbarbutton_production") == 0) {
								if (state->current_window == WINDOW_PRODUCTION) {
									state->current_window = WINDOW_MAP;
								} else {
									state->current_window = WINDOW_PRODUCTION;
								}
							} else if (strcmp(button_pressed, "topbarbutton_budget") == 0) {
								if (state->current_window == WINDOW_BUDGET) {
									state->current_window = WINDOW_MAP;
								} else {
									state->current_window = WINDOW_BUDGET;
								}
							} else {
								fprintf(stderr, "button %s pressed\n", child->name);
							}
						}
					}
				}
			}
		} else if (widget->type == TYPE_BUTTON) {
			if (button->x >= widget->position.x && button->x <= widget->position.x + widget->size.x &&
			    button->y >= widget->position.y && button->y <= widget->position.y + widget->size.y) {
				if (button_pressed == widget->name) {
					fprintf(stderr, "button %s pressed\n", widget->name);
				}
			}
		}
	}
	button_pressed = NULL;
}

void handle_events(struct game_state* state) {
	SDL_Event event;

	while (SDL_PollEvent(&event))
		switch (event.type) {
		case SDL_KEYDOWN:
			if (handle_key_down(state, &event.key.keysym)) {
				state->should_quit = true;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (state->current_window == WINDOW_MAP && event.button.button == SDL_BUTTON_MIDDLE) {
				state->is_dragging = true;
			} else {
				handle_mouse_button_down(state, &event.button);
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (state->current_window == WINDOW_MAP && event.button.button == SDL_BUTTON_MIDDLE) {
				state->is_dragging = false;
			} else {
				handle_mouse_button_up(state, &event.button);
			}
			break;
		case SDL_MOUSEMOTION:
			if (state->is_dragging) {
				state->camera[0] += (float) event.motion.xrel / (float) state->window_width * 2.0f;
				state->camera[1] -= (float) event.motion.yrel / (float) state->window_height * 2.0f;
			}
			break;
		case SDL_MOUSEWHEEL: {
			int32_t i;
			float mouse_x, mouse_y, offset_x, offset_y;
			float previous_scale = state->camera[2];
			if (event.wheel.y > 0) {
				for (i = 0; i < event.wheel.y; i++) {
					state->camera[2] *= 1.1f;
				}
			} else if (event.wheel.y < 0) {
				for (i = 0; i < -event.wheel.y; i++) {
					state->camera[2] /= 1.1f;
				}
			}
			if (state->camera[2] < 0.1f) {
				state->camera[2] = 0.1f;
			}
			if (state->camera[2] > 20.0f) {
				state->camera[2] = 20.0f;
			}

			/* Zoom relative to mouse cursor. */
			mouse_x = (float) event.wheel.mouseX / (float) state->window_width * 2.0f - 1.0f;
			mouse_y = (float) event.wheel.mouseY / (float) state->window_height * 2.0f - 1.0f;
			mouse_y = -mouse_y;
			offset_x = (mouse_x - state->camera[0]) * state->camera[2] / previous_scale;
			offset_y = (mouse_y - state->camera[1]) * state->camera[2] / previous_scale;
			state->camera[0] = mouse_x - offset_x;
			state->camera[1] = mouse_y - offset_y;
			break;
		}
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				state->window_width = event.window.data1;
				state->window_height = event.window.data2;
				glViewport(0, 0, event.window.data1, event.window.data2);
			}
			break;
		case SDL_QUIT:
			state->should_quit = true;
			break;
		}
}