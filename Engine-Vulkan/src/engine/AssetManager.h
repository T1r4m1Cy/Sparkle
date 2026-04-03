#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "AssetHandle.h"
#include "VulkanResources.h"

template<typename T>
struct AssetEntry {
	T asset;
	std::string path;
	uint32_t version = 0;
	uint32_t refCount = 0;
};

struct VulkanContext;

struct AssetManager {
	void assets_shutdown(VulkanContext& ctx);

	std::vector<AssetEntry<GpuMesh>> meshes;
	std::vector<AssetEntry<GpuTexture>> textures;

	std::unordered_map<std::string, AssetHandle<GpuMesh>> meshPathIndex;
	std::unordered_map<std::string, AssetHandle<GpuTexture>> texturePathIndex;

	AssetHandle<GpuMesh> load_mesh(
		VulkanContext& ctx, const std::string& path);
	AssetHandle<GpuTexture> load_texture(
		VulkanContext& ctx, const std::string& path);

	void unload_mesh(VulkanContext& ctx, AssetHandle<GpuMesh> handle);
	void unload_texture(VulkanContext& ctx, AssetHandle<GpuTexture> handle);

	GpuMesh* get_mesh(AssetHandle<GpuMesh> handle);
	GpuTexture* get_texture(AssetHandle<GpuTexture> handle);

	const std::string& get_mesh_path(AssetHandle<GpuMesh> handle);
	const std::string& get_texture_path(AssetHandle<GpuTexture> handle);
};