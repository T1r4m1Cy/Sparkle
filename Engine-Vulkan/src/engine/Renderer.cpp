#include <iostream>
#include <utility>

#include "ECS.h"
#include "Components.h"
#include "AssetManager.h"

#include "Renderer.h"

std::vector<Archetype*> archsTransformMesh;
std::vector<Archetype*> archsLight;

void init_renderer(std::vector<std::function<void(World&)>>& onWorldChanged)
{
	onWorldChanged.push_back([](World& world) {
		update_archs(world);
	});
}

void update_archs(World& world)
{
	archsTransformMesh = world_query<TransformComponent, MeshComponent>(world);
	archsLight = world_query<TransformComponent, PointLightComponent>(world);
}

FramePacket update_renderer(World& world, AssetManager& assets, float aspectRatio)
{
	FramePacket framePacket{};

	auto [camTransform, camComp] = world_get_current_camera(world);

	if (!camTransform || !camComp) {
		framePacket.ubo.view = glm::lookAt(
			glm::vec3(0.0f, 0.0f, 3.0f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f)
		);
		framePacket.ubo.proj = glm::perspective(
			glm::radians(45.0f),
			aspectRatio,
			0.1f,
			100.0f
		);
		framePacket.lightUbo.viewPos = glm::vec4(0.0f, 0.0f, 3.0f, 1.0f);
	}
	else {
		glm::vec3 direction{};
		direction.x = cos(glm::radians(camComp->yaw)) * cos(glm::radians(camComp->pitch));
		direction.y = sin(glm::radians(camComp->yaw)) * cos(glm::radians(camComp->pitch));
		direction.z = sin(glm::radians(camComp->pitch));

		framePacket.ubo.view = glm::lookAt(
			camTransform->position,
			camTransform->position + direction,
			glm::vec3(0.0f, 0.0f, 1.0f)
		);
		framePacket.ubo.proj = glm::perspective(
			glm::radians(camComp->fov),
			aspectRatio,
			camComp->nearPlane,
			camComp->farPlane
		);
		framePacket.lightUbo.viewPos = glm::vec4(camTransform->position, 1.0f);
	}

	framePacket.ubo.proj[1][1] *= -1;

	for (auto& arch : archsTransformMesh) {
		int transformColumnIndex = archetype_column_index(*arch, component_id<TransformComponent>());
		int meshColumnIndex = archetype_column_index(*arch, component_id<MeshComponent>());

		for (int i = 0; i < arch->entities.size(); i++) {
			TransformComponent* t = static_cast<TransformComponent*>(
				column_get(arch->columns[transformColumnIndex], i)
			);

			MeshComponent* m = static_cast<MeshComponent*>(
				column_get(arch->columns[meshColumnIndex], i)
			);

			GpuMesh* gpuMesh = assets.get_mesh(m->mesh);
			GpuTexture* gpuTexture = assets.get_texture(m->texture);

			if (!gpuMesh || !gpuTexture) continue;

			glm::mat4 model = glm::translate(glm::mat4(1.0f), t->position)
				* glm::mat4_cast(t->rotation)
				* glm::scale(glm::mat4(1.0f), t->scale);

			framePacket.drawCalls.push_back({ 
				m->mesh.id, m->mesh.version, 
				m->texture.id, m->texture.version, 
				model 
			});
		}
	}

	int lightCount = 0;
	for (auto& arch : archsLight) {
		int transformColumnIndex = archetype_column_index(*arch, component_id<TransformComponent>());
		int lightColumnIndex = archetype_column_index(*arch, component_id<PointLightComponent>());

		for (int i = 0; i < arch->entities.size(); i++) {
			TransformComponent* t = static_cast<TransformComponent*>(
				column_get(arch->columns[transformColumnIndex], i)
			);

			PointLightComponent* l = static_cast<PointLightComponent*>(
				column_get(arch->columns[lightColumnIndex], i)
			);
			
			framePacket.lightUbo.lightColor[lightCount] = glm::vec4(l->color * l->intensity, 1.0f);
			framePacket.lightUbo.lightPos[lightCount] = glm::vec4(t->position, 1.0f);

			lightCount++;
		}
	}

	framePacket.lightUbo.lightCount = lightCount;

	return framePacket;
}