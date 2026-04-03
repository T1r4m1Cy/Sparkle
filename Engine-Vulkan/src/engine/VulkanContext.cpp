// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <algorithm>

#include "AssetManager.h"

#include "VulkanContext.h"

static int vulkan_init_instance(VulkanContext& ctx, 
    const std::function<vk::Extent2D()>& getWindowSize,
    std::vector<const char*> requiredExtensions);

static int vulkan_init_surface(VulkanContext& ctx, 
    const std::function<vk::SurfaceKHR(vk::Instance)>& createSurface);

static int vulkan_init_physical_device(VulkanContext& ctx);
static int rate_physical_device(vk::PhysicalDevice device, vk::SurfaceKHR surface);
static bool is_device_suitable(vk::PhysicalDevice device, vk::SurfaceKHR surface);
static bool check_device_extension_support(vk::PhysicalDevice device);
static bool queue_family_indices_complete(const QueueFamilyIndices& indices);
static QueueFamilyIndices find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface);
static SwapChainSupportDetails query_swap_chain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface);

static int vulkan_init_device(VulkanContext& ctx);

static int vulkan_init_image_views(VulkanContext& ctx);

static int vulkan_init_command_pool(VulkanContext& ctx);

static int vulkan_init_command_buffer(VulkanContext& ctx);

static ImageInfo vulkan_init_texture_image(VulkanContext& ctx, const std::string& texPath);

//static int vulkan_init_texture_image_view(VulkanContext& ctx);



static Mesh load_model(const std::string& path);



static int vulkan_init_offscreen_sampler(VulkanContext& ctx);


static int vulkan_init_uniform_buffer(VulkanContext& ctx);
static int vulkan_init_light_buffers(VulkanContext& ctx);
static int vulkan_init_g_buffer(VulkanContext& ctx);

static int vulkan_init_sync_objects(VulkanContext& ctx);

vk::ImageView create_image_view(VulkanContext& ctx, vk::Image image, vk::Format format,
    vk::ImageAspectFlagBits aspectFlags);

static int vulkan_alloc_lighting_descriptor_sets(VulkanContext& ctx);


void update_uniform_buffer(VulkanContext& ctx, uint32_t currentImage, UniformBufferObject ubo);
void update_light_buffer(VulkanContext& ctx, uint32_t currentImage, LightUBO ubo);
void record_command_buffer(VulkanContext& ctx, AssetManager& assets,
    uint32_t imageIndex, uint32_t frame,
    const std::vector<DrawCall>& drawCalls,
    vk::Framebuffer framebuffer);



int vulkan_init(VulkanContext& ctx, 
    std::vector<const char*> requiredExtensions,
    const std::function<vk::SurfaceKHR(vk::Instance)>& createSurface,
    const std::function<vk::Extent2D()>& getWindowSize)
{
    if (vulkan_init_instance(ctx, getWindowSize, requiredExtensions) != 0) return 1;
    if (vulkan_init_surface(ctx, createSurface) != 0) return 1;
    if (vulkan_init_physical_device(ctx) != 0) return 1;
    if (vulkan_init_device(ctx) != 0) return 1;
	if (vulkan_init_offscreen_sampler(ctx) != 0) return 1;
    if (vulkan_init_swapchain(ctx, getWindowSize) != 0) return 1;
    if (vulkan_init_image_views(ctx) != 0) return 1;
    if (vulkan_init_render_pass(ctx) != 0) return 1;
    if (vulkan_init_descriptor_set_layout(ctx) != 0) return 1;
    if (vulkan_init_lighting_descriptor_set_layout(ctx) != 0) return 1;
    if (vulkan_init_graphics_pipeline(ctx) != 0) return 1;
    if (vulkan_init_lighting_pipeline(ctx) != 0) return 1;
    if (vulkan_init_command_pool(ctx) != 0) return 1;
    if (vulkan_init_command_buffer(ctx) != 0) return 1;
    if (vulkan_init_g_buffer(ctx) != 0) return 1;
    if (vulkan_init_depth_resources(ctx) != 0) return 1;
	if (vulkan_init_offscreen_buffers(ctx) != 0) return 1;
    if (vulkan_init_offscreen_framebuffers(ctx) != 0) return 1;
    if (vulkan_init_framebuffers(ctx) != 0) return 1;
    //if (vulkan_init_texture_image(ctx) != 0) return 1;
    //if (vulkan_init_texture_image_view(ctx) != 0) return 1;
    //if (vulkan_init_texture_sampler(ctx) != 0) return 1;
    //if (vulkan_init_vertex_buffer(ctx) != 0) return 1;
    //if (vulkan_init_index_buffer(ctx) != 0) return 1;
    if (vulkan_init_uniform_buffer(ctx) != 0) return 1;
    if (vulkan_init_light_buffers(ctx) != 0) return 1;
    if (create_descriptor_pool(ctx) != 0) return 1;
    if (vulkan_alloc_lighting_descriptor_sets(ctx) != 0) return 1;
	if (vulkan_update_lighting_descriptor_sets(ctx) != 0) return 1;
    //if (vulkan_init_descriptor_sets(ctx) != 0) return 1;
    if (vulkan_init_sync_objects(ctx) != 0) return 1;

    return 0;
}

