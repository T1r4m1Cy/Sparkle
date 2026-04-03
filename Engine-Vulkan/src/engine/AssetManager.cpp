#include "VulkanContext.h"

#include "AssetManager.h"

void AssetManager::assets_shutdown(VulkanContext& ctx)
{
	for (auto& entry : meshes) {
		ctx.device.destroyBuffer(entry.asset.vertexBuffer);
		ctx.device.freeMemory(entry.asset.vertexBufferMemory);
		ctx.device.destroyBuffer(entry.asset.indexBuffer);
		ctx.device.freeMemory(entry.asset.indexBufferMemory);
	}

	for (auto& entry : textures) {
		ctx.device.destroySampler(entry.asset.sampler);
		ctx.device.destroyImageView(entry.asset.imageView);
		ctx.device.destroyImage(entry.asset.image);
		ctx.device.freeMemory(entry.asset.imageMemory);
	}
}

AssetHandle<GpuMesh> AssetManager::load_mesh(VulkanContext& ctx, const std::string& path)
{
	auto it = meshPathIndex.find(path);
	if (it != meshPathIndex.end()) {
		AssetHandle<GpuMesh> handle = meshPathIndex[path];
		meshes[handle.id].refCount++;
		return handle;
	}

	GpuMesh gpuMesh = upload_mesh(ctx, path);

	AssetEntry<GpuMesh> entry{};
	entry.asset = gpuMesh;
	entry.path = path;
	entry.version = 1;
	entry.refCount = 1;

	uint32_t id = static_cast<uint32_t>(meshes.size());
	meshes.push_back(entry);
	meshPathIndex[path] = { id, 1 };

	return { id, entry.version };
}

AssetHandle<GpuTexture> AssetManager::load_texture(VulkanContext& ctx, const std::string& path)
{
	auto it = texturePathIndex.find(path);
	if (it != texturePathIndex.end()) {
		AssetHandle<GpuTexture> handle = texturePathIndex[path];
		textures[handle.id].refCount++;
		return handle;
	}

	GpuTexture gpuTexture = upload_texture(ctx, path);

	AssetEntry<GpuTexture> entry{};
	entry.asset = gpuTexture;
	entry.path = path;
	entry.version = 0;
	entry.refCount = 1;

	uint32_t id = static_cast<uint32_t>(textures.size());
	textures.push_back(entry);
	texturePathIndex[path] = { id, 1 };

	return { id, entry.version };
}

void AssetManager::unload_mesh(VulkanContext& ctx, AssetHandle<GpuMesh> handle)
{
	if (!handle.is_valid()) return;
	auto& entry = meshes[handle.id];

	if (entry.version != handle.version) return;

	entry.refCount--;
	if (entry.refCount > 0) return;

	ctx.device.destroyBuffer(entry.asset.vertexBuffer);
	ctx.device.freeMemory(entry.asset.vertexBufferMemory);
	ctx.device.destroyBuffer(entry.asset.indexBuffer);
	ctx.device.freeMemory(entry.asset.indexBufferMemory);

	meshPathIndex.erase(entry.path);
	entry.path = "";
	entry.version++;
}

void AssetManager::unload_texture(VulkanContext& ctx, AssetHandle<GpuTexture> handle)
{
	if (!handle.is_valid()) return;
	auto& entry = meshes[handle.id];

	if (entry.version != handle.version) return;

	entry.refCount--;
	if (entry.refCount > 0) return;

	ctx.device.destroyBuffer(entry.asset.vertexBuffer);
	ctx.device.freeMemory(entry.asset.vertexBufferMemory);
	ctx.device.destroyBuffer(entry.asset.indexBuffer);
	ctx.device.freeMemory(entry.asset.indexBufferMemory);

	texturePathIndex.erase(entry.path);
	entry.path = "";
	entry.version++;
}

GpuMesh* AssetManager::get_mesh(AssetHandle<GpuMesh> handle)
{
	if (!handle.is_valid()) return nullptr;
	auto& entry = meshes[handle.id];
	if (entry.version != handle.version) return nullptr;
	return &entry.asset;
}

GpuTexture* AssetManager::get_texture(AssetHandle<GpuTexture> handle)
{
	if (!handle.is_valid()) return nullptr;
	auto& entry = textures[handle.id];
	if (entry.version != handle.version) return nullptr;
	return &entry.asset;
}

const std::string& AssetManager::get_mesh_path(AssetHandle<GpuMesh> handle)
{
	std::string empty = "";
	if (!handle.is_valid()) return empty;
	return meshes[handle.id].path;
}

const std::string& AssetManager::get_texture_path(AssetHandle<GpuTexture> handle)
{
	std::string empty = "";
	if (!handle.is_valid()) return empty;
	return textures[handle.id].path;
}