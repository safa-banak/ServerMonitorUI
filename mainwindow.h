#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "tcpserver.h"
#include <QMainWindow>
#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QColor>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addServer(const QString &name, const QString &ip);
    void onMetricsReceived(const QString &name, int cpu, int ram);
    void onAgentDisconnected(const QString &name);
    void saveServers();
    void loadServers();
    void clearAllAgents();
    void clearOfflineAgents();
private:
    Ui::MainWindow *ui;
    QTimer *elapsedTimer;
    TcpServer *tcpServer;
    void updateStatus();
    int totalAgents;
    int onlineAgents;
    void updateElapsedTime();
    QIcon makeCircleIcon(const QColor &color);

};
#endif // MAINWINDOW_H
