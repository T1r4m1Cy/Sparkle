#include "ComponentRegistry.h"

ComponentRegistry& get_component_registry()
{
	static ComponentRegistry registry;
	return registry;
}