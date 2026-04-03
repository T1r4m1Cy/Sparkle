#pragma once

#include <vulkan/vulkan.hpp>

#include <iostream>
#include <optional>

#include "VulkanUtils.h"

struct SDL_Window;
struct VulkanContext;

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct VulkanSwapchain {
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> swapchainImages;
	std::vector<vk::ImageView> imageViews;
	vk::Format swapchainFormat;
	vk::Extent2D swapchainExtent;
	std::vector<vk::Framebuffer> swapchainFramebuffers;
	std::vector<vk::Framebuffer> imguiFramebuffers;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
};

int vulkan_init_swapchain(VulkanContext& ctx, SDL_Window* window);
int vulkan_init_image_views(VulkanContext& ctx);
int vulkan_init_depth_resources(VulkanContext& ctx);

int vulkan_init_framebuffers(VulkanContext& ctx);
int vulkan_init_imgui_framebuffers(VulkanContext& ctx);
int vulkan_init_offscreen_framebuffers(VulkanContext& ctx);

int vulkan_cleanup_swapchain(VulkanContext& ctx);
int vulkan_recreate_swapchain(VulkanContext& ctx, SDL_Window* window);

vk::Format find_depth_format(VulkanContext& ctx);
bool has_stencil_component(vk::Format format);
SwapChainSupportDetails query_swap_chain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface);