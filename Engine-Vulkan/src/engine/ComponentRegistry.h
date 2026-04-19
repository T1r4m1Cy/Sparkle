#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ECS.h"

// Abstract field serializer — implemented by ComponentBuilder in src/runtime.
struct FieldWriter {
	virtual void write_float (const char* name, float v)                      = 0;
	virtual void write_vec3  (const char* name, float x, float y, float z)    = 0;
	virtual void write_bool  (const char* name, bool v)                       = 0;
	virtual void write_string(const char* name, const char* s)                = 0;
	virtual void write_color (const char* name, float r, float g, float b)    = 0;
	virtual ~FieldWriter() = default;
};

struct FieldInfo {
	std::string name;
	std::function<void(void*, const float*)> applyFloat; // null = read-only
	std::function<void(void*, FieldWriter&)> serialize;  // null = hidden
};

struct ComponentInfo {
	std::string name;
	ComponentID id;
	std::function<void(World&, Entity)>  add;
	std::function<void(World&, Entity)>  remove;
	std::function<void*(World&, Entity)> get;
	std::vector<FieldInfo>               fields;
};

struct ComponentRegistry {
	std::unordered_map<ComponentID, ComponentInfo> components;
	std::unordered_map<std::string, ComponentID>   byName;

	void register_component(ComponentInfo info) {
		byName[info.name] = info.id;
		components[info.id] = std::move(info);
	}

	void apply_field(World& world, Entity entity,
	                 const std::string& compName, const std::string& fieldName,
	                 const float* data) const {
		auto cit = byName.find(compName);
		if (cit == byName.end()) return;
		auto& info = components.at(cit->second);
		void* ptr = info.get(world, entity);
		if (!ptr) return;
		for (auto& f : info.fields)
			if (f.name == fieldName && f.applyFloat) { f.applyFloat(ptr, data); return; }
	}
};

ComponentRegistry& get_component_registry();

// Forces the linker to include ComponentRegistrations.obj from EngineLib.
// Call once at startup (e.g. from main_runtime.cpp) — static libraries strip
// unreferenced translation units, which silently empties the registry.
void register_engine_components();

// ── Field macros ──────────────────────────────────────────────────────────────
// ComponentType is set by REGISTER_COMPONENT via `using ComponentType = ...`

#define FIELD_FLOAT(FieldName, Label) \
	FieldInfo{ Label, \
		[](void* p, const float* f) { \
			static_cast<ComponentType*>(p)->FieldName = f[0]; \
		}, \
		[](void* p, FieldWriter& w) { \
			w.write_float(Label, static_cast<ComponentType*>(p)->FieldName); \
		} \
	},

#define FIELD_VEC3(FieldName, Label) \
	FieldInfo{ Label, \
		[](void* p, const float* f) { \
			static_cast<ComponentType*>(p)->FieldName = { f[0], f[1], f[2] }; \
		}, \
		[](void* p, FieldWriter& w) { \
			auto& v = static_cast<ComponentType*>(p)->FieldName; \
			w.write_vec3(Label, v.x, v.y, v.z); \
		} \
	},

// Receives/sends euler angles in degrees, stores as quaternion.
#define FIELD_QUAT_EULER(FieldName, Label) \
	FieldInfo{ Label, \
		[](void* p, const float* f) { \
			static_cast<ComponentType*>(p)->FieldName = \
				glm::quat(glm::radians(glm::vec3(f[0], f[1], f[2]))); \
		}, \
		[](void* p, FieldWriter& w) { \
			glm::vec3 euler = glm::degrees(glm::eulerAngles( \
				static_cast<ComponentType*>(p)->FieldName)); \
			w.write_vec3(Label, euler.x, euler.y, euler.z); \
		} \
	},

// Color field — rgb float[3], displayed as a color swatch in the editor.
#define FIELD_COLOR(FieldName, Label) \
	FieldInfo{ Label, \
		[](void* p, const float* f) { \
			static_cast<ComponentType*>(p)->FieldName = { f[0], f[1], f[2] }; \
		}, \
		[](void* p, FieldWriter& w) { \
			auto& v = static_cast<ComponentType*>(p)->FieldName; \
			w.write_color(Label, v.x, v.y, v.z); \
		} \
	},

#define FIELD_BOOL(FieldName, Label) \
	FieldInfo{ Label, \
		[](void* p, const float* f) { \
			static_cast<ComponentType*>(p)->FieldName = \
				(reinterpret_cast<const uint32_t*>(f)[0] != 0); \
		}, \
		[](void* p, FieldWriter& w) { \
			w.write_bool(Label, static_cast<ComponentType*>(p)->FieldName); \
		} \
	},

#define FIELD_STRING(FieldName, Label) \
	FieldInfo{ Label, \
		[](void* p, const float* f) { \
			auto* comp = static_cast<ComponentType*>(p); \
			const char* s = reinterpret_cast<const char*>(f); \
			strncpy(comp->FieldName, s, sizeof(comp->FieldName) - 1); \
			comp->FieldName[sizeof(comp->FieldName) - 1] = '\0'; \
		}, \
		[](void* p, FieldWriter& w) { \
			w.write_string(Label, static_cast<ComponentType*>(p)->FieldName); \
		} \
	},

// ── Component registration macro ─────────────────────────────────────────────

#define REGISTER_COMPONENT(ComponentClass, DisplayName, ...) \
	namespace { \
		struct ComponentClass##_Registrar { \
			ComponentClass##_Registrar() { \
				using ComponentType = ComponentClass; \
				ComponentInfo info; \
				info.name = DisplayName; \
				info.id   = component_id<ComponentClass>(); \
				info.add  = [](World& w, Entity e) { \
					ComponentClass c{}; world_add_component(w, e, c); \
				}; \
				info.remove = [](World& w, Entity e) { \
					world_remove_component<ComponentClass>(w, e); \
				}; \
				info.get = [](World& w, Entity e) -> void* { \
					return world_get_component<ComponentClass>(w, e); \
				}; \
				info.fields = { __VA_ARGS__ }; \
				get_component_registry().register_component(std::move(info)); \
			} \
		}; \
		static ComponentClass##_Registrar ComponentClass##_registrar_instance; \
	}
