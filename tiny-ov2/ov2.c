#include "province_definitions.h"
#include "game_state.h"
#include "legacy_ui.h"
#include "parse.h"
#include "fs.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SOIL/SOIL.h> /* TODO: Replace SOIL with SDL_image */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int debug_x = 0;
int debug_y = 0;

/* returns false if a quit has been requested */
static bool handle_key_down(SDL_Keysym* keysym) {
	bool should_quit = false;

	switch (keysym->sym) {
	case SDLK_ESCAPE:
	case SDLK_q:
		should_quit = true;
		break;
	case SDLK_UP:
		debug_y--;
		fprintf(stderr, "debug_x: %d, debug_y: %d\n", debug_x, debug_y);
		break;
	case SDLK_DOWN:
		debug_y++;
		fprintf(stderr, "debug_x: %d, debug_y: %d\n", debug_x, debug_y);
		break;
	case SDLK_LEFT:
		debug_x--;
		fprintf(stderr, "debug_x: %d, debug_y: %d\n", debug_x, debug_y);
		break;
	case SDLK_RIGHT:
		debug_x++;
		fprintf(stderr, "debug_x: %d, debug_y: %d\n", debug_x, debug_y);
		break;
	default:
		break;
	}

	return should_quit;
}

/* returns false if a quit has been requested */
static bool handle_events(struct game_state* state) {
	bool should_quit = false;
	SDL_Event event;

	while (SDL_PollEvent(&event))
		switch (event.type) {
			case SDL_KEYDOWN:
				if (handle_key_down(&event.key.keysym)) {
					should_quit = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (state->current_window == WINDOW_MAP && event.button.button == SDL_BUTTON_MIDDLE) {
					state->is_dragging = true;
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (state->current_window == WINDOW_MAP && event.button.button == SDL_BUTTON_MIDDLE) {
					state->is_dragging = false;
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
				should_quit = true;
				break;
		}

	return should_quit;
}

static void render(struct game_state const* state) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

/*	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (double) (800) / (double) (600), 0.001, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		0.0, 0.0, 1.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0
	);*/

	/* region draw world map */
	if (state->current_window == WINDOW_MAP)
	{
		float const width = 5616.0f / (float) state->window_width;
		float const height = 2160.0f / (float) state->window_height;
		glPushMatrix();
		glTranslatef(state->camera[0], state->camera[1], 0.0f);
		glScalef(state->camera[2], state->camera[2], 1.0f);

		glBindTexture(GL_TEXTURE_2D, state->provinces_texture);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-width, -height);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(width, -height);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(width, height);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-width, height);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		glPopMatrix();
	}
	/* endregion */

	/* region draw ui */
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, state->window_width, state->window_height);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, state->window_width, state->window_height, 0, 1, -1);

//	if (state->current_window != WINDOW_MAP) {
//		render_texture(state, state->background_map_texture, &(struct frect) {
//			.x = 0.0f, .y = 0.0f, .w = (float) state->window_width, .h =(float) state->window_height
//		});
//	}
//	{
//		struct gui_defs* gui = state->gui_defs;
//		for (; gui != NULL; gui = gui->next) {
//			ui_render(state, gui->name);
//		}
//	}
	{
		struct gui_defs* gui = state->gui_defs;
		for (; gui != NULL; gui = gui->next) {
			if (strcmp(gui->name, "menubar") == 0) {
				gui = gui->window.children;
				for (; gui != NULL; gui = gui->next) {
					if (strcmp(gui->name, "chat_window") == 0) {
						gui->window.dont_render = "true";
					}
				}
				break;
			}
		}
	}
	ui_render(state, "topbar");
	ui_render(state, "FPS_Counter");
	ui_render(state, "menubar");
	ui_render(state, "minimap_pic");
//	ui_render(state, "console_entry_wnd");


//	render_topbar(state);
	glPopMatrix();
//	glBindTexture(GL_TEXTURE_2D, state->terrain_texture);
//	glEnable(GL_TEXTURE_2D);
//	glBegin(GL_QUADS);
//	glTexCoord2f(0.0f, 1.0f);
//	glVertex2f(-0.1f, -0.1f);
//	glTexCoord2f(1.0f, 1.0f);
//	glVertex2f(0.1f, -0.1f);
//	glTexCoord2f(1.0f, 0.0f);
//	glVertex2f(0.1f, 0.1f);
//	glTexCoord2f(0.0f, 0.0f);
//	glVertex2f(-0.1f, 0.1f);
//	glEnd();
//	glDisable(GL_TEXTURE_2D);
	/* endregion */
}

static GLenum init_opengl(void) {
	GLenum error = GL_NO_ERROR;

	/* Initialize Projection Matrix */
	glMatrixMode(GL_PROJECTION);
	if ((error = glGetError()) != GL_NO_ERROR) return error;
	glLoadIdentity();
	if ((error = glGetError()) != GL_NO_ERROR) return error;

	/* Initialize Modelview Matrix */
	glMatrixMode(GL_MODELVIEW);
	if ((error = glGetError()) != GL_NO_ERROR) return error;
	glLoadIdentity();
	if ((error = glGetError()) != GL_NO_ERROR) return error;

	/* Initialize clear color */
	glClearColor(0.f, 0.f, 0.f, 1.f);
	if ((error = glGetError()) != GL_NO_ERROR) return error;

	return error;
}

int main(int argc, char** argv) {
	int exit_code = EXIT_SUCCESS;
	SDL_Window* window = NULL;
	SDL_GLContext context = NULL;
	static const int window_width = 1280;
	static const int window_height = 1024;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
		exit_code = EXIT_FAILURE;
	}
	if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0) {
		fprintf(stderr, "Failed to set MAJOR_VERSION: %s\n", SDL_GetError());
		exit_code = EXIT_FAILURE;
	} else if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) != 0) {
		fprintf(stderr, "Failed to set MINOR_VERSION: %s\n", SDL_GetError());
		exit_code = EXIT_FAILURE;
	} else if ((window = SDL_CreateWindow(
		"ov2",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		window_width,
		window_height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /*| SDL_WINDOW_RESIZABLE*/
	)) == NULL) {
		fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
		exit_code = EXIT_FAILURE;
	} else if ((context = SDL_GL_CreateContext(window)) == NULL) {
		fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
		exit_code = EXIT_FAILURE;
	} else {
		SDL_SetWindowResizable(window, SDL_TRUE); /* This is done here instead of on creation to make it easier for me to debug with my wm */
		GLenum error = GL_NO_ERROR;
		struct game_state* game_state;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			fprintf(stderr, "WARNING: Failed to set vsync: %s\n", SDL_GetError());
		}
		if ((error = init_opengl()) != GL_NO_ERROR) {
			fprintf(stderr, "Failed to initialize OpenGL: %s\n", gluErrorString(error));
			exit_code = EXIT_FAILURE;
		} else if ((game_state = init_game_state(window_width, window_height)) == NULL) {
			fprintf(stderr, "Failed to initialize game state\n");
			exit_code = EXIT_FAILURE;
		} else {
			bool should_quit = false;
			while (!should_quit) {
				should_quit = handle_events(game_state);
				render(game_state);
				SDL_GL_SwapWindow(window);
			}
		}
		if (game_state != NULL) free_game_state(game_state);
	}

	if (context != NULL) SDL_GL_DeleteContext(context);
	if (window != NULL) SDL_DestroyWindow(window);
	SDL_Quit();
	return exit_code;
}