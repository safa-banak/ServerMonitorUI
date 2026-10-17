#ifndef ALERTWINDOW_H
#define ALERTWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QCloseEvent>
#include <QColor>

class AlertWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AlertWindow(QWidget *parent = nullptr);
    ~AlertWindow();

    void updateAgentAlert(const QString &name, int cpu, int ram, int disk);
    void removeAgent(const QString &name);
    void restoreFromMinimized();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    int findRowByName(const QString &name);
    QColor severityColor(int percent);

    QTableWidget *table;
};

#endif // ALERTWINDOW_H
