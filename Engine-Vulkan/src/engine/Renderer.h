#pragma once

#include <iostream>

#include "RendererData.h"

struct World;
struct VulkanContext;
struct AssetManager;

void init_renderer(std::vector<std::function<void(World&)>>& onWorldChanged);
void update_archs(World& world);

FramePacket update_renderer(World& world, AssetManager& assets, float aspectRatio);