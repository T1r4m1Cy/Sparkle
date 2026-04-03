#pragma once

#include <vulkan/vulkan.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static vk::VertexInputBindingDescription get_binding_description() {
		vk::VertexInputBindingDescription bindingDescription = vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);

		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 4> get_attribute_descriptions() {
		std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions{};

		attributeDescriptions[0]
			.setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, pos));
		attributeDescriptions[1]
			.setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, color));
		attributeDescriptions[2]
			.setBinding(0)
			.setLocation(2)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(offsetof(Vertex, texCoord));
		attributeDescriptions[3]
			.setBinding(0)
			.setLocation(3)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, normal));

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct GpuTexture {
	vk::Image image;
	vk::DeviceMemory imageMemory;
	vk::ImageView imageView;
	vk::Sampler sampler;
	std::vector<vk::DescriptorSet> descriptorSets;
};

struct TextureStorage {
	std::vector<GpuTexture> textures;
};

struct GpuMesh {
	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferMemory;
	vk::Buffer indexBuffer;
	vk::DeviceMemory indexBufferMemory;
	uint32_t indexCount;
};

struct MeshStorage {
	std::vector<GpuMesh> meshes;
};

struct GBufferAttachment {
	vk::Image texture;
	vk::DeviceMemory textureMemory;
	vk::ImageView textureView;
};

struct GBuffer {
	GBufferAttachment position;
	GBufferAttachment normal;
	GBufferAttachment albedo;
	std::vector<vk::DescriptorSet> descriptorSets;
};

struct OffscreenBuffer {
	vk::Image image;
	vk::DeviceMemory imageMemory;
	vk::ImageView imageView;
	vk::Framebuffer framebuffer;
	VkDescriptorSet imguiDescriptorSet = VK_NULL_HANDLE;
};

struct VulkanContext;

GpuTexture upload_texture(VulkanContext& ctx, const std::string& path);
GpuMesh upload_mesh(VulkanContext& ctx, const std::string& path);

vk::Sampler vulkan_init_texture_sampler(VulkanContext& ctx);

int create_descriptor_pool(VulkanContext& ctx);
void recreate_decriptor_pool(VulkanContext& ctx);