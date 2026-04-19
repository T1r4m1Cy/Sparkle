#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>

#include "IPCProtocol.h"

// TCP server running inside SparkleRuntime.
// Accepts one connection from SparkleEditor.
// Messages are framed: MsgHeader (8 bytes) + payload.
class RuntimeServer : public QObject {
    Q_OBJECT
public:
    explicit RuntimeServer(uint16_t port, QObject* parent = nullptr);

    bool isConnected() const { return m_client != nullptr; }

    void send(MsgType type, const QByteArray& payload = {});

signals:
    void editorConnected();
    void shutdownRequested();
    void resizeRequested(quint32 width, quint32 height);
    void viewportChanged(quint64 hwnd, quint32 width, quint32 height);
    void entitySelected(quint32 entityId);
    void fieldChanged(quint32 entityId, QString componentName, QString fieldName,
                      quint32 fieldType, QByteArray value);
    void entityCreateRequested(QString name);
    void entityRenameRequested(quint32 entityId, QString name);
    void parentChangeRequested(quint32 entityId, quint32 parentId);

private slots:
    void on_new_connection();
    void on_data_ready();
    void on_disconnected();

private:
    void process_message(MsgType type, const QByteArray& payload);

    QTcpServer* m_server;
    QTcpSocket* m_client = nullptr;
    QByteArray  m_buffer; // accumulates incoming bytes until a full message arrives
};
