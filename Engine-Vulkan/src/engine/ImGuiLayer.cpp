#include "libraries/imgui/imgui.h"
#include "libraries/imgui/imgui_impl_vulkan.h"
#include "VulkanContext.h"

#include <iostream>

#include "ImGuiLayer.h"

static vk::DescriptorPool imguiDescriptorPool;

void imgui_init(VulkanContext& ctx, SDL_Window* window)
{
	std::array<vk::DescriptorPoolSize, 1> poolSizes = {};
	poolSizes[0].type = vk::DescriptorType::eCombinedImageSampler;
	poolSizes[0].descriptorCount = 10;

	vk::DescriptorPoolCreateInfo poolInfo = vk::DescriptorPoolCreateInfo()
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(10)
		.setPoolSizeCount(1)
		.setPPoolSizes(poolSizes.data());

	imguiDescriptorPool = ctx.device.createDescriptorPool(poolInfo);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	ImFontConfig config;
	config.SizePixels = 16.0f;
	io.Fonts->AddFontDefault(&config);
	io.FontDefault = io.Fonts->Fonts.back();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForVulkan(window);
	
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = ctx.instance;
	initInfo.PhysicalDevice = ctx.physicalDevice;
	initInfo.Device = ctx.device;
	initInfo.QueueFamily = ctx.queueFamilies.graphicsFamily.value();
	initInfo.Queue = ctx.graphicsQueue;
	initInfo.DescriptorPool = imguiDescriptorPool;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = ctx.swapchain.swapchainImages.size();

	initInfo.PipelineInfoMain.RenderPass = ctx.pipeline.imguiRenderPass;
	initInfo.PipelineInfoMain.Subpass = 0;
	initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&initInfo);
}

void vulkan_init_offscreen_imgui_descriptors(VulkanContext& ctx)
{
	for (auto& offscreen : ctx.offscreenBuffers) {
		if (offscreen.imguiDescriptorSet != VK_NULL_HANDLE) {
			ImGui_ImplVulkan_RemoveTexture(offscreen.imguiDescriptorSet);
		}
		offscreen.imguiDescriptorSet = ImGui_ImplVulkan_AddTexture(
			ctx.offscreenSampler, 
			offscreen.imageView, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}
}

void imgui_shutdown(VulkanContext& ctx)
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	ctx.device.destroyDescriptorPool(imguiDescriptorPool);
	ctx.device.destroyRenderPass(ctx.pipeline.imguiRenderPass);
}

void imgui_new_frame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void imgui_render(VulkanContext& ctx, uint32_t imageIndex, uint32_t frame)
{
	ImGui::Render();

	vk::ClearValue clearValue = vk::ClearValue()
		.setColor({ 0.0f, 0.0f, 0.0f, 1.0f });

	vk::RenderPassBeginInfo renderPassInfo = vk::RenderPassBeginInfo()
		.setRenderPass(ctx.pipeline.imguiRenderPass)
		.setFramebuffer(ctx.swapchain.imguiFramebuffers[imageIndex])
		.setRenderArea({ {0,0}, ctx.swapchain.swapchainExtent })
		.setClearValueCount(1)
		.setPClearValues(&clearValue);

	auto& cmd = ctx.swapchain.commandBuffers[frame];
	cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
	cmd.endRenderPass();
}