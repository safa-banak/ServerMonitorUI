#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QTcpSocket>

class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    ~TcpServer();

    bool start(quint16 port);
    void stop();
    bool isListening() const;

signals:
    void agentConnected(const QString &name, const QString &ip);
    void agentDisconnected(const QString &name);
    void metricsReceived(const QString &name, int cpu, int ram);
    void logMessage(const QString &message);

private slots:
    void onNewConnection();

private:
    void handleReadyRead(QTcpSocket *client);
    void handleClientDisconnected(QTcpSocket *client);

    QTcpServer *server;
    QMap<QTcpSocket*, QString> socketToName;
};

#endif // TCPSERVER_H
