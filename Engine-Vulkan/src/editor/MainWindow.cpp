#include <algorithm>
#include <unordered_set>

#include <QAbstractItemModel>
#include <QApplication>
#include <QDockWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCloseEvent>
#include <QDir>
#include <QStatusBar>
#include <iostream>

#include "MainWindow.h"
#include "VulkanViewport.h"
#include "EditorClient.h"
#include "PropertyDrawers.h"

static constexpr uint16_t RUNTIME_PORT    = 57300;
static constexpr int      SHUTDOWN_TIMEOUT_MS = 3000;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Sparkle Editor");
    resize(1280, 720);
    setDockNestingEnabled(true);

    setup_layout();
    statusBar()->showMessage("Starting runtime...");

    m_client = new EditorClient(this);
    connect(m_client, &EditorClient::connected,    this, &MainWindow::on_runtime_connected);
    connect(m_client, &EditorClient::disconnected, this, &MainWindow::on_runtime_disconnected);

    // Debounce resize: swapchain recreation is expensive, send only after user stops dragging.
    m_resizeDebounce = new QTimer(this);
    m_resizeDebounce->setSingleShot(true);
    m_resizeDebounce->setInterval(150);
    connect(m_resizeDebounce, &QTimer::timeout, [this]() {
        if (m_client->is_connected() && m_viewport)
            m_client->send_resize(static_cast<uint32_t>(m_viewport->width()),
                                  static_cast<uint32_t>(m_viewport->height()));
    });

    connect(m_viewport, &VulkanViewport::viewport_resized, [this](int, int) {
        m_resizeDebounce->start(); // restarts the timer if already running
    });

    launch_runtime();
}

MainWindow::~MainWindow()
{
    // closeEvent handles graceful shutdown; destructor is just a safety net.
    if (m_runtimeProcess && m_runtimeProcess->state() != QProcess::NotRunning) {
        m_runtimeProcess->kill();
        m_runtimeProcess->waitForFinished(1000);
    }
}

void MainWindow::launch_runtime()
{
    // Look for SparkleRuntime.exe next to SparkleEditor.exe
    QString runtimePath = QDir(QApplication::applicationDirPath())
                              .filePath("SparkleRuntime.exe");

    m_runtimeProcess = new QProcess(this);
    m_runtimeProcess->setProcessChannelMode(QProcess::MergedChannels);

    // Forward runtime stdout/stderr to our own stdout so it appears in the IDE console
    connect(m_runtimeProcess, &QProcess::readyRead, [this]() {
        std::cout << m_runtimeProcess->readAll().toStdString();
        std::cout.flush();
    });

    connect(m_runtimeProcess, &QProcess::started,
            this, &MainWindow::on_runtime_started);
    connect(m_runtimeProcess, &QProcess::finished,
            this, &MainWindow::on_runtime_finished);
    connect(m_client, &EditorClient::world_snapshot_received,
            this, &MainWindow::on_world_snapshot);
    connect(m_client, &EditorClient::entity_components_received,
            this, &MainWindow::on_entity_components);
    connect(m_entityList, &QTreeWidget::itemClicked,
            this, &MainWindow::on_entity_clicked);
    connect(m_entityList, &QTreeWidget::itemChanged,
            this, &MainWindow::on_entity_name_changed);
    connect(m_entityList->model(), &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex&, int, int, const QModelIndex&, int) { on_entity_moved(); });
    connect(m_entityList->model(), &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex&, int, int) { on_entity_moved(); });

    std::cout << "[Editor] Launching " << runtimePath.toStdString() << "\n";
    m_runtimeProcess->start(runtimePath, {});

    if (!m_runtimeProcess->waitForStarted(3000)) {
        std::cerr << "[Editor] Failed to start SparkleRuntime.exe\n";
        statusBar()->showMessage("ERROR: could not start SparkleRuntime.exe");
        return;
    }

    // Start trying to connect — runtime may take a moment to bind its port
    m_client->connect_to_runtime("127.0.0.1", RUNTIME_PORT);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_client->is_connected()) {
        m_client->send_shutdown();
        m_client->disconnect_from_runtime();
    }

    if (m_runtimeProcess && m_runtimeProcess->state() != QProcess::NotRunning) {
        if (!m_runtimeProcess->waitForFinished(SHUTDOWN_TIMEOUT_MS)) {
            std::cerr << "[Editor] Runtime did not exit gracefully, killing\n";
            m_runtimeProcess->kill();
            m_runtimeProcess->waitForFinished(1000);
        }
    }

    event->accept();
}


void MainWindow::on_runtime_started()
{
    std::cout << "[Editor] Runtime process started (PID "
              << m_runtimeProcess->processId() << ")\n";
}

