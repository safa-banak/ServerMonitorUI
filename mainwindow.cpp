#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "alertwindow.h"
#include "tcpserver.h"
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
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),totalAgents(0),onlineAgents(0)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tabWidget->setCurrentIndex(0);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("monitoring.db");

    if(!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
    } else {
        QSqlQuery query;
        QString createTable = "CREATE TABLE IF NOT EXISTS metrics ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                              "agent_name TEXT, "
                              "cpu INTEGER, "
                              "ram INTEGER, "
                              "disk INTEGER)";
        if (!query.exec(createTable)) {
            qDebug() << "Failed to create table:" << query.lastError().text();
        }
    }

    // تنظیمات ظاهری جدول تاریخچه (فقط یک بار در سازنده)
    ui->tableWidget_history->setColumnCount(5);
    ui->tableWidget_history->setHorizontalHeaderLabels({"Time", "Agent", "CPU", "RAM", "Disk"});
    ui->tableWidget_history->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_history->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_history->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_history->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_history->setStyleSheet("QTableWidget::item { padding: 5px; }");

    loadSettings();
    connect(ui->pushButton_saveSettings, &QPushButton::clicked, this, &MainWindow::saveSettings);
    setupThresholdValidation();

    alertWindow = new AlertWindow(this);
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
    ui->tableWidget->setColumnCount(8);
    ui->tableWidget->setHorizontalHeaderLabels({"Status","Name", "IP", "CPU", "RAM",
                                                "Disk", "Last Update", "Elapsed"}); // وقتی کاربر روی این دکمه کلیک کرد تابع addServer روی this صدا زده میشه
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);   // تنظیم رفتار ستون‌ها: به اندازه محتوا باشن
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);           // ۲. اگر بعد از تنظیم محتوا، فضای خالی سمت راست موند، ستون آخر (Elapsed) اون فضا رو پر کنه
    ui->tableWidget->setStyleSheet("QTableWidget::item { padding: 5px; }");
    ui->tableWidget->horizontalHeader()->setMinimumSectionSize(50);              // تعیین حداقل عرض برای ستون‌ها تا بیش از حد باریک نشن

    // Connect buttons
    connect(tcpServer, &TcpServer::agentConnected, this, &MainWindow::addServer);
    connect(tcpServer, &TcpServer::metricsReceived, this, &MainWindow::onMetricsReceived);
    connect(tcpServer, &TcpServer::agentDisconnected, this, &MainWindow::onAgentDisconnected);
    connect(ui->pushButton_export , &QPushButton::clicked, this, &MainWindow::exportReport);
    //connect(ui->pushButton_load, &QPushButton::clicked, this, &MainWindow::loadServers);
    connect(ui->pushButton_clearAll, &QPushButton::clicked, this, &MainWindow::clearAllAgents);
    connect(ui->pushButton_clearOff, &QPushButton::clicked, this, &MainWindow::clearOfflineAgents);
    connect(ui->pushButton_testAlert, &QPushButton::clicked, this, &MainWindow::testAlert);
    connect(ui->pushButton_refreshHistory, &QPushButton::clicked, this, &MainWindow::loadHistory);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index){
        if(index == 1)
            loadHistory();
    });
}

void MainWindow::addServer(const QString &name, const QString &ip)
{
    // چک کن که این Agent قبلا اضافه نشده باشه
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        if(ui->tableWidget->item(i,1)->text() == name){
            // قبلا هست ، فقط آی پی رو آپدیت کن
            ui->tableWidget->item(i,2)->setText(ip);
            //ui->tableWidget->item(i, 0)->setBackground(QColor("#C6EFCE"));
            ui->tableWidget->item(i, 0)->setText("On");
            ui->tableWidget->item(i, 0)->setIcon(makeCircleIcon(Qt::green));

            onlineAgents++;
            updateStatus();
            statusBar()->showMessage("Agent reconnected: " + name);
            return;
        }
    }
    // Agent جدید یک ردیف اضافه کن
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    QTableWidgetItem *statusItem = new QTableWidgetItem("New");
    statusItem->setIcon(makeCircleIcon(Qt::blue));

    ui->tableWidget->setItem(row, 0, statusItem);
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(name));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(ip));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 6, new QTableWidgetItem("-"));
    ui->tableWidget->setItem(row, 6, new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss")));
    ui->tableWidget->setItem(row, 7, new QTableWidgetItem("-"));


    totalAgents++;
    onlineAgents++;
    updateStatus();
    statusBar()->showMessage("Agent connected: " + name);
}

