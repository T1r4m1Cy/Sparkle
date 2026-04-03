#pragma once

#include "ECS.h"

void camera_set_active(World& world, Entity entity);

void camera_rotate(World& world, float xoffset, float yoffset);

void camera_move(World& world, glm::vec3 direction, float speed);

glm::vec3 camera_get_forward(World& world);

glm::vec3 camera_get_right(World& world);