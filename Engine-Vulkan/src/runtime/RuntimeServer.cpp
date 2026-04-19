#include "RuntimeServer.h"

#include <QHostAddress>
#include <iostream>
#include <cstring>

RuntimeServer::RuntimeServer(uint16_t port, QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &RuntimeServer::on_new_connection);

    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        std::cerr << "[Runtime] Failed to start TCP server on port " << port << "\n";
        return;
    }
    std::cout << "[Runtime] Listening on 127.0.0.1:" << port << "\n";
}

void RuntimeServer::send(MsgType type, const QByteArray& payload)
{
    if (!m_client) return;

    MsgHeader header;
    header.type        = static_cast<uint32_t>(type);
    header.payloadSize = static_cast<uint32_t>(payload.size());

    m_client->write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!payload.isEmpty())
        m_client->write(payload);
}

void RuntimeServer::on_new_connection()
{
    QTcpSocket* incoming = m_server->nextPendingConnection();
    if (m_client) {
        // Only one editor at a time — reject
        incoming->disconnectFromHost();
        incoming->deleteLater();
        return;
    }

    m_client = incoming;
    connect(m_client, &QTcpSocket::readyRead,    this, &RuntimeServer::on_data_ready);
    connect(m_client, &QTcpSocket::disconnected, this, &RuntimeServer::on_disconnected);

    std::cout << "[Runtime] Editor connected from "
              << m_client->peerAddress().toString().toStdString() << "\n";

    emit editorConnected();
}

void RuntimeServer::on_data_ready()
{
    m_buffer.append(m_client->readAll());

    // Parse as many complete messages as available
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

void RuntimeServer::on_disconnected()
{
    std::cout << "[Runtime] Editor disconnected\n";
    m_client->deleteLater();
    m_client = nullptr;
    m_buffer.clear();
}

void RuntimeServer::process_message(MsgType type, const QByteArray& payload)
{
    switch (type) {
    case MsgType::Ping:
        send(MsgType::Pong);
        break;

    case MsgType::Resize:
        if (payload.size() >= static_cast<qsizetype>(sizeof(MsgResize))) {
            MsgResize msg;
            std::memcpy(&msg, payload.constData(), sizeof(msg));
            emit resizeRequested(static_cast<quint32>(msg.width),
                                 static_cast<quint32>(msg.height));
        }
        break;

    case MsgType::SetViewport:
        if (payload.size() >= static_cast<qsizetype>(sizeof(MsgSetViewport))) {
            MsgSetViewport msg;
            std::memcpy(&msg, payload.constData(), sizeof(msg));
            emit viewportChanged(static_cast<quint64>(msg.hwnd),
                                 static_cast<quint32>(msg.width),
                                 static_cast<quint32>(msg.height));
        }
        break;

    case MsgType::SelectEntity:
        if (payload.size() >= static_cast<qsizetype>(sizeof(MsgSelectEntity))) {
            MsgSelectEntity msg;
            std::memcpy(&msg, payload.constData(), sizeof(msg));
            emit entitySelected(static_cast<quint32>(msg.entityId));
        }
        break;

    case MsgType::SetComponentField:
        if (payload.size() >= static_cast<qsizetype>(sizeof(MsgSetComponentField))) {
            MsgSetComponentField msg;
            std::memcpy(&msg, payload.constData(), sizeof(msg));
            QByteArray valueBytes(reinterpret_cast<const char*>(msg.value), sizeof(msg.value));
            emit fieldChanged(msg.entityId,
                              QString(msg.componentName),
                              QString(msg.fieldName),
                              msg.fieldType,
                              valueBytes);
        }
        break;

    case MsgType::SetParent:
        if (payload.size() >= static_cast<qsizetype>(sizeof(MsgSetParent))) {
            MsgSetParent msg{};
            std::memcpy(&msg, payload.constData(), sizeof(msg));
            emit parentChangeRequested(msg.entityId, msg.parentId);
        }
        break;

    case MsgType::RenameEntity:
        if (payload.size() >= static_cast<qsizetype>(sizeof(MsgRenameEntity))) {
            MsgRenameEntity msg{};
            std::memcpy(&msg, payload.constData(), sizeof(msg));
            msg.name[sizeof(msg.name) - 1] = '\0';
            emit entityRenameRequested(msg.entityId, QString(msg.name));
        }
        break;

    case MsgType::CreateEntity:
        if (payload.size() >= static_cast<qsizetype>(sizeof(MsgCreateEntity))) {
            MsgCreateEntity msg{};
            std::memcpy(&msg, payload.constData(), sizeof(msg));
            msg.name[sizeof(msg.name) - 1] = '\0';
            emit entityCreateRequested(QString(msg.name));
        }
        break;

    case MsgType::Shutdown:
        emit shutdownRequested();
        break;

    default:
        std::cerr << "[Runtime] Unknown message type: " << static_cast<uint32_t>(type) << "\n";
        break;
    }
}
