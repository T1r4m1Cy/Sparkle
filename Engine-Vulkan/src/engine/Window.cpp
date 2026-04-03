#include <iostream>
#include <vector>

#include "Window.h"

int window_init(Window& w, int width, int height)
{
    // Create an SDL window that supports Vulkan rendering.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Could not initialize SDL." << std::endl;
        return 1;
    }
    w.handle = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (w.handle == NULL) {
        std::cout << "Could not create SDL window." << std::endl;
        return 1;
    }

    w.width = width;
    w.height = height;

    return 0;
}

static void framebuffer_resize_callback(SDL_Window* window, int width, int height)
{

}

void window_shutdown(Window& w)
{
    SDL_DestroyWindow(w.handle);
    SDL_Quit();
}