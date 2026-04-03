#include <fstream>

#include "VulkanContext.h"

#include "VulkanUtils.h"

ImageInfo create_image(VulkanContext& ctx, uint32_t width, uint32_t height, vk::Format format,
    vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    ImageInfo imageInfoRet;

    vk::Extent3D extent = vk::Extent3D()
        .setWidth(width)
        .setHeight(height)
        .setDepth(1);

    vk::ImageCreateInfo imageInfo = vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setExtent(extent)
        .setMipLevels(1)
        .setArrayLayers(1)
        .setFormat(format)
        .setTiling(tiling)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setUsage(usage)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive);

    imageInfoRet.image = ctx.device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements =
        ctx.device.getImageMemoryRequirements(imageInfoRet.image);

    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(find_memory_type(ctx.physicalDevice, memRequirements.memoryTypeBits, properties));

    imageInfoRet.imageMemory = ctx.device.allocateMemory(allocInfo);

    ctx.device.bindImageMemory(imageInfoRet.image, imageInfoRet.imageMemory, 0);

    return imageInfoRet;
}

vk::ImageView create_image_view(VulkanContext& ctx, vk::Image image, vk::Format format,
    vk::ImageAspectFlagBits aspectFlags)
{
    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
        .setAspectMask(aspectFlags)
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo()
        .setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format)
        .setSubresourceRange(subresourceRange);

    return ctx.device.createImageView(viewInfo);
}

uint32_t find_memory_type(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return 0;
}

vk::CommandBuffer begin_single_time_commands(VulkanContext& ctx)
{
    vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandPool(ctx.swapchain.commandPool)
        .setCommandBufferCount(1);

    vk::CommandBuffer commandBuffer = ctx.device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void end_single_time_commands(VulkanContext& ctx, vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setPCommandBuffers(&commandBuffer);

    ctx.graphicsQueue.submit(submitInfo);

    ctx.graphicsQueue.waitIdle();

    ctx.device.freeCommandBuffers(ctx.swapchain.commandPool, commandBuffer);
}

vk::Format find_supported_format(VulkanContext& ctx, std::vector<vk::Format> candidates,
    vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates) {
        vk::FormatProperties props = ctx.physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear
            && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal
            && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

BufferInfo create_buffer(VulkanContext& ctx,
    vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties)
{
    vk::Buffer buffer;
    vk::DeviceMemory bufferMemory;

    vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    buffer = ctx.device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = ctx.device.getBufferMemoryRequirements(buffer);

    std::cout << "memRequirements.size: " << memRequirements.size << std::endl;

    vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memRequirements.size)
        .setMemoryTypeIndex(find_memory_type(ctx.physicalDevice, memRequirements.memoryTypeBits,
            properties));

    bufferMemory = ctx.device.allocateMemory(allocInfo);

    ctx.device.bindBufferMemory(buffer, bufferMemory, 0);

    BufferInfo bufferToReturn = { buffer, bufferMemory };

    return bufferToReturn;
}

int copy_buffer(VulkanContext& ctx, vk::Buffer srcBuffer, vk::Buffer dstBuffer,
    vk::DeviceSize size)
{
    vk::CommandBuffer commandBuffer = begin_single_time_commands(ctx);

    vk::BufferCopy copyRegion = vk::BufferCopy()
        .setSrcOffset(0)
        .setDstOffset(0)
        .setSize(size);

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    end_single_time_commands(ctx, commandBuffer);

    return 0;
}

int copy_buffer_to_image(VulkanContext& ctx, vk::Buffer buffer, vk::Image image,
    uint32_t width, uint32_t height)
{
    vk::CommandBuffer commandBuffer = begin_single_time_commands(ctx);

    vk::ImageSubresourceLayers imageSubresource = vk::ImageSubresourceLayers()
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setMipLevel(0)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    vk::BufferImageCopy region = vk::BufferImageCopy()
        .setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource(imageSubresource)
        .setImageOffset({ 0,0,0 })
        .setImageExtent({ width, height, 1 });

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal,
        region);

    end_single_time_commands(ctx, commandBuffer);

    return 0;
}

std::vector<char> read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

void transition_image_layout(VulkanContext& ctx, vk::Image image, vk::Format format,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::CommandBuffer commandBuffer = begin_single_time_commands(ctx);

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
        .setBaseMipLevel(0)
        .setLevelCount(1)
        .setBaseArrayLayer(0)
        .setLayerCount(1);

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);

        if (has_stencil_component(format)) {
            subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    }
    else
    {
        subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    }

    vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
        .setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setImage(image)
        .setSubresourceRange(subresourceRange);

    if (oldLayout == vk::ImageLayout::eUndefined
        && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier
            .setSrcAccessMask(vk::AccessFlagBits::eNone)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal
        && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined
        && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier
            .setSrcAccessMask(vk::AccessFlagBits::eNone)
            .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead
                | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier(
        sourceStage, destinationStage,
        {}, nullptr, nullptr, barrier
    );

    end_single_time_commands(ctx, commandBuffer);
}

bool has_stencil_component(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}