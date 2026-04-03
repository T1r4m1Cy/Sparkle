#include "Components.h"

#include "CameraSystem.h"

void camera_set_active(World& world, Entity entity)
{
	world.currentCameraEntity = entity;

	world.notify_current_camera_changed(entity);
}

void camera_rotate(World& world, float xoffset, float yoffset)
{
	auto [t, cam] = world_get_current_camera(world);
	if (!t || !cam) return;

	cam->yaw -= xoffset;
	cam->pitch -= yoffset;
	cam->pitch = glm::clamp(cam->pitch, -89.0f, 89.0f);
}

void camera_move(World& world, glm::vec3 direction, float speed)
{
	auto [t, cam] = world_get_current_camera(world);
	if (!t || !cam) return;

	t->position += direction * speed;
}

glm::vec3 camera_get_forward(World& world)
{
	auto [t, cam] = world_get_current_camera(world);
	if (!t || !cam) return glm::vec3(0.0f);

	float yawRad = glm::radians(cam->yaw);
	float pitchRad = glm::radians(cam->pitch);

	return glm::normalize(glm::vec3(
		cos(yawRad) * cos(pitchRad),
		sin(yawRad) * cos(pitchRad),
		sin(pitchRad)
	));
}

glm::vec3 camera_get_right(World& world)
{
	glm::vec3 forward = camera_get_forward(world);
	if (forward == glm::vec3(0.0f)) return glm::vec3(0.0f);

	return glm::normalize(glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f)));
}