static int vulkan_init_instance(VulkanContext& ctx, 
    const std::function<vk::Extent2D()>& getWindowSize,
    std::vector<const char*> requiredExtensions)
{
    unsigned extension_count = requiredExtensions.size();
    std::vector<const char*> extensions = requiredExtensions;

    // Use validation layers if this is a debug build
    std::vector<const char*> layers;
#if defined(_DEBUG)
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
    // program, which can be useful for layers and tools to provide more debug information.
    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
        .setPApplicationName("Vulkan C++ Windowed Program Template")
        .setApplicationVersion(1)
        .setPEngineName("LunarG SDK")
        .setEngineVersion(1)
        .setApiVersion(VK_API_VERSION_1_0);

    // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
    // are needed.
    vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
        .setFlags(vk::InstanceCreateFlags())
        .setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
        .setPpEnabledExtensionNames(extensions.data())
        .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
        .setPpEnabledLayerNames(layers.data());

    // Create the Vulkan instance.
    vk::Instance instance;
    try {
        instance = vk::createInstance(instInfo);
    }
    catch (const std::exception& e) {
        std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
        return 1;
    }
    ctx.instance = instance;

    return 0;
}

static int vulkan_init_surface(VulkanContext& ctx, 
    const std::function<vk::SurfaceKHR(vk::Instance)>& createSurface)
{
    ctx.surface = createSurface(ctx.instance);

    return 0;
}

static int vulkan_init_physical_device(VulkanContext& ctx)
{
    std::vector<vk::PhysicalDevice> devices = ctx.instance.enumeratePhysicalDevices();

    if (devices.empty()) {
        std::cout << "No Vulkan-capable GPU found." << std::endl;
        return 1;
    }

    std::multimap<int, vk::PhysicalDevice> candidates;
    for (const auto& device : devices) {
        candidates.insert({ rate_physical_device(device, ctx.surface), device });
    }

    if (candidates.rbegin()->first == 0)
    {
        std::cout << "No suitable GPU was found" << std::endl;
        return 1;
    }

    ctx.physicalDevice = candidates.rbegin()->second;

    std::cout << "GPU: " << ctx.physicalDevice.getProperties().deviceName << std::endl;
    ctx.queueFamilies = find_queue_families(ctx.physicalDevice, ctx.surface);

    return 0;
}

static int rate_physical_device(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    if (!is_device_suitable(device, surface)) return 0;

    vk::PhysicalDeviceProperties props = device.getProperties();
    //vk::PhysicalDeviceFeatures features = device.getFeatures();

    int score = 0;

    if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;

    score += props.limits.maxImageDimension2D;

    return score;
}

