#pragma once

#include "libraries/imgui/imgui_internal.h"
#include "libraries/imgui/imgui_impl_sdl2.h"

#include <iostream>

struct VulkanContext;
struct SDL_Window;

void imgui_init(VulkanContext& ctx, SDL_Window* window);
void vulkan_init_offscreen_imgui_descriptors(VulkanContext& ctx);
void imgui_shutdown(VulkanContext& ctx);
void imgui_new_frame();
void imgui_render(VulkanContext& ctx, uint32_t imageIndex, uint32_t frame);