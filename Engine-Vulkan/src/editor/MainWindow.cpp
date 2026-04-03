#include <QDockWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QCloseEvent>

#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
//#include <vulkan/vulkan.hpp>
#include "VulkanContext.h"
#include "VulkanViewport.h"

#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Sparkle Editor");
    resize(1280, 720);
    setDockNestingEnabled(true);

    m_viewport = new VulkanViewport();

    m_engine.requiredExtensions = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    m_engine.createSurface = [this](vk::Instance instance) -> vk::SurfaceKHR {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = m_viewport->get_hwnd();
        createInfo.hinstance = GetModuleHandle(nullptr);

        VkSurfaceKHR surface;
        vkCreateWin32SurfaceKHR(
            static_cast<VkInstance>(instance),
            &createInfo, nullptr, &surface);
        return vk::SurfaceKHR(surface);
    };

    m_engine.getWindowSize = [this]() -> vk::Extent2D {
        return {
            static_cast<uint32_t>(m_viewport->width()),
            static_cast<uint32_t>(m_viewport->height())
        };
    };

    m_engine.getFramebuffer = [&](uint32_t imageIndex, uint32_t frame) {
        return m_engine.vulkan.swapchain.swapchainFramebuffers[imageIndex];
    };

    setup_layout();

    show();

    engine_init(m_engine, 1280, 720);

    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::on_render_frame);
    m_renderTimer->start(0);
}

MainWindow::~MainWindow()
{
    m_renderTimer->stop();
    engine_shutdown(m_engine);
}

void MainWindow::on_render_frame()
{
    bool stillRunning = true;
    engine_tick(m_engine);
    engine_render(m_engine);
    if (!stillRunning) close();
}

void MainWindow::setup_layout()
{
    //Inspector
    m_inspectorDock = new QDockWidget("Inspector", this);
    m_entityList = new QTreeWidget();
    m_entityList->setHeaderHidden(true);
    m_inspectorDock->setWidget(m_entityList);

    //Viewport
    m_viewport = new VulkanViewport();
    m_viewportDock = new QDockWidget("Viewport", this);
    m_viewportDock->setWidget(m_viewport);

    //Properties
    m_propertiesDock = new QDockWidget("Properties", this);
    m_propertiesLabel = new QLabel("Properties", m_propertiesDock);
    m_propertiesLabel->setAlignment(Qt::AlignCenter);
    m_propertiesDock->setWidget(m_propertiesLabel);

    //Layout
    addDockWidget(Qt::LeftDockWidgetArea, m_inspectorDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_viewportDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);

    splitDockWidget(m_inspectorDock, m_viewportDock, Qt::Horizontal);
    splitDockWidget(m_viewportDock, m_propertiesDock, Qt::Horizontal);

    resizeDocks(
        {m_inspectorDock, m_viewportDock, m_propertiesDock},
        {250, 780, 250},
        Qt::Horizontal
    );
}