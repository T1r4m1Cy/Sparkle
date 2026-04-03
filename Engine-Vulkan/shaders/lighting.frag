#version 450

#define MAX_LIGHTS 4

layout(binding = 3) uniform LightUBO {
	vec4 lightPos[MAX_LIGHTS];
	vec4 lightColor[MAX_LIGHTS];
	vec4 viewPos;
	int lightCount;
} light;

layout(location = 0) in vec2 texCoord;

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput gPosition;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput gNormal;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput gAlbedo;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 fragPos = subpassLoad(gPosition).rgb;
	vec3 normal = subpassLoad(gNormal).rgb;
	vec3 albedo = subpassLoad(gAlbedo).rgb;

	float specularStrength = 0.02;
	vec3 ambient = 0.1 * albedo;
	vec3 viewDir = normalize(light.viewPos.xyz - fragPos);

	vec3 result = ambient;

	for(int i = 0; i < light.lightCount; i++)
	{
		vec3 lightDir = normalize(light.lightPos[i].xyz - fragPos);
		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuse = diff * light.lightColor[i].rgb * albedo;

		vec3 reflectDir = reflect(-lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
		vec3 specular = specularStrength * spec * light.lightColor[i].rgb;

		result += diffuse + specular;
	}

	outColor = vec4(result, 1.0);
}