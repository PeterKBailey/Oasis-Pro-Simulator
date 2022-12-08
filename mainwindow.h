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
    QTimer wavelengthBlinkTimer;

    void setupGraph();
    void clearDisplay();
    void displayBatteryInfo();
    void setGraph(int, int, bool = false, QString = "black");
    void setGraphLights(int, int, QString = "black");
    void graphBlink(int, int, QString = "black");
    void setDeviceButtonsEnabled(bool);
    void setWavelength(QString, bool = false, QString = "black");
    void wavelengthBlink(QString);

private slots:
    void updateDisplay();
    void updateWavelength();
};
#endif // MAINWINDOW_H
