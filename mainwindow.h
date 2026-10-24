#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "tcpserver.h"
#include <QMainWindow>
#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class AlertWindow;                // forward declaration
class TcpServer;                  // forward declaration

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void addServer(const QString &name, const QString &ip);
    void onMetricsReceived(const QString &name, int cpu,
                           int ram, int ramTotal,
                           int disk, int diskTotal);
    void onAgentDisconnected(const QString &name);
    void exportReport();
    //void loadServers();
    void clearAllAgents();
    void clearOfflineAgents();
    void testAlert();
    void saveSettings();
private:
    Ui::MainWindow *ui;
    QTimer *elapsedTimer;
    AlertWindow *alertWindow;
    TcpServer *tcpServer;
    void updateStatus();
    int totalAgents;
    int onlineAgents;
    void updateElapsedTime();
    QIcon makeCircleIcon(const QColor &color);
    void loadSettings();
    void setupThresholdValidation();
};
#endif // MAINWINDOW_H