static bool is_device_suitable(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices = find_queue_families(device, surface);

    bool extensionsSupported = check_device_extension_support(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

    return queue_family_indices_complete(indices) && extensionsSupported && swapChainAdequate &&
        supportedFeatures.samplerAnisotropy;
}

static bool queue_family_indices_complete(const QueueFamilyIndices& indices)
{
    return indices.graphicsFamily.has_value() && indices.presentFamily.has_value();
}

static bool check_device_extension_support(vk::PhysicalDevice device)
{
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static QueueFamilyIndices find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> families = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < families.size(); i++) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphicsFamily = i;

        if (device.getSurfaceSupportKHR(i, surface))
            indices.presentFamily = i;

        if (queue_family_indices_complete(indices))
            break;
    }

    return indices;
}

static SwapChainSupportDetails query_swap_chain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}

static int vulkan_init_device(VulkanContext& ctx)
{
    float queuePriority = 1.0f;

    std::set<uint32_t> uniqueQueueFamilies = {
        ctx.queueFamilies.graphicsFamily.value(),
        ctx.queueFamilies.presentFamily.value()
    };

    std::vector<vk::DeviceQueueCreateInfo> queueInfos;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueInfo = vk::DeviceQueueCreateInfo()
            .setQueueFamilyIndex(queueFamily)
            .setQueueCount(1)
            .setPQueuePriorities(&queuePriority);
        queueInfos.push_back(queueInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures()
        .setSamplerAnisotropy(true);

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


    vk::DeviceCreateInfo deviceInfo = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(static_cast<uint32_t>(queueInfos.size()))
        .setPQueueCreateInfos(queueInfos.data())
        .setPEnabledFeatures(&deviceFeatures)
        .setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()))
        .setPpEnabledExtensionNames(deviceExtensions.data());

    try {
        ctx.device = ctx.physicalDevice.createDevice(deviceInfo);
    }
    catch (const std::exception& e) {
        std::cout << "Could not create logical device: " << e.what() << std::endl;
        return 1;
    }

    ctx.graphicsQueue = ctx.device.getQueue(ctx.queueFamilies.graphicsFamily.value(), 0);
    ctx.presentQueue = ctx.device.getQueue(ctx.queueFamilies.presentFamily.value(), 0);
    std::cout << "Logical device created" << std::endl;

    return 0;
}

static int vulkan_init_image_views(VulkanContext& ctx)
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

static int vulkan_init_command_pool(VulkanContext& ctx)
{
    vk::CommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(ctx.queueFamilies.graphicsFamily.value());

    ctx.swapchain.commandPool = ctx.device.createCommandPool(poolInfo);

    return 0;
}

