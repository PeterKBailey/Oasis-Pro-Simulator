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
    this->ui->powerButton->setStyleSheet("border: 5px solid green;");
    this->displayBatteryInfo();
}

void MainWindow::clearDisplay(){
    this->ui->powerButton->setStyleSheet("");
    this->ui->batteryDisplay->display(0);
    // turn off all the leds and stuff...
    this->setGraph(0,0);
}

void MainWindow::displayBatteryInfo(){
    static BatteryState deviceBatteryState = BatteryState::High;

    // battery
    auto newBatteryLevel = device->getBatteryLevel();
    BatteryState newBatteryState = device->getBatteryState();

    this->ui->batteryDisplay->display((int)newBatteryLevel);
    if(newBatteryState != deviceBatteryState){
        deviceBatteryState = newBatteryState;
        if(newBatteryState == BatteryState::Low){
            qDebug() << "low";
            this->setGraph(1, 2, true);
        }
        else if(newBatteryState == BatteryState::CriticallyLow){
            qDebug() << "crit low";
            this->setGraph(1, 1, true);
        }
    }
}

void MainWindow::setGraph(int start, int end, bool blink){
    // stop the graph timer so that it doesn't reset changes
    this->graphTimer.stop();
    // set the lights
    this->setGraphLights(start, end);
    if(blink){
        // graph timer can be applied to more than just blinking so disconnect other slots
        disconnect(&this->graphTimer, &QTimer::timeout, 0, 0);
        // qt signal slot using lambda expression to run graphBlink with the specified values
        connect(&this->graphTimer, &QTimer::timeout, this, [start, end, this](){this->graphBlink(start, end);});
        this->graphTimer.setInterval(1000);
        this->graphTimer.start();
    }
}

void MainWindow::setGraphLights(int start, int end){
    for(int i = 1; i < 9; ++i){
        if(i >= start && i <= end){
            this->graph.at(i-1)->setStyleSheet("background-color: #98ff98;");
        } else {
            this->graph.at(i-1)->setStyleSheet("background-color: black;");
        }
    }
}

void MainWindow::graphBlink(int start, int end){
    static int numBlinks = 0;
    static bool isOn = true;

    // if on, turn off
    if(isOn){
        this->setGraphLights(0,0);
    }
    // if off, turn on
    else {
        this->setGraphLights(start, end);
    }

    isOn = !isOn;
    numBlinks++;
    if(numBlinks == 5){
        numBlinks = 0;
        // by default blink assumes the lights are on and should be turned off
        isOn = true;
        this->graphTimer.stop();
    }
}
