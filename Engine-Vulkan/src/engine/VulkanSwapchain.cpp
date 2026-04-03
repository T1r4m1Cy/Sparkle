//#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include "VulkanContext.h"

#include "VulkanSwapchain.h"

int vulkan_init_swapchain(VulkanContext& ctx, 
    const std::function<vk::Extent2D()>& getWindowSize)
{
    SwapChainSupportDetails support = query_swap_chain_support(ctx.physicalDevice, ctx.surface);

    //Format
    vk::SurfaceFormatKHR chosenFormat = support.formats[0];
    for (const auto& f : support.formats) {
        if (f.format == vk::Format::eB8G8R8A8Srgb &&
            f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            chosenFormat = f;
            break;
        }
    }

    //Present mode
    vk::PresentModeKHR chosenPresentMode = vk::PresentModeKHR::eFifo;
    for (const auto& pm : support.presentModes) {
        if (pm == vk::PresentModeKHR::eMailbox) {
            chosenPresentMode = pm;
            break;
        }
    }

    //Window size
    vk::Extent2D extent = getWindowSize();
    if (support.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = support.capabilities.currentExtent;
    }

    //Number of buffers
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 &&
        imageCount > support.capabilities.maxImageCount)
        imageCount = support.capabilities.maxImageCount;

    //Sharing mode
    vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
    std::vector<uint32_t> queueFamilyIndices;

    if (ctx.queueFamilies.graphicsFamily != ctx.queueFamilies.presentFamily) {
        sharingMode = vk::SharingMode::eConcurrent;
        queueFamilyIndices = {
            ctx.queueFamilies.graphicsFamily.value(),
            ctx.queueFamilies.presentFamily.value()
        };
    }

    vk::SwapchainCreateInfoKHR swapchainInfo = vk::SwapchainCreateInfoKHR()
        .setSurface(ctx.surface)
        .setMinImageCount(imageCount)
        .setImageFormat(chosenFormat.format)
        .setImageColorSpace(chosenFormat.colorSpace)
        .setImageExtent(extent)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setImageSharingMode(sharingMode)
        .setQueueFamilyIndexCount(static_cast<uint32_t>(queueFamilyIndices.size()))
        .setPQueueFamilyIndices(queueFamilyIndices.data())
        .setPreTransform(support.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(chosenPresentMode)
        .setClipped(true);

    try {
        ctx.swapchain.swapchain = ctx.device.createSwapchainKHR(swapchainInfo);
    }
    catch (const std::exception& e) {
        std::cout << "Could not create swapchain: " << e.what() << std::endl;
        return 1;
    }

    ctx.swapchain.swapchainImages = ctx.device.getSwapchainImagesKHR(ctx.swapchain.swapchain);
    ctx.swapchain.swapchainFormat = chosenFormat.format;
    ctx.swapchain.swapchainExtent = extent;

    std::cout << "Swapchain created: " << extent.width << "x" << extent.height << std::endl;

    return 0;
}

int vulkan_init_image_views(VulkanContext& ctx)
{
    ctx.swapchain.imageViews.resize(ctx.swapchain.swapchainImages.size());

    for (size_t i = 0; i < ctx.swapchain.swapchainImages.size(); i++) {
        try {
            ctx.swapchain.imageViews[i] = create_image_view(ctx,
                ctx.swapchain.swapchainImages[i], ctx.swapchain.swapchainFormat,
                vk::ImageAspectFlagBits::eColor);
        }
        catch (const std::exception& e) {
            std::cout << "Could not create image view: " << e.what() << std::endl;
            return 1;
        }
    }

    std::cout << "Image views created: " << ctx.swapchain.imageViews.size() << std::endl;

    return 0;
}

