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


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //Set up table columns
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels({"Name", "IP", "CPU", "RAM"}); // وقتی کاربر روی این دکمه کلیک کرد تابع addServer روی this صدا زده میشه

    // Connect buttons
    connect(ui->pushButton_add, &QPushButton::clicked, this, &MainWindow::addServer);
    connect(ui->pushButton_refresh, &QPushButton::clicked, this, &MainWindow::refreshServers);
    connect(ui->pushButton_save, &QPushButton::clicked, this, &MainWindow::saveServers);
    connect(ui->pushButton_load, &QPushButton::clicked, this, &MainWindow::loadServers);
    connect(ui->pushButton_delete, &QPushButton::clicked, this, &MainWindow::deleteServer);

    // Update Status bar
    statusBar()->showMessage("Ready");

    // ساخت تایمر به‌روزرسانی خودکار
    autoRefreshTimer = new QTimer(this);
    // اتصال تایمر به slot به‌روزرسانی
    connect(autoRefreshTimer,&QTimer::timeout,this,&MainWindow::refreshServers);
    // اتصال چک‌باکس به شروع/توقف تایمر
    connect(ui->checkBox_autoRefresh, &QCheckBox::toggled, this, [this](bool checked){
        if (checked){
            int interval = ui->spinBox_interval->value() * 1000; //تبدیل ثانیه به میلی ثانیه
            autoRefreshTimer->start(interval);
            statusBar()->showMessage(QString("Auto refresh enabled (every %1 s)").arg(ui->spinBox_interval->value()));
        }
        else {
            autoRefreshTimer->stop();
            statusBar()->showMessage("Auto refresh disabled");
        }
    });
    // اگه کاربر بازه‌ی زمانی رو عوض کرد و تایمر فعاله، بازه‌ی جدید اعمال بشه
    connect(ui->spinBox_interval, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value){
        if(autoRefreshTimer->isActive()) {
            autoRefreshTimer->start(value * 1000);
            statusBar()->showMessage(QString("interval changed to %1 s").arg(value));
        }
    });
}

void MainWindow::addServer()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Add Server", "Server name", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString ip = QInputDialog::getText(this, "Add Server", "IP address:", QLineEdit::Normal, "192.168.1.", &ok);
    if (!ok || ip.isEmpty()) return;

    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(name));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(ip));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem("0"));

    statusBar()->showMessage(QString("Server added: %1").arg(name));
}

void MainWindow::refreshServers()
{
    if (ui->tableWidget->rowCount() == 0) {
        statusBar()->showMessage("No servers to refresh.");
        return;
    }
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        int cpu = QRandomGenerator::global()->bounded(101);          // 0-100
        int ram = QRandomGenerator::global()->bounded(512, 8192);   // 512-8191 MB
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(cpu)));
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(ram)));
    }

    statusBar()->showMessage(QString("Servers refreshed: %1").arg(ui->tableWidget->rowCount()));
}

void MainWindow::saveServers()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Servers", "", "JSON Files (*.json)");
    if (fileName.isEmpty())
        return;

    QJsonArray servers;
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        QJsonObject server;
        server["name"] = ui->tableWidget->item(i, 0)->text();
        server["ip"] = ui->tableWidget->item(i, 1)->text();
        server["cpu"] = ui->tableWidget->item(i, 2)->text().toInt();
        server["ram"] = ui->tableWidget->item(i, 3)->text().toInt();
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
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(obj["name"].toString()));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(obj["ip"].toString()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(obj["cpu"].toInt())));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(obj["ram"].toInt())));
    }

    statusBar()->showMessage(QString("Loaded %1 servers from %2").arg(servers.size()).arg(fileName));
}

void MainWindow::deleteServer()
{
    int row = ui->tableWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Delete Server", "Please select a server row first.");
        return;
    }
    ui->tableWidget->removeRow(row);
    ui->tableWidget->item(-1,-1);
    statusBar()->showMessage("Server deleted");
}

MainWindow::~MainWindow()
{
    delete ui;
}
