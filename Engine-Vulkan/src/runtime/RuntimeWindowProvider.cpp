#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "RuntimeWindowProvider.h"
#include "VulkanContext.h"

static constexpr wchar_t CLASS_NAME[] = L"SparkleRuntimeWindow";

RuntimeWindowProvider::RuntimeWindowProvider(uint32_t w, uint32_t h)
    : width(w), height(h)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = DefWindowProcW;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    // WS_POPUP + no WS_VISIBLE = a real but invisible window
    m_ownHwnd = CreateWindowExW(
        0, CLASS_NAME, L"Sparkle Runtime",
        WS_POPUP,
        0, 0, static_cast<int>(width), static_cast<int>(height),
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr
    );
    hwnd = m_ownHwnd;
}

RuntimeWindowProvider::~RuntimeWindowProvider()
{
    if (m_ownHwnd) {
        DestroyWindow(m_ownHwnd);
        m_ownHwnd = nullptr;
        hwnd = nullptr;
    }
    UnregisterClassW(CLASS_NAME, GetModuleHandleW(nullptr));
}

void RuntimeWindowProvider::use_external_hwnd(HWND externalHwnd, uint32_t w, uint32_t h)
{
    hwnd     = externalHwnd;
    width    = w;
    height   = h;
    m_usingOwn = false;
}

void RuntimeWindowProvider::use_own_hwnd()
{
    hwnd     = m_ownHwnd;
    m_usingOwn = true;
}

std::vector<const char*> RuntimeWindowProvider::get_required_extensions()
{
    return { "VK_KHR_surface", "VK_KHR_win32_surface" };
}

vk::SurfaceKHR RuntimeWindowProvider::create_surface(vk::Instance instance)
{
    VkWin32SurfaceCreateInfoKHR info{};
    info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd      = hwnd;
    info.hinstance = GetModuleHandleW(nullptr);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    vkCreateWin32SurfaceKHR(static_cast<VkInstance>(instance), &info, nullptr, &surface);
    return vk::SurfaceKHR(surface);
}

vk::Extent2D RuntimeWindowProvider::get_window_size()
{
    return { width, height };
}

vk::Framebuffer RuntimeWindowProvider::get_framebuffer(uint32_t imageIndex, uint32_t /*frame*/)
{
    return vulkan->swapchain.swapchainFramebuffers[imageIndex];
}