int vulkan_init_depth_resources(VulkanContext& ctx)
{
    vk::Format depthFormat = find_depth_format(ctx);

    ImageInfo depthImageInfo = create_image(ctx, ctx.swapchain.swapchainExtent.width, ctx.swapchain.swapchainExtent.height,
        depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    ctx.depthImage = depthImageInfo.image;
    ctx.depthImageMemory = depthImageInfo.imageMemory;
    ctx.depthImageView = create_image_view(ctx, ctx.depthImage, depthFormat,
        vk::ImageAspectFlagBits::eDepth);

    transition_image_layout(ctx, ctx.depthImage, depthFormat,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    return 0;
}

int vulkan_init_framebuffers(VulkanContext& ctx)
{
    ctx.swapchain.swapchainFramebuffers.resize(ctx.swapchain.imageViews.size());

    for (size_t i = 0; i < ctx.swapchain.imageViews.size(); i++) {
        std::array<vk::ImageView, 5> attachments = {
            ctx.gBuffer.position.textureView,
            ctx.gBuffer.normal.textureView,
            ctx.gBuffer.albedo.textureView,
            ctx.swapchain.imageViews[i],
            ctx.depthImageView
        };

        vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
            .setRenderPass(ctx.pipeline.renderPass)
            .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
            .setPAttachments(attachments.data())
            .setWidth(ctx.swapchain.swapchainExtent.width)
            .setHeight(ctx.swapchain.swapchainExtent.height)
            .setLayers(1);

        ctx.swapchain.swapchainFramebuffers[i] = ctx.device.createFramebuffer(framebufferInfo);
    }

    return 0;
}

int vulkan_init_offscreen_framebuffers(VulkanContext& ctx)
{
    for (size_t i = 0; i < ctx.offscreenBuffers.size(); i++) {
        std::array<vk::ImageView, 5> attachments = {
            ctx.gBuffer.position.textureView,
            ctx.gBuffer.normal.textureView,
            ctx.gBuffer.albedo.textureView,
            ctx.offscreenBuffers[i].imageView,
            ctx.depthImageView
        };

        vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
            .setRenderPass(ctx.pipeline.renderPass)
            .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
            .setPAttachments(attachments.data())
            .setWidth(ctx.swapchain.swapchainExtent.width)
            .setHeight(ctx.swapchain.swapchainExtent.height)
            .setLayers(1);

        ctx.offscreenBuffers[i].framebuffer = ctx.device.createFramebuffer(framebufferInfo);
    }

    return 0;
}


int vulkan_cleanup_swapchain(VulkanContext& ctx)
{
    for (auto& buffer : ctx.offscreenBuffers) {
        ctx.device.destroyFramebuffer(buffer.framebuffer);
		ctx.device.destroyImageView(buffer.imageView);
        ctx.device.destroyImage(buffer.image);
		ctx.device.freeMemory(buffer.imageMemory);
	}

    for (auto& frameBuffer : ctx.swapchain.swapchainFramebuffers) {
        ctx.device.destroyFramebuffer(frameBuffer);
    }
    ctx.swapchain.swapchainFramebuffers.clear();

    for (auto& imageView : ctx.swapchain.imageViews) {
        ctx.device.destroyImageView(imageView);
    }

    ctx.device.destroySwapchainKHR(ctx.swapchain.swapchain);

	ctx.device.destroyImageView(ctx.depthImageView);
	ctx.device.destroyImage(ctx.depthImage);
	ctx.device.freeMemory(ctx.depthImageMemory);

    return 0;
}

int vulkan_recreate_swapchain(VulkanContext& ctx, 
    const std::function<vk::Extent2D()>& getWindowSize) 
{
    ctx.device.waitIdle();

    vulkan_cleanup_swapchain(ctx);

    if (vulkan_init_swapchain(ctx, getWindowSize) != 0) return 1;
    if (vulkan_init_image_views(ctx) != 0) return 1;
	if (vulkan_init_depth_resources(ctx) != 0) return 1;
    if (vulkan_recreate_g_buffer(ctx) != 0) return 1;
	if (vulkan_init_offscreen_buffers(ctx) != 0) return 1;
	if (vulkan_init_offscreen_framebuffers(ctx) != 0) return 1;
	if (vulkan_update_lighting_descriptor_sets(ctx) != 0) return 1;
    if (vulkan_init_framebuffers(ctx) != 0) return 1;

    std::cout << "Swapchain recreated!" << std::endl;

    return 0;
}

vk::Format find_depth_format(VulkanContext& ctx)
{
    return find_supported_format(ctx, {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint
        }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

SwapChainSupportDetails query_swap_chain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}