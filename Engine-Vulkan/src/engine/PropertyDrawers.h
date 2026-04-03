#pragma once

#include <string>
#include <functional>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "libraries/imgui/imgui.h"

#include "AssetHandle.h"

inline void draw_field(const std::string& label, float* data)
{
	ImGui::DragFloat(label.c_str(), data, 0.1f);
}

inline void draw_field(const std::string& label, int* data)
{
	ImGui::InputInt(label.c_str(), data);
}

inline void draw_field(const std::string& label, uint32_t* data)
{
	int temp = static_cast<int>(*data);
	if (ImGui::InputInt(label.c_str(), &temp)) {
		if (temp >= 0) {
			*data = static_cast<uint32_t>(temp);
		}
	}
}

inline void draw_field(const std::string& label, bool* data)
{
	ImGui::Checkbox(label.c_str(), data);
}

inline void draw_field(const std::string& label, glm::vec3* data)
{
	ImGui::DragFloat3(label.c_str(), &data->x, 0.1f);
}

inline void draw_field(const std::string& label, glm::quat* data)
{
	glm::vec3 euler = glm::eulerAngles(*data);
	ImGui::DragFloat3(label.c_str(), &euler.x, 0.1f);
	*data = glm::quat(euler);
}

inline void draw_field(const std::string& label, std::string* data)
{
	char buf[256];
	strncpy_s(buf, data->c_str(), sizeof(buf));
	if (ImGui::InputText(label.c_str(), buf, sizeof(buf))) {
		*data = buf;
	}
}

inline void draw_field(const std::string& label, MeshHandle* data)
{
	ImGui::Text("%s: id=%d v=%d", label.c_str(), data->id, data->version);
}

inline void draw_field(const std::string& label, TextureHandle* data)
{
	ImGui::Text("%s: id=%d v=%d", label.c_str(), data->id, data->version);
}

inline void draw_field_color(const std::string& label, glm::vec3* data)
{
	ImGui::ColorEdit3(label.c_str(), &data->x);
}