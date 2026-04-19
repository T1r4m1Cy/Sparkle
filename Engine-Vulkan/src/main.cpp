/*
 * Vulkan Windowed Program
 *
 * Copyright (C) 2016, 2018 Valve Corporation
 * Copyright (C) 2016, 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
Vulkan C++ Windowed Project Template
Create and destroy a Vulkan surface on an SDL window.
*/

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <filesystem>

#include "engine/Engine.h"
#include "engine/ECS.h"
#include "engine/IWindowProvider.h"

struct SDLWindowProvider : IWindowProvider {
    SDL_Window* window;
    VulkanContext* vulkan = nullptr;

    explicit SDLWindowProvider(SDL_Window* win) : window(win) {}

    std::vector<const char*> get_required_extensions() override {
        unsigned count;
        SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);
        std::vector<const char*> extensions(count);
        SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data());
        return extensions;
    }

    vk::SurfaceKHR create_surface(vk::Instance instance) override {
        VkSurfaceKHR surface;
        SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(instance), &surface);
        return vk::SurfaceKHR(surface);
    }

    vk::Extent2D get_window_size() override {
        int w, h;
        SDL_Vulkan_GetDrawableSize(window, &w, &h);
        return { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
    }

    vk::Framebuffer get_framebuffer(uint32_t imageIndex, uint32_t /*frame*/) override {
        return vulkan->swapchain.swapchainFramebuffers[imageIndex];
    }
};

int main()
{
    std::cout << "START" << std::endl;
    std::cout.flush();

    std::cout << "Working Directory: "
        << std::filesystem::current_path() << std::endl;
    std::cout.flush();

    Engine engine;

    window_init(engine.window, 1280, 720);

    SDLWindowProvider provider(engine.window.handle);
    provider.vulkan = &engine.vulkan;
    engine.windowProvider = &provider;

    if (engine_init(engine, 1280, 720) != 0) return 1;

    bool stillRunning = true;

    while (stillRunning) 
    {
        engine_run(engine, stillRunning);
    }

    engine_shutdown(engine);
}