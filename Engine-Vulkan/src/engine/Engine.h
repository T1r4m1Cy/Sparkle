#pragma once

#include <chrono>

#include "Window.h"
#include "VulkanContext.h"
#include "ECS.h"
#include "AssetManager.h"

struct Engine {
	Window window;
	VulkanContext vulkan;
	World world;
	AssetManager assets;

	float deltaTime = 0.0f;
	std::chrono::steady_clock::time_point lastFrameTime;

	std::vector<const char*> requiredExtensions;
	std::function<vk::SurfaceKHR(vk::Instance)> createSurface;
	std::function<vk::Extent2D()> getWindowSize;

	std::function<vk::Framebuffer(uint32_t imageIndex, uint32_t frame)> 
		getFramebuffer;
};

int engine_init(Engine& e, int width, int height);
void engine_tick(Engine& e);
void engine_render(Engine& e);
void engine_run(Engine& e, bool& stillRunning);
void engine_shutdown(Engine& e);