void MainWindow::onMetricsReceived(const QString &name, int cpu,
                                   int ram, int ramTotal,
                                   int disk, int diskTotal)
{
    int ramPercent = (ramTotal > 0) ? ( ram * 100 / ramTotal ) : 0;
    int diskPercent = ( diskTotal > 0) ? (disk * 100 / diskTotal) : 0;

    for(int i = 0; i < ui->tableWidget->rowCount(); ++i){
        if(ui->tableWidget->item(i,1)->text() == name){

            QTableWidgetItem *cpuItem = ui->tableWidget->item(i, 3);
            cpuItem->setText(QString::number(cpu) + " %");
            cpuItem->setData(Qt::UserRole, cpu);

            QTableWidgetItem *ramItem = ui->tableWidget->item(i, 4);
            ramItem->setText(QString("%1 MB (%2 %)").arg(ram).arg(ramPercent));
            ramItem->setData(Qt::UserRole, ram);
            ramItem->setData(Qt::UserRole +1, ramPercent);

            QTableWidgetItem *diskItem = ui->tableWidget->item(i, 5);
            diskItem->setText(QString("%1 GB (%2 %)").arg(disk).arg(diskPercent));
            diskItem->setData(Qt::UserRole, disk);
            diskItem->setData(Qt::UserRole + 1, diskPercent);

            QDateTime now = QDateTime::currentDateTime();
            ui->tableWidget->item(i,6)->setText(now.toString("HH:mm:ss"));
            ui->tableWidget->item(i,6)->setData(Qt::UserRole, now);

            QSqlQuery query;
            query.prepare("INSERT INTO metrics (agent_name, cpu, ram, disk) "
                          "VALUES (:name, :cpu, :ram, :disk)");                 // آماده سازی دستور درج (INSERT)
            query.bindValue(":name", name);                                     // جایگذاری مقادیر
            query.bindValue(":cpu", cpu);
            query.bindValue(":ram", ramPercent);
            query.bindValue(":disk", diskPercent);

            if(!query.exec()) {
                qDebug() << "Database Insert Error:" << query.lastError().text();
            }

            statusBar()->showMessage("Received metrics from " + name);
            break;
        }
    }

    int warningThreshold = ui->spinBox_warning->value();
    int criticalThreshold = ui->spinBox_critical->value();
    bool anyAlert = (cpu > warningThreshold) ||
                    (ramPercent > warningThreshold) ||
                    (diskPercent > warningThreshold);

    bool isCritical = (cpu > criticalThreshold) ||
                      (ramPercent > criticalThreshold) ||
                      (diskPercent > criticalThreshold);
    //bool anyAlert = (cpu > 80) || (ramPercent > 80) || (diskPercent > 80);
    //bool isCritical = (cpu > 90) || (ramPercent > 90) || (diskPercent > 90);
    if(anyAlert) {
        alertWindow->updateAgentAlert(name, cpu, ramPercent, diskPercent);
        if(!alertWindow->isVisible()){
            alertWindow->show();
        }
        if(isCritical){
            alertWindow->restoreFromMinimized();
        }
    }else{
        alertWindow->removeAgent(name);
    }

}

void MainWindow::onAgentDisconnected(const QString &name)
{
    for(int i = 0; i < ui->tableWidget->rowCount(); ++i){
        if(ui->tableWidget->item(i,1)->text() == name){
            // تغییر رنگ پس زمینه همه این ردیف به زرد
            ui->tableWidget->item(i, 0)->setText("Off");
            ui->tableWidget->item(i, 0)->setIcon(makeCircleIcon(Qt::yellow));
            ui->tableWidget->item(i, 3)->setText(" 0 ");
            ui->tableWidget->item(i, 3)->setData(Qt::UserRole, 0);
            ui->tableWidget->item(i, 4)->setText(" 0 ");
            ui->tableWidget->item(i, 4)->setData(Qt::UserRole, 0);
            ui->tableWidget->item(i, 4)->setData(Qt::UserRole + 1, 0);
            ui->tableWidget->item(i, 5)->setText(" 0 ");
            ui->tableWidget->item(i, 5)->setData(Qt::UserRole, 0);
            ui->tableWidget->item(i, 5)->setData(Qt::UserRole + 1, 0);
            alertWindow->updateAgentAlert(name, 0, 0, 0);
            alertWindow->removeAgent(name);

            onlineAgents--;
            updateStatus();
            statusBar()->showMessage(name + " went OFFLINE");
            break;
        }
    }
}

void MainWindow::updateStatus()
{
    ui->label_status->setText(QString("Online : %1  /  Total : %2 ").arg(onlineAgents).arg(totalAgents));
}

void MainWindow::exportReport()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Report", "", "JSON Files (*.json)");
    if (fileName.isEmpty())
        return;

    QJsonArray servers;
    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        QJsonObject server;

        server["name"] = ui->tableWidget->item(i, 1)->text();
        server["ip"] = ui->tableWidget->item(i, 2)->text();
        server["status"] = ui->tableWidget->item(i, 0)->text();
        server["cpu_percent"] = ui->tableWidget->item(i, 3)->data(Qt::UserRole).toInt();
        server["ram_used_mb"] = ui->tableWidget->item(i, 4)->data(Qt::UserRole).toInt();
        server["ram_percent"] = ui->tableWidget->item(i, 4)->data(Qt::UserRole +1).toInt();
        server["disk_used_gb"] = ui->tableWidget->item(i, 5)->data(Qt::UserRole).toInt();
        server["disk_percent"]= ui->tableWidget->item(i, 5)->data(Qt::UserRole +1).toInt();

        servers.append(server);
    }

    QJsonObject rootObject;
    rootObject["export_timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    rootObject["servers"] = servers;

    QJsonDocument doc(rootObject);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Could not open file for writing.");
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));   // Indented برای خوانایی بهتر
    file.close();

    statusBar()->showMessage(QString("ٍExported %1 servers to %2").arg(servers.size()).arg(fileName));
}

