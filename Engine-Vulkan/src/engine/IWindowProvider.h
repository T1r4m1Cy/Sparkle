#pragma once

#include <vector>
#include <cstdint>
#include <vulkan/vulkan.hpp>

struct IWindowProvider {
    virtual ~IWindowProvider() = default;
    virtual std::vector<const char*> get_required_extensions() = 0;
    virtual vk::SurfaceKHR create_surface(vk::Instance instance) = 0;
    virtual vk::Extent2D get_window_size() = 0;
    virtual vk::Framebuffer get_framebuffer(uint32_t imageIndex, uint32_t frame) = 0;
};