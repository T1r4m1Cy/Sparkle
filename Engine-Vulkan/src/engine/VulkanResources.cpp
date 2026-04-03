#define STB_IMAGE_IMPLEMENTATION
#include "libraries/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
//#define FAST_FLOAT_SKIP_WHITE_SPACE
#define TINYOBJLOADER_USE_DOUBLE
#include "libraries/tiny_obj_loader.h"

#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "AssetManager.h"
#include "RendererData.h"

#include "VulkanResources.h"

Mesh load_model(const std::string& path);
ImageInfo create_texture_image(VulkanContext& ctx, const std::string& texPath);

int create_descriptor_sets(VulkanContext& ctx, GpuTexture& gpuTexture);


BufferInfo create_vertex_buffer(VulkanContext& ctx, Mesh& mesh);
BufferInfo create_index_buffer(VulkanContext& ctx, Mesh& mesh);

GpuMesh upload_mesh(VulkanContext& ctx, const std::string& path)
{
    Mesh mesh = load_model(path);

    GpuMesh gpuMesh{};

    BufferInfo vertexBufferInfo = create_vertex_buffer(ctx, mesh);

    gpuMesh.vertexBuffer = vertexBufferInfo.buffer;
    gpuMesh.vertexBufferMemory = vertexBufferInfo.bufferMemory;

    BufferInfo indexBufferInfo = create_index_buffer(ctx, mesh);

    gpuMesh.indexBuffer = indexBufferInfo.buffer;
    gpuMesh.indexBufferMemory = indexBufferInfo.bufferMemory;

    gpuMesh.indexCount = static_cast<uint32_t>(mesh.indices.size());

    return gpuMesh;
}

Mesh load_model(const std::string& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    Mesh mesh;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
                mesh.vertices.push_back(vertex);
            }

            mesh.indices.push_back(uniqueVertices[vertex]);
        }
    }

    return mesh;
}

GpuTexture upload_texture(VulkanContext& ctx, const std::string& path)
{
    GpuTexture gpuTexture{};

    ImageInfo imageInfo = create_texture_image(ctx, path);

    gpuTexture.image = imageInfo.image;
    gpuTexture.imageMemory = imageInfo.imageMemory;

    gpuTexture.imageView = create_image_view(ctx, gpuTexture.image,
        vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

    gpuTexture.sampler = vulkan_init_texture_sampler(ctx);

    create_descriptor_sets(ctx, gpuTexture);

    return gpuTexture;
}

int create_descriptor_sets(VulkanContext& ctx, GpuTexture& gpuTexture)
{
    std::vector<vk::DescriptorSetLayout> layouts(ctx.MAX_FRAMES_IN_FLIGHT, ctx.pipeline.descriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
        .setDescriptorPool(ctx.descriptorPool)
        .setDescriptorSetCount(static_cast<uint32_t>(ctx.MAX_FRAMES_IN_FLIGHT))
        .setPSetLayouts(layouts.data());

    gpuTexture.descriptorSets.resize(ctx.MAX_FRAMES_IN_FLIGHT);
    gpuTexture.descriptorSets = ctx.device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < ctx.MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo uniformBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(ctx.uniformBuffers[i])
            .setOffset(0)
            .setRange(sizeof(UniformBufferObject));

        vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(gpuTexture.imageView)
            .setSampler(gpuTexture.sampler);

        vk::DescriptorBufferInfo lightBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(ctx.lightBuffers[i])
            .setOffset(0)
            .setRange(sizeof(LightUBO));

        std::array<vk::WriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0]
            .setDstSet(gpuTexture.descriptorSets[i])
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&uniformBufferInfo);
        descriptorWrites[1]
            .setDstSet(gpuTexture.descriptorSets[i])
            .setDstBinding(1)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setPImageInfo(&imageInfo);
        descriptorWrites[2]
            .setDstSet(gpuTexture.descriptorSets[i])
            .setDstBinding(2)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setPBufferInfo(&lightBufferInfo);

        ctx.device.updateDescriptorSets(descriptorWrites, nullptr);
    }

    return 0;
}

