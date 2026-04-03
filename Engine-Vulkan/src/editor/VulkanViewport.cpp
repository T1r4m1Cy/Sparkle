#include <windows.h>

#include "VulkanViewport.h"

VulkanViewport::VulkanViewport(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
}

HWND VulkanViewport::get_hwnd() const
{
    return reinterpret_cast<HWND>(winId());
}