/*void MainWindow::loadServers()
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
}*/

void MainWindow::clearAllAgents()
{   //اول به سرور میگیم که کلاینتهارو اخراج کنه
    if (tcpServer) {
        tcpServer->kickAllClients();
    }
    // سپس جدول رو پاک کن
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
        if(ui->tableWidget->item(i,0)->text() == "Off" ){
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
        QVariant data = ui->tableWidget->item(i, 6)->data(Qt::UserRole);
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

            ui->tableWidget->item(i, 7)->setText(elapsedText);
        }

    }
}

QIcon MainWindow::makeCircleIcon(const QColor &color)
{
    int size = 12 ;            // اندازه دایره به پیکسل
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);   // پس زمینه شفاف

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing); //لبه های صاف

    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);
    painter.end();

    return QIcon(pixmap);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(alertWindow) {
        alertWindow->close();  // hide() !
        delete alertWindow;
        alertWindow = nullptr;
    }

    if(tcpServer)
        tcpServer->stop();

    QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
    // این شرط و دستور بعد آن لازم نیست به لحاظ فنی اما برای شفافیت کد بد نیست
    if(tcpServer)
        tcpServer->stop();

    delete ui;
}

void MainWindow::testAlert()
{
    // ۱. اول یک هشدار Warning برای Agent دوم می‌سازیم
    alertWindow->updateAgentAlert("test-agent-02", 85, 82, 81);
    if (!alertWindow->isVisible()) alertWindow->show();

    // ۲. بعد از ۲ ثانیه، یک هشدار Critical برای Agent اول می‌سازیم
    QTimer::singleShot(2000, this, [this]() {
        alertWindow->updateAgentAlert("test-agent-01", 95, 92, 91);
        alertWindow->restoreFromMinimized();
    });

    // ۳. بعد از ۴ ثانیه، مقادیر Agent دوم رو نرمال می‌کنیم تا ردیفش حذف بشه
    QTimer::singleShot(4000, this, [this]() {
        alertWindow->removeAgent("test-agent-02");
    });

    // ۴. بعد از ۶ ثانیه، همه رو پاک می‌کنیم
    QTimer::singleShot(6000, this, [this]() {
        alertWindow->removeAgent("test-agent-01");
        statusBar()->showMessage("Test alerts cleared.");
    });

    statusBar()->showMessage("Test alerts triggered...");
}

void MainWindow::saveSettings()
{
    QSettings settings("SafaBanak", "ServerMonitorDashboard");

    settings.setValue("warningThreshold", ui->spinBox_warning->value());

    settings.setValue("criticalThreshold", ui->spinBox_critical->value());

    statusBar()->showMessage("Settings saved successfully.");
}

void MainWindow::loadSettings()
{
    QSettings settings("SafaBanak", "ServerMonitorDashboard");

    int warning = settings.value("warningThreshold", 80).toInt();
    int critical = settings.value("criticalThreshold", 90).toInt();

    ui->spinBox_warning->setValue(warning);
    ui->spinBox_critical->setValue(critical);
}
void MainWindow::setupThresholdValidation()
{
    connect(ui->spinBox_warning, QOverload<int>::of(&QSpinBox::valueChanged)
            ,this ,[this](int value){
        ui->spinBox_critical->setMinimum(value +1);
    });

    connect(ui->spinBox_critical, QOverload<int>::of(&QSpinBox::valueChanged)
            , this, [this](int value){
        ui->spinBox_warning->setMaximum(value - 1);
    });
}

void MainWindow::loadHistory()
{
    ui->tableWidget_history->setRowCount(0);                        // پاک کردن (سطرهای) جدول قبلی

    QSqlQuery query;
    if(!query.exec("SELECT timestamp, agent_name, cpu, ram, disk FROM metrics ORDER BY timestamp DESC LIMIT 100")) {
        qDebug() << "Failed to load history:" << query.lastError().text();
        return;
    }

    // پر کردن جدول با نتایج
    int row = 0;
    while (query.next()) {
        ui->tableWidget_history->insertRow(row);

        QString timestamp = query.value(0).toString();
        QString agentName = query.value(1).toString();
        int cpu = query.value(2).toInt();
        int ram = query.value(3).toInt();
        int disk = query.value(4).toInt();

        ui->tableWidget_history->setItem(row, 0, new QTableWidgetItem(timestamp));
        ui->tableWidget_history->setItem(row, 1, new QTableWidgetItem(agentName));
        ui->tableWidget_history->setItem(row, 2, new QTableWidgetItem(QString::number(cpu) + " %"));
        ui->tableWidget_history->setItem(row, 3, new QTableWidgetItem(QString::number(ram) + " %"));
        ui->tableWidget_history->setItem(row, 4, new QTableWidgetItem(QString::number(disk) + " %"));

        row++;
    }

    statusBar()->showMessage(QString("Loaded %1 records from data base.").arg(row));
}