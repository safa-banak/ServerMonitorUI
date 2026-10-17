#include "alertwindow.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QBrush>
#include <QColor>

AlertWindow::AlertWindow(QWidget *parent)
    : QWidget{parent, Qt::Window | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint}
{
    setWindowTitle("Active Alerts");
    resize(500, 300);

    //setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);  //حذف دکمه X

    table = new QTableWidget(this);                   // ساخت جدول
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Agent", "CPU", "RAM", "Disk"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(table);
}

AlertWindow::~AlertWindow()
{
}

int AlertWindow::findRowByName(const QString &name)
{
    for(int i = 0; i < table->rowCount(); ++i) {
        if(table->item(i,0) && table->item(i, 0)->text() == name)
        {
            return i;
        }
    }
    return -1;
}

QColor AlertWindow::severityColor(int percent)
{
    if (percent > 90) return QColor(QString("#8B0000"));     // Critical
    if (percent > 80) return QColor(QString("#8B8000"));     // warning
    return QColor();                                // Normal
}

void AlertWindow::updateAgentAlert(const QString &name, int cpu, int ram, int disk)
{
    int row = findRowByName(name);

    if(row == -1){
        row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(name));
        table->setItem(row, 1, new QTableWidgetItem());
        table->setItem(row, 2, new QTableWidgetItem());
        table->setItem(row, 3, new QTableWidgetItem());
    }

    QTableWidgetItem *cpuItem = table->item(row, 1);
    cpuItem->setText(QString::number(cpu) + " %");
    cpuItem->setBackground(severityColor(cpu));

    QTableWidgetItem *ramItem = table->item(row, 2);
    ramItem->setText(QString::number(ram) + " %");
    ramItem->setBackground(severityColor(ram));

    QTableWidgetItem *diskItem = table->item(row, 3);
    diskItem->setText(QString::number(disk) + " %");
    diskItem->setBackground(severityColor(disk));
}

void AlertWindow::removeAgent(const QString &name)
{
    int row = findRowByName(name);
    if (row >= 0)
        table->removeRow(row);

    if(table->rowCount() == 0)
        hide();                    // or showMinimized();
}

void AlertWindow::restoreFromMinimized()
{
    if (isMinimized())
        showNormal();

    raise();
    activateWindow();
}

void AlertWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    showMinimized();
}
