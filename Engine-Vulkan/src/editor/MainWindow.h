#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <QMainWindow>
#include <QModelIndex>
#include <QProcess>
#include <QString>
#include <QTimer>

#include "EditorClient.h"

class QDockWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QScrollArea;
class QVBoxLayout;
class VulkanViewport;

struct SceneNode {
    uint32_t             parentId = UINT32_MAX;
    QString              name;
    std::vector<uint32_t> children;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_runtime_started();
    void on_runtime_finished(int exitCode, QProcess::ExitStatus status);
    void on_runtime_connected();
    void on_runtime_disconnected();
    void on_world_snapshot(QVector<EntityInfo> entities);
    void on_entity_clicked(QTreeWidgetItem* item, int column);
    void on_entity_components(uint32_t entityId, QVector<IpcComponent> components);
    void on_create_entity_clicked();
    void on_entity_name_changed(QTreeWidgetItem* item, int column);
    void on_entity_moved();

private:
    void setup_layout();
    void launch_runtime();

    void merge_snapshot_into_graph(const QVector<EntityInfo>& entities);
    void rebuild_tree_from_graph();
    void rebuild_tree_item(QTreeWidgetItem* parent, uint32_t entityId);
    void sync_graph_from_tree();
    void sync_graph_item(QTreeWidgetItem* item, uint32_t parentId);

    QProcess*      m_runtimeProcess = nullptr;
    EditorClient*  m_client         = nullptr;
    QTimer*        m_resizeDebounce = nullptr;

    QDockWidget*    m_inspectorDock;
    QDockWidget*    m_viewportDock;
    VulkanViewport* m_viewport;
    QDockWidget*    m_propertiesDock;

    QTreeWidget*  m_entityList;
    QScrollArea*  m_propertiesScroll;
    QVBoxLayout*  m_propertiesLayout;

    std::unordered_map<uint32_t, SceneNode> m_sceneGraph;
    std::vector<uint32_t>                   m_rootOrder;
    bool                                    m_rebuildingTree = false;
    uint32_t                                m_selectedEntityId = UINT32_MAX;
};