static int vulkan_init_command_buffer(VulkanContext& ctx)
{
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(ctx.swapchain.commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(VulkanContext::MAX_FRAMES_IN_FLIGHT);

    ctx.swapchain.commandBuffers = ctx.device.allocateCommandBuffers(allocInfo);

    return 0;
}

/*static int vulkan_init_texture_image_view(VulkanContext& ctx)
{
    ctx.textureImageView = create_image_view(ctx, ctx.textureImage, vk::Format::eR8G8B8A8Srgb, 
        vk::ImageAspectFlagBits::eColor);

    return 0;
}*/

void record_command_buffer(VulkanContext& ctx, AssetManager& assets,
    uint32_t imageIndex, uint32_t frame,
    const std::vector<DrawCall>& drawCalls,
    vk::Framebuffer framebuffer)
{
    vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo();

    ctx.swapchain.commandBuffers[frame].begin(beginInfo);

    std::array<vk::ClearValue, 5> clearValues{};
    clearValues[0].setColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    clearValues[1].setColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    clearValues[2].setColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    clearValues[3].setColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    clearValues[4].setDepthStencil({ 1.0f, 0 });

    vk::RenderPassBeginInfo renderPassInfo = vk::RenderPassBeginInfo()
        .setRenderPass(ctx.pipeline.renderPass)
        .setFramebuffer(framebuffer)
        .setRenderArea({ {0,0}, ctx.swapchain.swapchainExtent })
        .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
        .setPClearValues(clearValues.data());

    ctx.swapchain.commandBuffers[frame].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    ctx.swapchain.commandBuffers[frame].bindPipeline(vk::PipelineBindPoint::eGraphics, ctx.pipeline.pipeline);

    vk::Viewport viewport = vk::Viewport()
        .setX(0.0f)
        .setY(0.0f)
        .setWidth(static_cast<float>(ctx.swapchain.swapchainExtent.width))
        .setHeight(static_cast<float>(ctx.swapchain.swapchainExtent.height))
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    ctx.swapchain.commandBuffers[frame].setViewport(0, viewport);

    vk::Rect2D scissor = vk::Rect2D()
        .setOffset({ 0,0 })
        .setExtent(ctx.swapchain.swapchainExtent);

    ctx.swapchain.commandBuffers[frame].setScissor(0, scissor);

    //update_uniform_buffer(ctx, frame);

    //update_light_buffer(ctx, frame);

    for (auto& drawCall : drawCalls) {
        GpuMesh* gpuMesh = assets.get_mesh({ drawCall.meshId, drawCall.meshVersion });
		GpuTexture* gpuTexture = assets.get_texture({ drawCall.textureId, drawCall.textureVersion });

        ctx.swapchain.commandBuffers[frame].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            ctx.pipeline.pipelineLayout,
            0,
            gpuTexture->descriptorSets[frame],
            nullptr
        );

        vk::Buffer vertexBuffers[] = { gpuMesh->vertexBuffer };
        vk::DeviceSize offsets[] = { 0 };
        ctx.swapchain.commandBuffers[frame].bindVertexBuffers(
            0, vertexBuffers, offsets
        );
        ctx.swapchain.commandBuffers[frame].bindIndexBuffer(
            gpuMesh->indexBuffer, 0, vk::IndexType::eUint32
        );
        ctx.swapchain.commandBuffers[frame].pushConstants(
            ctx.pipeline.pipelineLayout,
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(glm::mat4),
            &drawCall.model
        );
        ctx.swapchain.commandBuffers[frame].drawIndexed(
            static_cast<uint32_t>(gpuMesh->indexCount), 1, 0, 0, 0
        );
    }

    ctx.swapchain.commandBuffers[frame].nextSubpass(vk::SubpassContents::eInline);

    ctx.swapchain.commandBuffers[frame].bindPipeline(vk::PipelineBindPoint::eGraphics, ctx.pipeline.lightingPipeline);

    ctx.swapchain.commandBuffers[frame].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        ctx.pipeline.lightingPipelineLayout,
        0,
        ctx.gBuffer.descriptorSets[frame],
        nullptr
    );

    ctx.swapchain.commandBuffers[frame].draw(6, 1, 0, 0);

    ctx.swapchain.commandBuffers[frame].endRenderPass();

    ctx.swapchain.commandBuffers[frame].end();
}

static int vulkan_init_uniform_buffer(VulkanContext& ctx)
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    ctx.uniformBuffers.resize(ctx.MAX_FRAMES_IN_FLIGHT);
    ctx.uniformBuffersMemory.resize(ctx.MAX_FRAMES_IN_FLIGHT);
    ctx.uniformBuffersMapped.resize(ctx.MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        BufferInfo uniformBufferInfo = create_buffer(ctx, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        ctx.uniformBuffers[i] = uniformBufferInfo.buffer;
        ctx.uniformBuffersMemory[i] = uniformBufferInfo.bufferMemory;
        ctx.uniformBuffersMapped[i] = ctx.device.mapMemory(ctx.uniformBuffersMemory[i], 0, bufferSize);
    }

    return 0;
}

