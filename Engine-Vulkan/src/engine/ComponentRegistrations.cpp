#include "ComponentRegistry.h"
#include "Components.h"

void register_engine_components() {}

// Field names must match what ComponentBuilder uses in main_runtime.cpp.

REGISTER_COMPONENT(EntityMetaComponent, "EntityMetaComponent",
	FIELD_STRING(name, "Name")
	FIELD_BOOL  (active, "Active")
)

REGISTER_COMPONENT(TransformComponent, "TransformComponent",
	FIELD_VEC3      (position, "Position")
	FIELD_QUAT_EULER(rotation, "Rotation")
	FIELD_VEC3      (scale,    "Scale")
)

REGISTER_COMPONENT(CameraComponent, "CameraComponent",
	FIELD_FLOAT   (fov,       "Fov")
	FIELD_FLOAT   (nearPlane, "Near Plane")
	FIELD_FLOAT   (farPlane,  "Far Plane")
	FIELD_BOOL(controlledByPlayer, "Controlled")
)

REGISTER_COMPONENT(DirectionalLightComponent, "DirectionalLightComponent",
	FIELD_COLOR(color,     "Color")
	FIELD_FLOAT(intensity, "Intensity")
)

REGISTER_COMPONENT(PointLightComponent, "PointLightComponent",
	FIELD_COLOR(color,     "Color")
	FIELD_FLOAT(intensity, "Intensity")
	FIELD_FLOAT(radius,    "Radius")
)