int create_descriptor_pool(VulkanContext& ctx)
{
    std::array<vk::DescriptorPoolSize, 4> poolSizes{};
    poolSizes[0]
        .setType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(static_cast<uint32_t>(ctx.MAX_FRAMES_IN_FLIGHT * ctx.descriptorPoolCapacity));
    poolSizes[1]
        .setType(vk::DescriptorType::eCombinedImageSampler)
        .setDescriptorCount(static_cast<uint32_t>(ctx.MAX_FRAMES_IN_FLIGHT * ctx.descriptorPoolCapacity));
    poolSizes[2]
        .setType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(static_cast<uint32_t>(ctx.MAX_FRAMES_IN_FLIGHT * ctx.descriptorPoolCapacity));
    poolSizes[3]
        .setType(vk::DescriptorType::eInputAttachment)
        .setDescriptorCount(static_cast<uint32_t>(ctx.MAX_FRAMES_IN_FLIGHT * ctx.descriptorPoolCapacity));

    vk::DescriptorPoolCreateInfo poolInfo = vk::DescriptorPoolCreateInfo()
        .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
        .setPPoolSizes(poolSizes.data())
        .setMaxSets(static_cast<uint32_t>(ctx.MAX_FRAMES_IN_FLIGHT * ctx.descriptorPoolCapacity));

    ctx.descriptorPool = ctx.device.createDescriptorPool(poolInfo);

    return 0;
}

void recreate_decriptor_pool(VulkanContext& ctx, AssetManager& assets)
{
    ctx.device.waitIdle();
    ctx.device.destroyDescriptorPool(ctx.descriptorPool);

    ctx.descriptorPoolCapacity *= 2;
    create_descriptor_pool(ctx);

    for (auto& texture : assets.textures) {
        create_descriptor_sets(ctx, texture.asset);
    }
}

ImageInfo create_texture_image(VulkanContext& ctx, const std::string& texPath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texPath.c_str(),
        &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    BufferInfo stagingBufferInfo = create_buffer(ctx, imageSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = ctx.device.mapMemory(stagingBufferInfo.bufferMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    ctx.device.unmapMemory(stagingBufferInfo.bufferMemory);

    stbi_image_free(pixels);

    ImageInfo imageInfo = create_image(ctx, texWidth, texHeight, vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

    transition_image_layout(ctx, imageInfo.image, vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    copy_buffer_to_image(ctx, stagingBufferInfo.buffer, imageInfo.image,
        static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    transition_image_layout(ctx, imageInfo.image, vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    ctx.device.destroyBuffer(stagingBufferInfo.buffer);
    ctx.device.freeMemory(stagingBufferInfo.bufferMemory);

    return imageInfo;
}

vk::Sampler vulkan_init_texture_sampler(VulkanContext& ctx)
{
    vk::PhysicalDeviceProperties properties = ctx.physicalDevice.getProperties();

    vk::SamplerCreateInfo samplerInfo = vk::SamplerCreateInfo()
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setAnisotropyEnable(true)
        .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
        .setUnnormalizedCoordinates(false)
        .setCompareEnable(false)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMipLodBias(0.0f)
        .setMinLod(0.0f)
        .setMaxLod(0.0f);

    return ctx.device.createSampler(samplerInfo);
}

BufferInfo create_vertex_buffer(VulkanContext& ctx, Mesh& mesh)
{
    vk::DeviceSize bufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();

    BufferInfo stagingBufferInfo = create_buffer(ctx, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = ctx.device.mapMemory(stagingBufferInfo.bufferMemory, 0, bufferSize);
    memcpy(data, mesh.vertices.data(), (size_t)bufferSize);
    ctx.device.unmapMemory(stagingBufferInfo.bufferMemory);

    BufferInfo vertexBufferInfo = create_buffer(ctx, bufferSize, vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    copy_buffer(ctx, stagingBufferInfo.buffer, vertexBufferInfo.buffer, bufferSize);

    ctx.device.destroyBuffer(stagingBufferInfo.buffer);
    ctx.device.freeMemory(stagingBufferInfo.bufferMemory);

    return vertexBufferInfo;
}

BufferInfo create_index_buffer(VulkanContext& ctx, Mesh& mesh)
{
    vk::DeviceSize bufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();

    BufferInfo stagingBufferInfo = create_buffer(ctx, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = ctx.device.mapMemory(stagingBufferInfo.bufferMemory, 0, bufferSize);
    memcpy(data, mesh.indices.data(), (size_t)bufferSize);
    ctx.device.unmapMemory(stagingBufferInfo.bufferMemory);

    BufferInfo indexBufferInfo = create_buffer(ctx, bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    copy_buffer(ctx, stagingBufferInfo.buffer, indexBufferInfo.buffer, bufferSize);

    ctx.device.destroyBuffer(stagingBufferInfo.buffer);
    ctx.device.freeMemory(stagingBufferInfo.bufferMemory);

    return indexBufferInfo;
}