#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(Device* d, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setupGraph();
    this->device = d;

    // observe
    connect(d, SIGNAL(deviceUpdated()), this, SLOT(updateDisplay()));

    // setup ui
    connect(ui->powerButton, SIGNAL(pressed()), this->device, SLOT(PowerButtonPressed()));
    connect(ui->powerButton, SIGNAL(released()), this->device, SLOT(PowerButtonReleased()));
    connect(ui->intArrowButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this->device, SLOT(INTArrowButtonClicked(QAbstractButton*)));
    connect(ui->checkMarkButton, SIGNAL(pressed()), this->device, SLOT(StartSessionButtonClicked()));
    connect(ui->replaceBatteryButton, SIGNAL(pressed()), this->device, SLOT(ResetBattery()));
}

void MainWindow::setupGraph(){
    auto gWidget = this->ui->graphWidget;
    for(int i = 1; i < 9; ++i){
        QString graphLightName = "graphLight";
        graphLightName.append(QString::number(i));
        this->graph.append(gWidget->findChild<QLabel*>(graphLightName));
    }
}

MainWindow::~MainWindow()
{
    delete device;
    delete ui;
}

void MainWindow::updateDisplay(){
    auto state = device->getState();
    if(state == State::Off){
        this->clearDisplay();
        return;
    }
    this->setGraph(3,6);
    this->ui->powerButton->setStyleSheet("border: 5px solid green;");
    this->ui->batteryDisplay->display((int)device->getBatteryLevel());
}

void MainWindow::clearDisplay(){
    this->ui->powerButton->setStyleSheet("");
    this->ui->batteryDisplay->display(0);
    // turn off all the leds and stuff...
    this->setGraph(0,0);
}

void MainWindow::setGraph(int start, int end, bool blink){
    for(int i = 1; i < 9; ++i){
        if(i >= start && i <= end){
            this->graph.at(i-1)->setStyleSheet("background-color: #98ff98;");
        } else {
            this->graph.at(i-1)->setStyleSheet("background-color: black;");
        }
    }
}
