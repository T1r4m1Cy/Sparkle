#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "PropertyDrawers.h"
#include "ECS.h"

struct FieldInfo {
	std::string name;
	std::function<void(void*)> drawImGui;
};

struct ComponentInfo {
	std::string name;
	ComponentID id;
	std::function<void(World&, Entity)> add;
	std::function<void(World&, Entity)> remove;
	std::vector<FieldInfo> fields;
};

struct ComponentRegistry {
	std::unordered_map<ComponentID, ComponentInfo> components;

	void register_component(ComponentInfo info)
	{
		components[info.id] = info;
	}
};

ComponentRegistry& get_component_registry();

#define FIELD(Type, FieldName, Label) \
	FieldInfo{ \
		Label, \
		[](void* compPtr) { \
			auto* comp = static_cast<ComponentType*>(compPtr); \
			draw_field(Label "##" #FieldName, &comp->FieldName); \
		} \
	},

#define FIELD_COLOR(Type, FieldName, Label) \
	FieldInfo{ \
		Label, \
		[](void* compPtr) { \
			auto* comp = static_cast<ComponentType*>(compPtr); \
			draw_field_color(Label "##" #FieldName, &comp->FieldName); \
		} \
	},

#define REGISTER_COMPONENT(ComponentClass, DisplayName, ...) \
	namespace { \
		struct ComponentClass##_Registrar { \
			ComponentClass##_Registrar() { \
				using ComponentType = ComponentClass; \
				ComponentInfo info; \
				info.name = DisplayName; \
				info.id = component_id<ComponentClass>(); \
				info.add = [](World& w, Entity e) { \
					ComponentClass c{}; \
					world_add_component(w, e, c); \
				}; \
				info.remove = [](World& w, Entity e) { \
					world_remove_component<ComponentClass>(w, e); \
				}; \
				info.fields = { __VA_ARGS__ }; \
				get_component_registry().register_component(info); \
			} \
		}; \
		static ComponentClass##_Registrar \
			ComponentClass##_registrar_instance; \
	} 