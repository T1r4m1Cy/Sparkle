#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

#include "AssetHandle.h"

struct GpuMesh;
struct GpuTexture;

struct NameComponent {
	std::string name = "Entity";
};

struct TransformComponent {
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
};

struct MeshComponent {
	AssetHandle<GpuMesh> mesh;
	AssetHandle<GpuTexture> texture;
};

struct DirectionalLightComponent {
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };
	float intensity = 1.0f;
};

struct PointLightComponent {
	glm::vec3 color = { 1.0f, 1.0f, 1.0f, };
	float intensity = 1.0f;
	float radius = 10.0f;
};

struct SpotLightComponent {
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };
	float intensity = 1.0f;
	float radius = 15.0f;
	float cutoff = 12.5f;
	float outerCutOff = 15.0f;
};

struct CameraComponent {
	float yaw = 0.0f;
	float pitch = 0.0f;
	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;
	bool controlledByPlayer = false;
};