#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include <QRandomGenerator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QTableWidgetItem>
#include <QStatusBar>
#include <QColor>
#include <QDateTime>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),totalAgents(0),onlineAgents(0)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tcpServer = new TcpServer(this);

    //tcpServer->start(12345);
    bool starterted = tcpServer->start(12345);
    if(starterted)
    {
        statusBar()->showMessage("Server started on port 12345");
        //qDebug() << "Message set to status bar";
    }
    else
    {
        statusBar()->showMessage("Failed to start server on port 12345");
        //qDebug() << "Failed message set";
    }

    elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MainWindow::updateElapsedTime);
    elapsedTimer->start(1000);

    //Set up table columns
    ui->tableWidget->setColumnCount(7);
    ui->tableWidget->setHorizontalHeaderLabels({"Status","Name", "IP", "CPU", "RAM", "Last Update", "Elapsed"}); // وقتی کاربر روی این دکمه کلیک کرد تابع addServer روی this صدا زده میشه

    // Connect buttons
    connect(tcpServer, &TcpServer::agentConnected, this, &MainWindow::addServer);
    connect(tcpServer, &TcpServer::metricsReceived, this, &MainWindow::onMetricsReceived);
    connect(tcpServer, &TcpServer::agentDisconnected, this, &MainWindow::onAgentDisconnected);
    connect(ui->pushButton_save, &QPushButton::clicked, this, &MainWindow::saveServers);
    connect(ui->pushButton_load, &QPushButton::clicked, this, &MainWindow::loadServers);
    connect(ui->pushButton_clearAll, &QPushButton::clicked, this, &MainWindow::clearAllAgents);
    connect(ui->pushButton_clearOff, &QPushButton::clicked, this, &MainWindow::clearOfflineAgents);
}

void MainWindow::addServer(const QString &name, const QString &ip)
{
    // چک کن که این Agent قبلا اضافه نشده باشه
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        if(ui->tableWidget->item(i,1)->text() == name){
            // قبلا هست ، فقط آی پی رو آپدیت کن
            ui->tableWidget->item(i,2)->setText(ip);
            ui->tableWidget->item(i, 0)->setBackground(QColor("#C6EFCE"));
            ui->tableWidget->item(i,0)->setText("On");
            onlineAgents++;
            updateStatus();
            statusBar()->showMessage("Agent reconnected: " + name);
            return;
        }
    }
    // Agent جدید یک ردیف اضافه کن
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem("New"));
    ui->tableWidget->item(row, 0)->setBackground(QColor("#FFC7CE"));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(name));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(ip));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem("-"));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss")));
    ui->tableWidget->setItem(row, 6, new QTableWidgetItem("-"));

    totalAgents++;
    onlineAgents++;
    updateStatus();
    statusBar()->showMessage("Agent connected: " + name);
}

void MainWindow::onMetricsReceived(const QString &name, int cpu, int ram)
{
    for(int i = 0; i < ui->tableWidget->rowCount(); ++i){
        if(ui->tableWidget->item(i,1)->text() == name){
            ui->tableWidget->item(i,3)->setText(QString::number(cpu) + " %");
            ui->tableWidget->item(i,4)->setText(QString::number(ram) + " Mb");
            ui->tableWidget->item(i,5)->setText(QDateTime::currentDateTime().toString());
            ui->tableWidget->item(i,5)->setData(Qt::UserRole, QDateTime::currentDateTime());

            statusBar()->showMessage(QString("Received cpu and ram usage for %1 server agent").arg(name));
            break;
        }
    }
}

void MainWindow::onAgentDisconnected(const QString &name)
{
    for(int i = 0; i < ui->tableWidget->rowCount(); ++i){
        if(ui->tableWidget->item(i,1)->text() == name){
            //ui->tableWidget->item(i, 0)->setText(QString("%1 (OffLine)").arg(name));
            // تغییر رنگ پس زمینه همه این ردیف به زرد
            ui->tableWidget->item(i, 0)->setBackground(QColor("#FFEB9C"));
            ui->tableWidget->item(i,0)->setText(" Off ");
            ui->tableWidget->item(i,3)->setText(" 0 ");
            ui->tableWidget->item(i,4)->setText(" 0 ");

            onlineAgents--;
            updateStatus();
            statusBar()->showMessage(QString(" %1's server went OFFLINE").arg(name));
            break;
        }
    }
}

void MainWindow::updateStatus()
{
    ui->label_status->setText(QString("Online : %1  /  Total : %2 ").arg(onlineAgents).arg(totalAgents));
}

void MainWindow::saveServers()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Servers", "", "JSON Files (*.json)");
    if (fileName.isEmpty())
        return;

    QJsonArray servers;
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        QJsonObject server;
        server["name"] = ui->tableWidget->item(i, 1)->text();
        server["ip"] = ui->tableWidget->item(i, 2)->text();
        server["cpu"] = ui->tableWidget->item(i, 3)->text().toInt();
        server["ram"] = ui->tableWidget->item(i, 4)->text().toInt();
        servers.append(server);
    }

    QJsonDocument doc(servers);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for writing.");
        return;
    }
    file.write(doc.toJson());
    file.close();

    statusBar()->showMessage(QString("Saved %1 servers to %2").arg(servers.size()).arg(fileName));
}

void MainWindow::loadServers()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Servers", "", "JSON Files (*.json)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for reading.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        QMessageBox::warning(this, "Error", "Invalid JSON format.");
        return;
    }

    QJsonArray servers = doc.array();

    // Clear table
    ui->tableWidget->setRowCount(0);

    for (const QJsonValue &value : servers) {
        QJsonObject obj = value.toObject();
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(obj["name"].toString()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(obj["ip"].toString()));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(obj["cpu"].toInt())));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(obj["ram"].toInt())));
    }

    statusBar()->showMessage(QString("Loaded %1 servers from %2").arg(servers.size()).arg(fileName));
}

void MainWindow::clearAllAgents()
{
    ui->tableWidget->setRowCount(0);
    totalAgents = 0;
    onlineAgents = 0;
    updateStatus();
    statusBar()->showMessage("All Agents cleared.");
    tcpServer->stop();
    tcpServer->start(12345);
}

void MainWindow::clearOfflineAgents()
{
    for(int i = ui->tableWidget->rowCount()-1; i >= 0; --i){
        if(ui->tableWidget->item(i,0)->text() == " Off " ){
            ui->tableWidget->removeRow(i);
            totalAgents--;
        }
    }
    updateStatus();
    statusBar()->showMessage("Offline agents cleard.");
}

void MainWindow::updateElapsedTime()
{
    for(int i = 0; i < ui->tableWidget->rowCount(); ++i){
        QVariant data = ui->tableWidget->item(i, 5)->data(Qt::UserRole);
        if (data.canConvert<QDateTime>()){
            QDateTime lastTime = data.toDateTime();
            qint64 secs = lastTime.secsTo(QDateTime::currentDateTime());

            QString elapsedText;

            if (secs<5)
                elapsedText = "just now";
            else if (secs < 60)
                elapsedText = QString("%1s ago").arg(secs);
            else if (secs < 3600)
                elapsedText = QString("%1m ago").arg(secs/60);
            else if (secs < 86400)
                elapsedText = QString("%1h ago").arg(secs/3600);
            else
                elapsedText = QString("%1d ago").arg(secs/86400);

            ui->tableWidget->item(i, 6)->setText(elapsedText);
        }

    }
}

MainWindow::~MainWindow()
{
    // این شرط و دستور بعد آن لازم نیست به لحاظ فنی اما برای شفافیت کد بد نیست
    if(tcpServer)
        tcpServer->stop();

    delete ui;
}
