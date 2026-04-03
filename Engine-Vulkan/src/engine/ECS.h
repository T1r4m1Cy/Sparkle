#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <memory>

#include "Components.h"

using ArchetypeID = uint32_t;
using ComponentID = uint32_t;
using Entity = uint32_t;

static constexpr Entity NULL_ENTITY = UINT32_MAX;

struct Column {
	void* data;
	size_t elementSize;
	size_t count;
	size_t capacity;
};

Column column_create(size_t elementSize);
void column_push(Column& col, void* data);
void* column_get(Column& col, uint32_t row);
void column_swap_remove(Column& col, uint32_t rowToRemove);
void column_delete(Column& col);

struct Archetype {
	ArchetypeID archetypeID;
	std::vector<ComponentID> componentIDs;
	std::unordered_map<ComponentID, uint32_t> componentIndex;
	std::vector<Column> columns;
	std::vector<Entity> entities;

	std::unordered_map<ComponentID, Archetype*> addEdges;
	std::unordered_map<ComponentID, Archetype*> removeEdges;

	~Archetype() {
		for (auto& col : columns) {
			column_delete(col);
		}
	}
};

struct EntityRecord {
	ArchetypeID archetypeID;
	uint32_t row;
};

struct World {
	std::vector<std::unique_ptr<Archetype>> archetypes;
	std::unordered_map<ArchetypeID, Archetype*> archetypesIndex;
	std::unordered_map<Entity, EntityRecord> entityIndex;

	std::unordered_map<ComponentID, Archetype*> initialArchetypes;

	Entity nextEntity = 0;

	Entity currentCameraEntity = NULL_ENTITY;

	std::vector<std::function<void(World&)>> onWorldChanged;
	void notify_world_changed() {
		for (auto& callback : onWorldChanged) {
			callback(*this);
		}
	}

	std::vector<std::function<void(World&, Entity)>> onCameraChanged;
	void notify_current_camera_changed(Entity e) {
		for (auto& callback : onCameraChanged) {
			callback(*this, e);
		}
	}
};

inline ComponentID next_component_id() {
	static ComponentID counter = 0;
	return counter++;
}

template<typename T>
inline ComponentID component_id() {
	static ComponentID id = next_component_id();
	return id;
}

inline ArchetypeID next_archetype_id() {
	static ArchetypeID counter = 0;
	return counter++;
}

World world_create();
Entity world_create_entity(World& world);
void world_destroy_entity(World& world, Entity entity);

template<typename T>
void world_add_component(World& world, Entity entity, T& component);
Archetype* find_archetype(World& world, const std::vector<ComponentID>& type);
int archetype_column_index(const Archetype& arch, ComponentID id);

template<typename T>
T* world_get_component(World& world, Entity entity);

template<typename... Components>
std::vector<Archetype*> world_query(World& world);

Archetype* create_archetype(World& world);
Archetype* get_initial_archetype(World& world, ComponentID id);
Archetype* find_archetype_by_adding_edge(World& world, Archetype* arch,
	ComponentID id);
Archetype* find_archetype_by_removing_edge(World& world, Archetype* arch,
	ComponentID id);

Archetype* find_archetype(World& world, const std::vector<ComponentID>& type);
const Archetype* find_archetype(const World& world, ArchetypeID archID);
Archetype* find_archetype(World& world, ArchetypeID archID);
//Archetype* get_or_create_archetype(World& world, ArchetypeID id);
//Archetype* get_or_create_archetype(World& world, const std::vector<ComponentID>& type);

void debug_world(World& world);

template<typename T>
void world_add_component(World& world, Entity entity, T& component)
{
	auto entityIt = world.entityIndex.find(entity);
	if (entityIt == world.entityIndex.end()) {
		ComponentID firstType = component_id<T>();
		Archetype* arch = get_initial_archetype(world, firstType);
		if (arch == nullptr) {
			arch = create_archetype(world);
			arch->componentIDs.push_back(firstType);
			arch->componentIndex[component_id<T>()] = 0;
			arch->columns.push_back(column_create(sizeof(T)));

			world.initialArchetypes[firstType] = arch;
		}

		column_push(arch->columns[0], &component);
		arch->entities.push_back(entity);
		world.entityIndex[entity] = { arch->archetypeID,(uint32_t)arch->entities.size() - 1 };
		world.notify_world_changed();
		return;
	}

	EntityRecord& rec = entityIt->second;

	Archetype* oldArch = world.archetypesIndex[rec.archetypeID];
	uint32_t entityRow = rec.row;

	Archetype* newArch = find_archetype_by_adding_edge(
		world, oldArch, component_id<T>());

	if (newArch == nullptr) {
		std::vector<ComponentID> newType = oldArch->componentIDs;
		newType.push_back(component_id<T>());
		std::sort(newType.begin(), newType.end());

		newArch = find_archetype(world, newType);

		if (newArch == nullptr) {
			newArch = create_archetype(world);
			newArch->componentIDs = newType;

			int i = 0;
			for (ComponentID id : newType) {
				int oldIdx = archetype_column_index(*oldArch, id);
				if (oldIdx != -1) {
					newArch->columns.push_back(column_create(oldArch->columns[oldIdx].elementSize));
					newArch->componentIndex[id] = i;
				}
				else {
					newArch->columns.push_back(column_create(sizeof(T)));
					newArch->componentIndex[id] = i;
				}
				i++;
			}
		}

		oldArch->addEdges[component_id<T>()] = newArch;
		newArch->removeEdges[component_id<T>()] = oldArch;
	}

	for (int i = 0; i < oldArch->componentIDs.size(); i++) {
		void* srcData = column_get(oldArch->columns[i], entityRow);
		int newIdx = archetype_column_index(*newArch, oldArch->componentIDs[i]);
		column_push(newArch->columns[newIdx], srcData);
	}

	int newColIdx = archetype_column_index(*newArch, component_id<T>());
	column_push(newArch->columns[newColIdx], &component);

	for (auto& col : oldArch->columns) {
		column_swap_remove(col, entityRow);
	}
	oldArch->entities[entityRow] = oldArch->entities.back();
	oldArch->entities.pop_back();

	newArch->entities.push_back(entity);
	world.entityIndex[entity] = { newArch->archetypeID, (uint32_t)newArch->entities.size() - 1 };

	if (entityRow < oldArch->entities.size()) {
		Entity movedEntity = oldArch->entities[entityRow];
		world.entityIndex[movedEntity].row = entityRow;
	}

	world.notify_world_changed();
}

