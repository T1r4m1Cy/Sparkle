#pragma once

#include <QWidget>

class VulkanViewport : public QWidget
{
    Q_OBJECT

public:
    explicit VulkanViewport(QWidget* parent = nullptr);

    HWND get_hwnd() const;
};