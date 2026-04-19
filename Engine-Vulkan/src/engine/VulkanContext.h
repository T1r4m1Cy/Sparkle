#pragma once

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <unordered_map>

#include "RendererData.h"

#include "VulkanUtils.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanResources.h"
#include "IWindowProvider.h"

struct SDL_Window;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
};

struct VulkanContext {
	vk::Instance instance;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	QueueFamilyIndices queueFamilies;
	VulkanSwapchain swapchain;
	VulkanPipeline pipeline;

	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	uint32_t currentFrame = 0;
	uint32_t acquireSemaphoreIndex = 0;

	std::vector<vk::Buffer> uniformBuffers;
	std::vector<vk::DeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	std::vector<vk::Buffer> lightBuffers;
	std::vector<vk::DeviceMemory> lightBuffersMemory;
	std::vector<void*> lightBuffersMapped;

	vk::DescriptorPool descriptorPool;
	int descriptorPoolCapacity = 100;

	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;

	GBuffer gBuffer;

	std::array<OffscreenBuffer, MAX_FRAMES_IN_FLIGHT> offscreenBuffers;
	vk::Sampler offscreenSampler;
};

int vulkan_init(VulkanContext& ctx, IWindowProvider& provider);
void vulkan_shutdown(VulkanContext& ctx);

int vulkan_recreate_g_buffer(VulkanContext& ctx);
int vulkan_init_offscreen_buffers(VulkanContext& ctx);

int vulkan_update_lighting_descriptor_sets(VulkanContext& ctx);

struct AssetManager;

void draw_frame(VulkanContext& ctx, AssetManager& assets,
    IWindowProvider& provider, FramePacket framePacket);