template<typename T>
void world_remove_component(World& world, Entity entity)
{
	auto entityIt = world.entityIndex.find(entity);
	if (entityIt == world.entityIndex.end()) {
		return;
	}

	EntityRecord& rec = entityIt->second;

	Archetype* oldArch = world.archetypesIndex[rec.archetypeID];
	uint32_t entityRow = rec.row;

	Archetype* newArch = find_archetype_by_removing_edge(
		world, oldArch, component_id<T>());

	if (newArch == nullptr) {
		std::vector<ComponentID> newType;
		for (int i = 0; i < oldArch->componentIDs.size(); i++) {
			if (oldArch->componentIDs[i] != component_id<T>()) {
				newType.push_back(oldArch->componentIDs[i]);
			}
		}
		std::sort(newType.begin(), newType.end());

		newArch = find_archetype(world, newType);

		if (newArch == nullptr) {
			newArch = create_archetype(world);
			newArch->componentIDs = newType;

			int i = 0;
			for (ComponentID id : newType) {
				int oldIdx = archetype_column_index(*oldArch, id);
				newArch->columns.push_back(column_create(oldArch->columns[oldIdx].elementSize));
				newArch->componentIndex[id] = i++;
			}
		}

		oldArch->removeEdges[component_id<T>()] = newArch;
		newArch->addEdges[component_id<T>()] = oldArch;
	}

	for (int i = 0; i < oldArch->componentIDs.size(); i++) {
		void* srcData = column_get(oldArch->columns[i], entityRow);
		int newIdx = archetype_column_index(*newArch, oldArch->componentIDs[i]);
		if (oldArch->componentIDs[i] == component_id<T>()) continue;
		column_push(newArch->columns[newIdx], srcData);
	}

	for (auto& col : oldArch->columns) {
		column_swap_remove(col, entityRow);
	}
	oldArch->entities[entityRow] = oldArch->entities.back();
	oldArch->entities.pop_back();

	newArch->entities.push_back(entity);
	world.entityIndex[entity] = { newArch->archetypeID, (uint32_t)newArch->entities.size() - 1 };

	if (entityRow < oldArch->entities.size()) {
		Entity movedEntity = oldArch->entities[entityRow];
		world.entityIndex[movedEntity].row = entityRow;
	}

	world.notify_world_changed();
}

template<typename T>
T* world_get_component(World& world, Entity entity)
{
	auto entityIt = world.entityIndex.find(entity);
	if (entityIt == world.entityIndex.end()) return nullptr;

	EntityRecord rec = entityIt->second;

	auto archIt = world.archetypesIndex.find(rec.archetypeID);
	if (archIt == world.archetypesIndex.end()) return nullptr;

	Archetype* arch = archIt->second;
	ComponentID comID = component_id<T>();

	auto compIt = arch->componentIndex.find(comID);
	if (compIt == arch->componentIndex.end()) return nullptr;

	return static_cast<T*>(column_get(arch->columns[compIt->second], rec.row));
}

template<typename... Components>
std::vector<Archetype*> world_query(World& world)
{
	std::vector<ComponentID> required = { component_id<Components>()... };
	std::vector<Archetype*> result;

	for (auto& arch : world.archetypes) {
		bool hasAll = true;
		for (auto id : required) {
			if (archetype_column_index(*arch, id) == -1) {
				hasAll = false;
				break;
			}
		}
		if (hasAll) result.push_back(arch.get());
	}

	return result;
}

inline std::pair<TransformComponent*, CameraComponent*> world_get_current_camera(World& world)
{
	if (world.currentCameraEntity == NULL_ENTITY)
		return { nullptr, nullptr };

	return {
		world_get_component<TransformComponent>(world, world.currentCameraEntity),
		world_get_component<CameraComponent>(world, world.currentCameraEntity)
	};
}