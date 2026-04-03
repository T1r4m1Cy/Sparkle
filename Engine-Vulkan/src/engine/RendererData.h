#pragma once

#include <glm/glm.hpp>

inline constexpr int MAX_LIGHTS = 4;

struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
};

struct LightUBO {
	glm::vec4 lightPos[MAX_LIGHTS];
	glm::vec4 lightColor[MAX_LIGHTS];
	glm::vec4 viewPos;
	int lightCount;
	glm::vec3 padding;
};

struct DrawCall {
	uint32_t meshId;
	uint32_t meshVersion;
	uint32_t textureId;
	uint32_t textureVersion;
	glm::mat4 model;
};

struct FramePacket {
	UniformBufferObject ubo;
	LightUBO lightUbo;
	std::vector<DrawCall> drawCalls;
};