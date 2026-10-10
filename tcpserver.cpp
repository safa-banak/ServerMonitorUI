#include "tcpserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>

TcpServer::TcpServer(QObject *parent) : QObject(parent)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(quint16 port)
{
    if (server->isListening())
        return true;
    // for all of IP protocol
    //bool ok = server->listen(QHostAddress::Any, port);
    // for just IPv4 protocol
    bool ok = server->listen(QHostAddress::AnyIPv4, port);
    if (ok) {
        emit logMessage(QString("Server listening on port %1").arg(port));
    } else {
        emit logMessage("Failed to start server: " + server->errorString());
    }
    return ok;
}

void TcpServer::stop()
{
    for (auto *client : socketToName.keys()) {
        client->abort();
        client->deleteLater();
    }
    socketToName.clear();
    server->close();
    emit logMessage("Server stopped.");
}

bool TcpServer::isListening() const
{
    return server->isListening();
}

void TcpServer::onNewConnection()
{
    QTcpSocket *client = server->nextPendingConnection();
    QString ip = client->peerAddress().toString();
    emit logMessage("New connection from " + ip);

    connect(client, &QTcpSocket::readyRead, this, [this, client](){
        handleReadyRead(client);
    });
    connect(client, &QTcpSocket::disconnected, this, [this, client](){
        handleClientDisconnected(client);
    });
}

void TcpServer::handleReadyRead(QTcpSocket *client)
{
     // چون ممکنه چند پیام پشت سر هم برسه، با \n جدا می‌کنیم
    while (client->canReadLine()) {
        QByteArray line = client->readLine().trimmed();
        if (line.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) {
            emit logMessage("Invalid JSON from " + client->peerAddress().toString());
            continue;
        }

        QJsonObject obj = doc.object();
        QString name = obj["agent"].toString();
        int cpu = obj["cpu"].toInt();
        int ram = obj["ram"].toInt();
        int ramTotal = obj["ramTotal"].toInt();
        int disk = obj["disk"].toInt();
        int diskTotal = obj["diskTotal"].toInt();

        // اولین پیام: ثبت نام Agent
        if (!socketToName.contains(client)) {
            socketToName[client] = name;
            emit agentConnected(name, client->peerAddress().toString());
        }

        emit metricsReceived(name, cpu, ram, ramTotal, disk, diskTotal);
    }
}

void TcpServer::handleClientDisconnected(QTcpSocket *client)
{
    QString name = socketToName.value(client, "unknown");
    socketToName.remove(client);
    emit agentDisconnected(name);
    emit logMessage("Agent disconnected: " + name);

    client->deleteLater();
}