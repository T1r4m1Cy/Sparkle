#include "ComponentRegistry.h"
#include "Components.h"

REGISTER_COMPONENT(NameComponent, "Name",
	FIELD(std::string, name, "Name")
)

REGISTER_COMPONENT(TransformComponent, "Transform",
	FIELD(glm::vec3, position, "Position")
	FIELD(glm::quat, rotation, "Rotation")
	FIELD(glm::vec3, scale, "Scale")
)

REGISTER_COMPONENT(MeshComponent, "Mesh",
	FIELD(MeshHandle, mesh, "Mesh")
	FIELD(TextureHandle, texture, "Texture")
)

REGISTER_COMPONENT(PointLightComponent, "Point Light",
	FIELD_COLOR(glm::vec3, color, "Color")
	FIELD(float, intensity, "Intensity")
	FIELD(float, radius, "Radius")
)

REGISTER_COMPONENT(CameraComponent, "Camera",
	FIELD(float, fov, "FOV")
	FIELD(float, nearPlane, "Near Plane")
	FIELD(float, farPlane, "Far Plane")
	FIELD(bool, controlledByPlayer, "Controlled By Player")
)