void MainWindow::on_runtime_finished(int exitCode, QProcess::ExitStatus status)
{
    std::cout << "[Editor] Runtime finished with code " << exitCode << "\n";
    statusBar()->showMessage(
        status == QProcess::NormalExit
            ? QString("Runtime exited (code %1)").arg(exitCode)
            : "Runtime crashed"
    );
}

void MainWindow::on_runtime_connected()
{
    setWindowTitle("Sparkle Editor — Runtime connected");
    statusBar()->showMessage("Runtime connected");

    // Send the Qt viewport's native HWND so the runtime renders directly into it.
    // The runtime will destroy its hidden window surface and create a new one from this HWND.
    if (m_viewport) {
        m_client->send_set_viewport(
            reinterpret_cast<uint64_t>(m_viewport->get_hwnd()),
            static_cast<uint32_t>(m_viewport->width()),
            static_cast<uint32_t>(m_viewport->height())
        );
    }
}

void MainWindow::on_runtime_disconnected()
{
    setWindowTitle("Sparkle Editor — Runtime disconnected");
    statusBar()->showMessage("Runtime disconnected — reconnecting...");
}

void MainWindow::on_world_snapshot(QVector<EntityInfo> entities) {
    merge_snapshot_into_graph(entities);
    rebuild_tree_from_graph();
    if (m_selectedEntityId != UINT32_MAX && m_client->is_connected())
        m_client->send_select_entity(m_selectedEntityId);
}

void MainWindow::on_entity_clicked(QTreeWidgetItem* item, int) {
    if (!m_client->is_connected()) return;
    m_selectedEntityId = item->data(0, Qt::UserRole).toUInt();
    m_client->send_select_entity(m_selectedEntityId);
}

void MainWindow::on_entity_components(uint32_t entityId, QVector<IpcComponent> components) {
    while (QLayoutItem* child = m_propertiesLayout->takeAt(0)) {
        if (QWidget* w = child->widget()) {
            w->setParent(nullptr);
            w->deleteLater();
        }
        delete child;
    }

    for (auto& comp : components) {
        auto* group = new QGroupBox(comp.typeName);
        auto* form  = new QFormLayout(group);
        form->setContentsMargins(6, 6, 6, 6);

        for (auto& field : comp.fields) {
            QWidget* widget = create_field_widget(field.type, field.value);
            connect_field_changes(widget, field.type,
                [this, entityId, typeName = comp.typeName,
                 fieldName = field.name, fieldType = field.type](QByteArray bytes) {
                    m_client->send_set_component_field(
                        entityId, typeName, fieldName, fieldType, bytes);
                });
            form->addRow(field.name, widget);
        }

        m_propertiesLayout->addWidget(group);
    }

    m_propertiesLayout->addStretch();
}

void MainWindow::setup_layout()
{
    // Inspector
    m_inspectorDock = new QDockWidget("Inspector", this);
    auto* inspectorContainer = new QWidget();
    auto* inspectorLayout = new QVBoxLayout(inspectorContainer);
    inspectorLayout->setContentsMargins(0, 0, 0, 0);
    inspectorLayout->setSpacing(2);

    auto* createEntityBtn = new QPushButton("+ Create Entity");
    inspectorLayout->addWidget(createEntityBtn);

    m_entityList = new QTreeWidget();
    m_entityList->setHeaderHidden(true);
    m_entityList->setDragEnabled(true);
    m_entityList->setAcceptDrops(true);
    m_entityList->setDragDropMode(QAbstractItemView::InternalMove);
    m_entityList->setDefaultDropAction(Qt::MoveAction);
    inspectorLayout->addWidget(m_entityList);

    m_inspectorDock->setWidget(inspectorContainer);

    connect(createEntityBtn, &QPushButton::clicked, this, &MainWindow::on_create_entity_clicked);

    // Viewport
    m_viewport = new VulkanViewport();
    m_viewportDock = new QDockWidget("Viewport", this);
    m_viewportDock->setWidget(m_viewport);

    // Properties
    m_propertiesDock = new QDockWidget("Properties", this);
    auto* propertiesContainer = new QWidget();
    m_propertiesLayout = new QVBoxLayout(propertiesContainer);
    m_propertiesLayout->setAlignment(Qt::AlignTop);
    m_propertiesLayout->setContentsMargins(4, 4, 4, 4);
    m_propertiesScroll = new QScrollArea();
    m_propertiesScroll->setWidget(propertiesContainer);
    m_propertiesScroll->setWidgetResizable(true);
    m_propertiesDock->setWidget(m_propertiesScroll);

    // Layout
    addDockWidget(Qt::LeftDockWidgetArea, m_inspectorDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_viewportDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);

    splitDockWidget(m_inspectorDock, m_viewportDock, Qt::Horizontal);
    splitDockWidget(m_viewportDock, m_propertiesDock, Qt::Horizontal);

    resizeDocks(
        { m_inspectorDock, m_viewportDock, m_propertiesDock },
        { 240, 720, 320 },
        Qt::Horizontal
    );
}

