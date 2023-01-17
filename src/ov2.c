#include "province_definitions.h"
#include "game_state.h"
#include "legacy_ui.h"
#include "parse.h"
#include "fs.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SOIL/SOIL.h> /* TODO: Replace SOIL with SDL_image */
#include <stdio.h>
#include <stdlib.h>
#include "ui_event.h"
#include "game_tick.h"

static void render(struct game_state const* state) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	render_ui(state);
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
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
	} if (TTF_Init() != 0) {
		fprintf(stderr, "TTF initialization failed: %s\n", TTF_GetError());
		exit_code = EXIT_FAILURE;
	} else if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0) {
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
		GLenum error = GL_NO_ERROR;
		struct game_state* game_state;
		SDL_SetWindowResizable(window, SDL_TRUE); /* This is done here instead of on creation to make it easier for me to debug with my wm */
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
			while (!game_state->should_quit) {
				handle_events(game_state);
				game_tick(game_state);
				render(game_state);
				SDL_GL_SwapWindow(window);
			}
		}
		if (game_state != NULL) free_game_state(game_state);
	}

	if (context != NULL) SDL_GL_DeleteContext(context);
	if (window != NULL) SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
	return exit_code;
}