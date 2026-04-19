#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include <QVector>
#include <QString>

#include "IPCProtocol.h"

struct IpcField {
    QString   name;
    FieldType type;
    QByteArray value;  // raw bytes, size = field_type_size(type)
};

struct IpcComponent {
    QString          typeName;
    QVector<IpcField> fields;
};

// TCP client running inside SparkleEditor.
// Connects to SparkleRuntime's server.
// Automatically retries connection until runtime is ready.
class EditorClient : public QObject {
    Q_OBJECT
public:
    explicit EditorClient(QObject* parent = nullptr);

    void connect_to_runtime(const QString& host = "127.0.0.1", uint16_t port = 57300);
    void disconnect_from_runtime();

    bool is_connected() const;

    void send_ping();
    void send_set_viewport(uint64_t hwnd, uint32_t width, uint32_t height);
    void send_resize(uint32_t width, uint32_t height);
    void send_shutdown();
    void send_select_entity(uint32_t entityId);
    void send_set_component_field(uint32_t entityId, const QString& componentName,
                                  const QString& fieldName, FieldType fieldType,
                                  const QByteArray& value);
    void send_create_entity(const QString& name);
    void send_rename_entity(uint32_t entityId, const QString& name);
    void send_set_parent(uint32_t entityId, uint32_t parentId);

signals:
    void connected();
    void disconnected();
    void pong_received();
    void world_snapshot_received(QVector<EntityInfo> entities);
    void entity_components_received(uint32_t entityId, QVector<IpcComponent> components);

private slots:
    void on_connected();
    void on_disconnected();
    void on_data_ready();
    void try_connect();

private:
    void send(MsgType type, const QByteArray& payload = {});
    void process_message(MsgType type, const QByteArray& payload);

    QTcpSocket* m_socket;
    QTimer*     m_reconnectTimer;
    QByteArray  m_buffer;

    QString  m_host;
    uint16_t m_port = 57300;
    bool     m_wantConnected = false; // false = don't auto-reconnect
};