static int vulkan_init_light_buffers(VulkanContext& ctx)
{
    vk::DeviceSize bufferSize = sizeof(LightUBO);

    std::cout << "Allocating light buffer size: " << bufferSize << std::endl;

    ctx.lightBuffers.resize(ctx.MAX_FRAMES_IN_FLIGHT);
    ctx.lightBuffersMemory.resize(ctx.MAX_FRAMES_IN_FLIGHT);
    ctx.lightBuffersMapped.resize(ctx.MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        BufferInfo lightBufferInfo = create_buffer(ctx, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        ctx.lightBuffers[i] = lightBufferInfo.buffer;
        ctx.lightBuffersMemory[i] = lightBufferInfo.bufferMemory;
        ctx.lightBuffersMapped[i] = ctx.device.mapMemory(ctx.lightBuffersMemory[i], 0, vk::WholeSize);
    }

    return 0;
}

static int vulkan_init_g_buffer(VulkanContext& ctx)
{
    ImageInfo imageInfo{};
    vk::ImageView imageView{};

    imageInfo = create_image(ctx,
        ctx.swapchain.swapchainExtent.width, ctx.swapchain.swapchainExtent.height,
        vk::Format::eR16G16B16A16Sfloat, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eInputAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    imageView = create_image_view(ctx,
        imageInfo.image, vk::Format::eR16G16B16A16Sfloat,
        vk::ImageAspectFlagBits::eColor);

    ctx.gBuffer.position.texture = imageInfo.image;
    ctx.gBuffer.position.textureMemory = imageInfo.imageMemory;
    ctx.gBuffer.position.textureView = imageView;

    imageInfo = create_image(ctx,
        ctx.swapchain.swapchainExtent.width, ctx.swapchain.swapchainExtent.height,
        vk::Format::eR16G16B16A16Sfloat, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eInputAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    imageView = create_image_view(ctx,
        imageInfo.image, vk::Format::eR16G16B16A16Sfloat,
        vk::ImageAspectFlagBits::eColor);

    ctx.gBuffer.normal.texture = imageInfo.image;
    ctx.gBuffer.normal.textureMemory = imageInfo.imageMemory;
    ctx.gBuffer.normal.textureView = imageView;

    imageInfo = create_image(ctx,
        ctx.swapchain.swapchainExtent.width, ctx.swapchain.swapchainExtent.height,
        vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eInputAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    imageView = create_image_view(ctx,
        imageInfo.image, vk::Format::eR8G8B8A8Unorm,
        vk::ImageAspectFlagBits::eColor);

    ctx.gBuffer.albedo.texture = imageInfo.image;
    ctx.gBuffer.albedo.textureMemory = imageInfo.imageMemory;
    ctx.gBuffer.albedo.textureView = imageView;

    return 0;
}

int vulkan_init_offscreen_buffers(VulkanContext& ctx)
{
    for (size_t i = 0; i < ctx.offscreenBuffers.size(); i++) {
        ImageInfo imageInfo = create_image(ctx,
            ctx.swapchain.swapchainExtent.width, ctx.swapchain.swapchainExtent.height,
            ctx.swapchain.swapchainFormat, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        vk::ImageView imageView = create_image_view(ctx,
            imageInfo.image, ctx.swapchain.swapchainFormat,
            vk::ImageAspectFlagBits::eColor);

        ctx.offscreenBuffers[i].image = imageInfo.image;
        ctx.offscreenBuffers[i].imageMemory = imageInfo.imageMemory;
        ctx.offscreenBuffers[i].imageView = imageView;
	}

	return 0;
}

static int vulkan_init_offscreen_sampler(VulkanContext& ctx)
{
	ctx.offscreenSampler = vulkan_init_texture_sampler(ctx);

    return 0;
}

int vulkan_recreate_g_buffer(VulkanContext& ctx)
{
    ctx.device.destroyImageView(ctx.gBuffer.position.textureView);
    ctx.device.destroyImage(ctx.gBuffer.position.texture);
    ctx.device.freeMemory(ctx.gBuffer.position.textureMemory);
    ctx.device.destroyImageView(ctx.gBuffer.normal.textureView);
    ctx.device.destroyImage(ctx.gBuffer.normal.texture);
    ctx.device.freeMemory(ctx.gBuffer.normal.textureMemory);
    ctx.device.destroyImageView(ctx.gBuffer.albedo.textureView);
    ctx.device.destroyImage(ctx.gBuffer.albedo.texture);
    ctx.device.freeMemory(ctx.gBuffer.albedo.textureMemory);
    return vulkan_init_g_buffer(ctx);
}

static int vulkan_init_sync_objects(VulkanContext& ctx)
{
    for (int i = 0; i < ctx.swapchain.swapchainImages.size() + ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
        ctx.imageAvailableSemaphores.push_back(ctx.device.createSemaphore(semaphoreInfo));
    }

    for (int i = 0; i < ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
        vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo()
            .setFlags(vk::FenceCreateFlagBits::eSignaled);

        ctx.renderFinishedSemaphores.push_back(ctx.device.createSemaphore(semaphoreInfo));
        ctx.inFlightFences.push_back(ctx.device.createFence(fenceInfo));
    }

    return 0;
}

void vulkan_shutdown(VulkanContext& ctx)
{
    vulkan_cleanup_swapchain(ctx);
    ctx.device.destroyPipeline(ctx.pipeline.pipeline);
    ctx.device.destroyPipelineLayout(ctx.pipeline.pipelineLayout);
    ctx.device.destroyPipeline(ctx.pipeline.lightingPipeline);
    ctx.device.destroyPipelineLayout(ctx.pipeline.lightingPipelineLayout);
    ctx.device.destroyRenderPass(ctx.pipeline.renderPass);

    ctx.device.destroyImageView(ctx.gBuffer.position.textureView);
    ctx.device.destroyImage(ctx.gBuffer.position.texture);
    ctx.device.freeMemory(ctx.gBuffer.position.textureMemory);

    ctx.device.destroyImageView(ctx.gBuffer.normal.textureView);
    ctx.device.destroyImage(ctx.gBuffer.normal.texture);
    ctx.device.freeMemory(ctx.gBuffer.normal.textureMemory);

    ctx.device.destroyImageView(ctx.gBuffer.albedo.textureView);
    ctx.device.destroyImage(ctx.gBuffer.albedo.texture);
    ctx.device.freeMemory(ctx.gBuffer.albedo.textureMemory);

    for (size_t i = 0; i < ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        ctx.device.destroyBuffer(ctx.lightBuffers[i]);
        ctx.device.freeMemory(ctx.lightBuffersMemory[i]);
    }

    for (size_t i = 0; i < ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        ctx.device.destroyBuffer(ctx.uniformBuffers[i]);
        ctx.device.freeMemory(ctx.uniformBuffersMemory[i]);
    }

    ctx.device.destroyDescriptorPool(ctx.descriptorPool);

    ctx.device.destroyDescriptorSetLayout(ctx.pipeline.descriptorSetLayout);
    ctx.device.destroyDescriptorSetLayout(ctx.pipeline.lightingDescriptorSetLayout);


    for (auto& semaphore : ctx.imageAvailableSemaphores)
        ctx.device.destroySemaphore(semaphore);

    for (auto& semaphore : ctx.renderFinishedSemaphores)
        ctx.device.destroySemaphore(semaphore);

    for (auto& fence : ctx.inFlightFences)
        ctx.device.destroyFence(fence);

    ctx.device.destroyCommandPool(ctx.swapchain.commandPool);

	ctx.device.destroySampler(ctx.offscreenSampler);

    ctx.device.destroy();

	ctx.instance.destroySurfaceKHR(ctx.surface);

	ctx.instance.destroy();
}

void update_uniform_buffer(VulkanContext& ctx, uint32_t currentImage, UniformBufferObject ubo)
{
    memcpy(ctx.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void update_light_buffer(VulkanContext& ctx, uint32_t currentImage, LightUBO ubo)
{   
    memcpy(ctx.lightBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void draw_frame(VulkanContext& ctx, AssetManager& assets, 
    const std::function<vk::Extent2D()>& getWindowSize, FramePacket framePacket,
    std::function<vk::Framebuffer(uint32_t imageIndex, uint32_t frame)> getFramebuffer)
{
    uint32_t frame = ctx.currentFrame;
    uint32_t semIdx = ctx.acquireSemaphoreIndex;

    vk::Result result;

    result = ctx.device.waitForFences(ctx.inFlightFences[frame], VK_TRUE, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        std::cout << "waitForFences failed!" << std::endl;
    }

    uint32_t imageIndex;
    result = ctx.device.acquireNextImageKHR(
        ctx.swapchain.swapchain,
        UINT64_MAX,
        ctx.imageAvailableSemaphores[semIdx],
        vk::Fence{},
        &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        vulkan_recreate_swapchain(ctx, getWindowSize);
        return;
    }
    else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to acquire swapchain image!");
    }

    ctx.device.resetFences(ctx.inFlightFences[frame]);

    ctx.swapchain.commandBuffers[frame].reset();

    update_light_buffer(ctx, frame, framePacket.lightUbo);

    update_uniform_buffer(ctx, frame, framePacket.ubo);

    vk::Framebuffer framebuffer = getFramebuffer(imageIndex, frame);
    record_command_buffer(ctx, assets, imageIndex, frame, 
        framePacket.drawCalls, framebuffer);

    vk::Semaphore waitSemaphores[] = { ctx.imageAvailableSemaphores[semIdx] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signalSemaphores[] = { ctx.renderFinishedSemaphores[frame] };

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(waitSemaphores)
        .setPWaitDstStageMask(waitStages)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&ctx.swapchain.commandBuffers[frame])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(signalSemaphores);

    ctx.graphicsQueue.submit(submitInfo, ctx.inFlightFences[frame]);

    vk::SwapchainKHR swapchains[] = { ctx.swapchain.swapchain };

    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(signalSemaphores)
        .setSwapchainCount(1)
        .setPSwapchains(swapchains)
        .setPImageIndices(&imageIndex)
        .setPResults(nullptr);

    ctx.presentQueue.presentKHR(&presentInfo);

    ctx.currentFrame = (frame + 1) % VulkanContext::MAX_FRAMES_IN_FLIGHT;
    ctx.acquireSemaphoreIndex = (semIdx + 1) % ctx.imageAvailableSemaphores.size();
}

static int vulkan_alloc_lighting_descriptor_sets(VulkanContext& ctx)
{
    std::vector<vk::DescriptorSetLayout> layouts(ctx.MAX_FRAMES_IN_FLIGHT, ctx.pipeline.lightingDescriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(ctx.descriptorPool)
        .setDescriptorSetCount(static_cast<uint32_t>(ctx.MAX_FRAMES_IN_FLIGHT))
        .setPSetLayouts(layouts.data());

    ctx.gBuffer.descriptorSets.resize(ctx.MAX_FRAMES_IN_FLIGHT);
    ctx.gBuffer.descriptorSets = ctx.device.allocateDescriptorSets(allocInfo);
    return 0;
}

int vulkan_update_lighting_descriptor_sets(VulkanContext& ctx)
{
    for (size_t i = 0; i < ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorImageInfo positionImageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(ctx.gBuffer.position.textureView);

        vk::DescriptorImageInfo normanImageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(ctx.gBuffer.normal.textureView);

        vk::DescriptorImageInfo albedoImageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(ctx.gBuffer.albedo.textureView);

        vk::DescriptorBufferInfo lightBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(ctx.lightBuffers[i])
            .setOffset(0)
            .setRange(sizeof(LightUBO));

        std::array<vk::WriteDescriptorSet, 4> descriptorWrites{};
        descriptorWrites[0]
            .setDstSet(ctx.gBuffer.descriptorSets[i])
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eInputAttachment)
            .setDescriptorCount(1)
            .setPImageInfo(&positionImageInfo);
        descriptorWrites[1]
            .setDstSet(ctx.gBuffer.descriptorSets[i])
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eInputAttachment)
            .setDescriptorCount(1)
            .setPImageInfo(&normanImageInfo);
        descriptorWrites[2]
            .setDstSet(ctx.gBuffer.descriptorSets[i])
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eInputAttachment)
            .setDescriptorCount(1)
            .setPImageInfo(&albedoImageInfo);
        descriptorWrites[3]
            .setDstSet(ctx.gBuffer.descriptorSets[i])
            .setDstBinding(3)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&lightBufferInfo);

        ctx.device.updateDescriptorSets(descriptorWrites, nullptr);
    }

    return 0;
}