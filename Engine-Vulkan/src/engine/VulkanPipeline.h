#pragma once

#include <vulkan/vulkan.hpp>

struct VulkanContext;

struct VulkanPipeline {
	vk::RenderPass renderPass;
	vk::RenderPass imguiRenderPass;

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline pipeline;

	vk::DescriptorSetLayout lightingDescriptorSetLayout;
	vk::PipelineLayout lightingPipelineLayout;
	vk::Pipeline lightingPipeline;
};

int vulkan_init_graphics_pipeline(VulkanContext& ctx);
int vulkan_init_lighting_pipeline(VulkanContext& ctx);

int vulkan_init_render_pass(VulkanContext& ctx);
int vulkan_init_imgui_render_pass(VulkanContext& ctx);

int vulkan_init_descriptor_set_layout(VulkanContext& ctx);
int vulkan_init_lighting_descriptor_set_layout(VulkanContext& ctx);

vk::ShaderModule create_shader_module(VulkanContext& ctx, const std::vector<char>& code);