void MainWindow::on_entity_name_changed(QTreeWidgetItem* item, int column) {
    if (column != 0 || !m_client->is_connected()) return;
    uint32_t id = item->data(0, Qt::UserRole).toUInt();
    m_client->send_rename_entity(id, item->text(0));
}

void MainWindow::on_create_entity_clicked() {
    if (!m_client->is_connected()) return;
    m_client->send_create_entity("New Entity");
}

void MainWindow::merge_snapshot_into_graph(const QVector<EntityInfo>& entities) {
    // Build set of incoming IDs
    std::unordered_set<uint32_t> incoming;
    for (auto& e : entities) incoming.insert(e.id);

    // Remove deleted entities
    for (auto it = m_sceneGraph.begin(); it != m_sceneGraph.end(); ) {
        if (!incoming.count(it->first)) {
            uint32_t id = it->first;
            uint32_t pid = it->second.parentId;
            if (pid == UINT32_MAX) {
                m_rootOrder.erase(std::remove(m_rootOrder.begin(), m_rootOrder.end(), id), m_rootOrder.end());
            } else {
                auto pit = m_sceneGraph.find(pid);
                if (pit != m_sceneGraph.end()) {
                    auto& ch = pit->second.children;
                    ch.erase(std::remove(ch.begin(), ch.end(), id), ch.end());
                }
            }
            it = m_sceneGraph.erase(it);
        } else {
            ++it;
        }
    }

    // Add new entities, update names for existing
    for (auto& e : entities) {
        auto it = m_sceneGraph.find(e.id);
        if (it == m_sceneGraph.end()) {
            SceneNode node;
            node.name     = QString(e.name);
            node.parentId = e.parentId;

            m_sceneGraph[e.id] = node;

            auto parentIt = m_sceneGraph.find(e.parentId);
            if (e.parentId == UINT32_MAX || parentIt == m_sceneGraph.end()) {
                m_rootOrder.push_back(e.id);
            } else {
                parentIt->second.children.push_back(e.id);
            }
        } else {
            it->second.name = QString(e.name);
        }
    }
}

void MainWindow::rebuild_tree_from_graph() {
    m_rebuildingTree = true;
    QSignalBlocker blocker(m_entityList);
    m_entityList->clear();
    for (uint32_t rootId : m_rootOrder)
        rebuild_tree_item(nullptr, rootId);
    m_rebuildingTree = false;
    m_entityList->expandAll();
}

void MainWindow::rebuild_tree_item(QTreeWidgetItem* parent, uint32_t entityId) {
    auto it = m_sceneGraph.find(entityId);
    if (it == m_sceneGraph.end()) return;

    auto* item = parent ? new QTreeWidgetItem(parent)
                        : new QTreeWidgetItem(m_entityList);
    item->setText(0, it->second.name);
    item->setData(0, Qt::UserRole, entityId);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    for (uint32_t childId : it->second.children)
        rebuild_tree_item(item, childId);
}

void MainWindow::on_entity_moved() {
    if (m_rebuildingTree) return;
    sync_graph_from_tree();
}

void MainWindow::sync_graph_from_tree() {
    m_rootOrder.clear();
    for (auto& [id, node] : m_sceneGraph)
        node.children.clear();

    for (int i = 0; i < m_entityList->topLevelItemCount(); i++)
        sync_graph_item(m_entityList->topLevelItem(i), UINT32_MAX);
}

void MainWindow::sync_graph_item(QTreeWidgetItem* item, uint32_t parentId) {
    uint32_t id = item->data(0, Qt::UserRole).toUInt();

    auto it = m_sceneGraph.find(id);
    if (it == m_sceneGraph.end()) return;

    uint32_t oldParent = it->second.parentId;
    it->second.parentId = parentId;

    if (parentId == UINT32_MAX)
        m_rootOrder.push_back(id);

    for (int i = 0; i < item->childCount(); i++) {
        auto* child = item->child(i);
        uint32_t childId = child->data(0, Qt::UserRole).toUInt();
        it->second.children.push_back(childId);
        sync_graph_item(child, id);
    }

    if (oldParent != parentId && m_client->is_connected())
        m_client->send_set_parent(id, parentId);
}