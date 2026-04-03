#pragma once

#include <SDL2/SDL.h>

struct Window {
	SDL_Window* handle;
	int width, height;
};

int window_init(Window& w, int width, int height);
void window_shutdown(Window& w);