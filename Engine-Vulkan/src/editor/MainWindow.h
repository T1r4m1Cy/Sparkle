#pragma once

#include <QMainWindow>
#include <QTimer>

#include "Engine.h"

class QDockWidget;
class QTreeWidget;
class QLabel;
class VulkanViewport;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_render_frame();

private:
    void setup_layout();

    Engine m_engine;
    QTimer* m_renderTimer;

    QDockWidget* m_inspectorDock;
    QDockWidget* m_viewportDock;
    VulkanViewport* m_viewport;
    QDockWidget* m_propertiesDock;

    QTreeWidget* m_entityList;
    QLabel* m_propertiesLabel;
};
