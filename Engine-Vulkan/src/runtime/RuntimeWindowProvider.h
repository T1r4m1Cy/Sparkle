#pragma once

#include "IWindowProvider.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct VulkanContext;

// Creates a hidden Win32 window to obtain a VkSurfaceKHR.
// The window is never shown; it exists solely for the Vulkan surface.
// Later replaced by offscreen rendering + Vulkan External Memory.
struct RuntimeWindowProvider : IWindowProvider {
    HWND hwnd = nullptr;          // active HWND used for surface creation
    VulkanContext* vulkan = nullptr;
    uint32_t width;
    uint32_t height;

    RuntimeWindowProvider(uint32_t w, uint32_t h);
    ~RuntimeWindowProvider() override;

    // Switch to an external HWND (e.g. the editor's Qt viewport).
    // Call this before recreating the Vulkan surface.
    void use_external_hwnd(HWND externalHwnd, uint32_t w, uint32_t h);

    // Restore to the hidden window owned by this provider.
    void use_own_hwnd();

    std::vector<const char*> get_required_extensions() override;
    vk::SurfaceKHR create_surface(vk::Instance instance) override;
    vk::Extent2D get_window_size() override;
    vk::Framebuffer get_framebuffer(uint32_t imageIndex, uint32_t frame) override;

private:
    HWND m_ownHwnd    = nullptr; // hidden window we created
    bool m_usingOwn   = true;
};
