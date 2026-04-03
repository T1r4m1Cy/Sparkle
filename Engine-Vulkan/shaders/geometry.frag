#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;

void main() {
	vec4 albedoColor = texture(texSampler, fragTexCoord);
	gPosition = vec4(fragPos, 1.0);
	gNormal = vec4(normalize(fragNormal), 1.0);
	gAlbedo = albedoColor;
}