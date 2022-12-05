#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QTimer>
#include <QLabel>
#include "device.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Device*, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Device* device;
    QVector<QLabel*> graph;
    QTimer graphTimer;

    void setupGraph();
    void clearDisplay();
    void displayBatteryInfo();
    void setGraph(int, int, bool = false);
    void setGraphLights(int, int);
    void graphBlink(int, int);
    void setDeviceButtonsEnabled(bool);

private slots:
    void updateDisplay();
};
#endif // MAINWINDOW_H
