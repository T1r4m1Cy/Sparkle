#pragma once

#include <vulkan/vulkan.hpp>

struct VulkanContext;

struct ImageInfo {
	vk::Image image;
	vk::DeviceMemory imageMemory;
};

struct BufferInfo {
	vk::Buffer buffer;
	vk::DeviceMemory bufferMemory;
};

ImageInfo create_image(VulkanContext& ctx, uint32_t width, uint32_t height, vk::Format format,
	vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
vk::ImageView create_image_view(VulkanContext& ctx, vk::Image image, vk::Format format,
	vk::ImageAspectFlagBits aspectFlags);
uint32_t find_memory_type(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, 
	vk::MemoryPropertyFlags properties);
vk::CommandBuffer begin_single_time_commands(VulkanContext& ctx);
void end_single_time_commands(VulkanContext& ctx, vk::CommandBuffer commandBuffer);
vk::Format find_supported_format(VulkanContext& ctx, std::vector<vk::Format> candidates,
	vk::ImageTiling tiling, vk::FormatFeatureFlags features);
BufferInfo create_buffer(VulkanContext& ctx,
	vk::DeviceSize size, vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags properties);
int copy_buffer(VulkanContext& ctx, vk::Buffer srcBuffer, vk::Buffer dstBuffer,
	vk::DeviceSize size);
int copy_buffer_to_image(VulkanContext& ctx, vk::Buffer buffer, vk::Image image,
	uint32_t width, uint32_t height);
std::vector<char> read_file(const std::string& filename);
void transition_image_layout(VulkanContext& ctx, vk::Image image, vk::Format format,
	vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
bool has_stencil_component(vk::Format format);