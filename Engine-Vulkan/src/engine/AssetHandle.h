#pragma once

#include <cstdint>

struct GpuMesh;
struct GpuTexture;

template<typename T>
struct AssetHandle {
	uint32_t id = UINT32_MAX;
	uint32_t version = 0;

	bool is_valid() const {
		return id != UINT32_MAX;
	}

	bool operator==(const AssetHandle& other) const {
		return id == other.id && version == other.version;
	}
};

using MeshHandle = AssetHandle<GpuMesh>;
using TextureHandle = AssetHandle<GpuTexture>;