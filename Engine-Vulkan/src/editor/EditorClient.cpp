#include "EditorClient.h"

#include <QHostAddress>
#include <iostream>
#include <cstring>

static constexpr int RECONNECT_INTERVAL_MS = 500;

EditorClient::EditorClient(QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_reconnectTimer(new QTimer(this))
{
    connect(m_socket, &QTcpSocket::connected,    this, &EditorClient::on_connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &EditorClient::on_disconnected);
    connect(m_socket, &QTcpSocket::readyRead,    this, &EditorClient::on_data_ready);

    m_reconnectTimer->setInterval(RECONNECT_INTERVAL_MS);
    m_reconnectTimer->setSingleShot(false);
    connect(m_reconnectTimer, &QTimer::timeout, this, &EditorClient::try_connect);
}

void EditorClient::connect_to_runtime(const QString& host, uint16_t port)
{
    m_host = host;
    m_port = port;
    m_wantConnected = true;
    m_reconnectTimer->start();
    try_connect();
}

void EditorClient::disconnect_from_runtime()
{
    m_wantConnected = false;
    m_reconnectTimer->stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();
}

bool EditorClient::is_connected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void EditorClient::send_ping()
{
    send(MsgType::Ping);
}

void EditorClient::send_set_viewport(uint64_t hwnd, uint32_t width, uint32_t height)
{
    MsgSetViewport msg{ hwnd, width, height };
    QByteArray payload(reinterpret_cast<const char*>(&msg), sizeof(msg));
    send(MsgType::SetViewport, payload);
}

void EditorClient::send_resize(uint32_t width, uint32_t height)
{
    MsgResize msg{ width, height };
    QByteArray payload(reinterpret_cast<const char*>(&msg), sizeof(msg));
    send(MsgType::Resize, payload);
}

void EditorClient::send_shutdown()
{
    send(MsgType::Shutdown);
}

void EditorClient::send_select_entity(uint32_t entityId)
{
    MsgSelectEntity msg{ entityId };
    QByteArray payload(reinterpret_cast<const char*>(&msg), sizeof(msg));
    send(MsgType::SelectEntity, payload);
}

void EditorClient::send_set_component_field(uint32_t entityId, const QString& componentName,
                                             const QString& fieldName, FieldType fieldType,
                                             const QByteArray& value)
{
    MsgSetComponentField msg{};
    msg.entityId  = entityId;
    msg.fieldType = static_cast<uint32_t>(fieldType);
    strncpy(msg.componentName, componentName.toUtf8().constData(), sizeof(msg.componentName) - 1);
    strncpy(msg.fieldName,     fieldName.toUtf8().constData(),     sizeof(msg.fieldName)     - 1);
    int copyLen = qMin(value.size(), static_cast<qsizetype>(sizeof(msg.value)));
    memcpy(msg.value, value.constData(), copyLen);

    QByteArray payload(reinterpret_cast<const char*>(&msg), sizeof(msg));
    send(MsgType::SetComponentField, payload);
}

void EditorClient::send_set_parent(uint32_t entityId, uint32_t parentId)
{
    MsgSetParent msg{ entityId, parentId };
    QByteArray payload(reinterpret_cast<const char*>(&msg), sizeof(msg));
    send(MsgType::SetParent, payload);
}

void EditorClient::send_rename_entity(uint32_t entityId, const QString& name)
{
    MsgRenameEntity msg{};
    msg.entityId = entityId;
    strncpy(msg.name, name.toUtf8().constData(), sizeof(msg.name) - 1);
    QByteArray payload(reinterpret_cast<const char*>(&msg), sizeof(msg));
    send(MsgType::RenameEntity, payload);
}

void EditorClient::send_create_entity(const QString& name)
{
    MsgCreateEntity msg{};
    strncpy(msg.name, name.toUtf8().constData(), sizeof(msg.name) - 1);
    QByteArray payload(reinterpret_cast<const char*>(&msg), sizeof(msg));
    send(MsgType::CreateEntity, payload);
}

void EditorClient::send(MsgType type, const QByteArray& payload)
{
    if (!is_connected()) return;

    MsgHeader header;
    header.type        = static_cast<uint32_t>(type);
    header.payloadSize = static_cast<uint32_t>(payload.size());

    m_socket->write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!payload.isEmpty())
        m_socket->write(payload);
}

void EditorClient::try_connect()
{
    if (!m_wantConnected) return;
    if (m_socket->state() != QAbstractSocket::UnconnectedState) return;

    m_socket->connectToHost(QHostAddress(m_host), m_port);
}

void EditorClient::on_connected()
{
    m_reconnectTimer->stop();
    std::cout << "[Editor] Connected to runtime\n";
    emit connected();
}

void EditorClient::on_disconnected()
{
    std::cout << "[Editor] Disconnected from runtime\n";
    m_buffer.clear();
    emit disconnected();

    if (m_wantConnected)
        m_reconnectTimer->start(); // will try again in RECONNECT_INTERVAL_MS
}

void EditorClient::on_data_ready()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        if (m_buffer.size() < static_cast<qsizetype>(sizeof(MsgHeader)))
            break;

        MsgHeader header;
        std::memcpy(&header, m_buffer.constData(), sizeof(header));

        qsizetype totalSize = sizeof(MsgHeader) + header.payloadSize;
        if (m_buffer.size() < totalSize)
            break;

        QByteArray payload = m_buffer.mid(sizeof(MsgHeader), header.payloadSize);
        m_buffer.remove(0, static_cast<qsizetype>(totalSize));

        process_message(static_cast<MsgType>(header.type), payload);
    }
}

void EditorClient::process_message(MsgType type, const QByteArray& payload)
{
    switch (type) {
        case MsgType::Pong: {
            emit pong_received();
            break;
        }

        case MsgType::EntityComponents: {
            QDataStream stream(payload);
            stream.setByteOrder(QDataStream::LittleEndian);

            quint32 entityId, componentCount;
            stream >> entityId >> componentCount;

            QVector<IpcComponent> components;
            for (quint32 c = 0; c < componentCount && stream.status() == QDataStream::Ok; c++) {
                IpcComponent comp;
                char typeName[64] = {};
                stream.readRawData(typeName, 64);
                comp.typeName = QString(typeName);

                quint32 fieldCount;
                stream >> fieldCount;

                for (quint32 f = 0; f < fieldCount && stream.status() == QDataStream::Ok; f++) {
                    IpcField field;
                    char fieldName[32] = {};
                    stream.readRawData(fieldName, 32);
                    field.name = QString(fieldName);

                    quint32 fieldType;
                    stream >> fieldType;
                    field.type = static_cast<FieldType>(fieldType);

                    quint32 valueSize = field_type_size(field.type);
                    field.value.resize(static_cast<qsizetype>(valueSize));
                    stream.readRawData(field.value.data(), static_cast<int>(valueSize));

                    comp.fields.append(std::move(field));
                }
                components.append(std::move(comp));
            }

            if (stream.status() == QDataStream::Ok)
                emit entity_components_received(entityId, components);
            break;
        }

        case MsgType::WorldSnapshot: {
            uint32_t count;
            memcpy(&count, payload.constData(), sizeof(count));
            QVector<EntityInfo> entities(count);
            memcpy(entities.data(), payload.constData() + sizeof(count), count * sizeof(EntityInfo));
            emit world_snapshot_received(entities);
            break;
        }

        default: {
            std::cerr << "[Editor] Unhandled message type: "
                    << static_cast<uint32_t>(type) << "\n";
            break;
        }
    }
}
