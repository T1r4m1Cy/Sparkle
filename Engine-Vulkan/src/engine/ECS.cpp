#include "ECS.h"

Column column_create(size_t elementSize)
{
	Column col;

	col.data = nullptr;
	col.elementSize = elementSize;
	col.count = 0;
	col.capacity = 0;

	return col;
}

void column_push(Column& col, void* data)
{
	if (col.count == col.capacity) {
		col.capacity = col.capacity == 0 ? 8 : col.capacity * 2;
		void* newData = realloc(col.data, col.elementSize * col.capacity);
		if (newData == nullptr)
		{
			std::free(col.data);
			col.data = nullptr;
			return;
		}
		col.data = newData;
	}

	std::memcpy((char*)col.data + col.elementSize * col.count, (char*)data, col.elementSize);

	col.count++;
}

void* column_get(Column& col, uint32_t row)
{
	return (char*)col.data + col.elementSize * row;
}

void column_swap_remove(Column& col, uint32_t rowToRemove)
{
	col.count--;

	if (col.count == rowToRemove) {
		return;
	}

	std::memcpy((char*)col.data + col.elementSize * rowToRemove, (char*)col.data + col.elementSize * col.count, col.elementSize);
}

void column_delete(Column& col)
{
	std::free(col.data);
}

World world_create()
{
	World world;

	return world;
}

Entity world_create_entity(World& world)
{
	return world.nextEntity++;
}

void world_destroy_entity(World& world, Entity entity)
{
	auto entityIt = world.entityIndex.find(entity);
	if (entityIt == world.entityIndex.end()) return;

	EntityRecord rec = entityIt->second;

	auto archIt = world.archetypesIndex.find(rec.archetypeID);
	if (archIt == world.archetypesIndex.end()) return;

	Archetype* arch = archIt->second;

	uint32_t row = rec.row;

	for (auto& col : arch->columns) {
		column_swap_remove(col, row);
	}

	arch->entities[row] = arch->entities.back();
	arch->entities.pop_back();

	if (row < arch->entities.size()) {
		Entity movedEntity = arch->entities[row];
		world.entityIndex[movedEntity].row = row;
	}

	if (world.currentCameraEntity == entity) {
		world.currentCameraEntity = NULL_ENTITY;
	}

	world.entityIndex.erase(entityIt);

	world.notify_world_changed();
}

Archetype* find_archetype(World& world, const std::vector<ComponentID>& type)
{
	for (auto& arch : world.archetypes) {
		if (arch.get()->componentIDs == type) {
			return arch.get();
		}
	}

	return nullptr;
}

const Archetype* find_archetype(const World& world, ArchetypeID archID)
{
	auto it = world.archetypesIndex.find(archID);
	
	if (it == world.archetypesIndex.end()) return nullptr;

	return it->second;
}

Archetype* find_archetype(World& world, ArchetypeID archID)
{
	auto it = world.archetypesIndex.find(archID);

	if (it == world.archetypesIndex.end()) return nullptr;

	return it->second;
}

/*Archetype* get_or_create_archetype(World& world, ArchetypeID id)
{
	auto it = world.archetypesIndex.find(id);

	if (it != world.archetypesIndex.end()) return it->second;

	return create_archetype(world);
}

Archetype* get_or_create_archetype(World& world, const std::vector<ComponentID>& type)
{
	for (auto& arch : world.archetypes) {
		if (arch.get()->componentIDs == type) {
			return arch.get();
		}
	}

	return create_archetype(world);
}*/

Archetype* get_initial_archetype(World& world, ComponentID id)
{
	auto it = world.initialArchetypes.find(id);

	if (it != world.initialArchetypes.end()) return it->second;

	return nullptr;
}

Archetype* find_archetype_by_adding_edge(World& world, Archetype* arch,
	ComponentID id)
{
	auto edgesIt = arch->addEdges.find(id);

	if (edgesIt != arch->addEdges.end()) return edgesIt->second;

	return nullptr;
}

Archetype* find_archetype_by_removing_edge(World& world, Archetype* arch,
	ComponentID id)
{
	auto edgesIt = arch->removeEdges.find(id);

	if (edgesIt != arch->removeEdges.end()) return edgesIt->second;

	return nullptr;
}

Archetype* create_archetype(World& world)
{
	auto arch = std::make_unique<Archetype>();

	Archetype* ptr = arch.get();
	arch->archetypeID = next_archetype_id();

	world.archetypes.push_back(std::move(arch));
	world.archetypesIndex[ptr->archetypeID] = ptr;

	return ptr;
}

int archetype_column_index(const Archetype& arch, ComponentID id)
{
	auto it = arch.componentIndex.find(id);
	if (it == arch.componentIndex.end()) return -1;
	return it->second;
}

int archetype_entity_index(const Archetype& arch, Entity e)
{
	for (int i = 0; i < arch.entities.size(); i++) {
		if (arch.entities[i] == e) return i;
	}

	return -1;
}

void debug_world(World& world)
{
	std::cout << "\n--- World State ---" << std::endl;
	std::cout << "Archetypes: " << world.archetypes.size() << std::endl;

	for (auto& arch : world.archetypes) {
		std::cout << "  Archetype " << arch.get()->archetypeID << ": [";
		for (int i = 0; i < arch.get()->componentIDs.size(); i++) {
			std::cout << arch.get()->componentIDs[i];
			if (i < arch.get()->componentIDs.size() - 1) std::cout << ", ";
		}
		std::cout << "] - entities: " << arch.get()->entities.size() << " [";
		for (int i = 0; i < arch.get()->entities.size(); i++) {
			std::cout << arch.get()->entities[i];
			if (i < arch.get()->entities.size() - 1) std::cout << ", ";
		}
		std::cout << "]" << std::endl;
	}
	std::cout << "Initial Archetypes: [";
	int i = 0;
	for (auto& arch : world.initialArchetypes) {
		std::cout << arch.second->archetypeID;
		if (i < world.initialArchetypes.size() - 1) std::cout << ", ";
		i++;
	}
	std::cout << "]" << std::endl;
	std::cout << "-------------------\n" << std::endl;
}