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
    connect(d, SIGNAL(endConnectionTest()), this, SLOT(updateWavelength()));

    // setup ui
    connect(ui->powerButton, SIGNAL(pressed()), this->device, SLOT(PowerButtonPressed()));
    connect(ui->powerButton, SIGNAL(released()), this->device, SLOT(PowerButtonReleased()));
    connect(ui->intArrowButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this->device, SLOT(INTArrowButtonClicked(QAbstractButton*)));
    connect(ui->checkMarkButton, SIGNAL(pressed()), this->device, SLOT(StartSessionButtonClicked()));
    connect(ui->replaceBatteryButton, SIGNAL(pressed()), this->device, SLOT(ResetBattery()));
    connect(ui->connectionStrengthSlider, SIGNAL(valueChanged(int)), this->device, SLOT(SetConnectionStatus(int)));

    clearDisplay();
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
    } else if (state == State::TestingConnection){
        auto status = this->device->getConnectionStatus();
        switch(status){
            case ConnectionStatus::No:
                this->setGraph(7, 8, true, "red");
                break;
            case ConnectionStatus::Okay:
                this->setGraph(4, 6, false, "yellow");
                break;
            case ConnectionStatus::Excellent:
                this->setGraph(1, 3, false, "green");
                break;
        }
        auto wavelength = this->device->getActiveWavelength();
        this->setWavelength(wavelength, true, "red");
    } else if (state == State::InSession){
        auto intensity = this->device->getIntensity();
        auto wavelength = this->device->getActiveWavelength();
        this->setWavelength(wavelength, false, "red");
        this->setGraph(intensity, intensity, false, "green");
    }
    //also need to call setWavelength() for other cases

    setDeviceButtonsEnabled(true);
    this->ui->powerButton->setStyleSheet("border: 5px solid green;");
    this->displayBatteryInfo();
}

void MainWindow::clearDisplay(){
    this->ui->powerButton->setStyleSheet("");
    this->ui->batteryDisplay->display(0);
    // turn off all the leds and stuff...
    this->setGraph(0,0);

    // turn off CES mode wavelengths
    this->setWavelength("none");

    //disable device buttons
    setDeviceButtonsEnabled(false);

}

void MainWindow::setDeviceButtonsEnabled(bool flag)
{
    this->ui->checkMarkButton->setEnabled(flag);
    this->ui->intUpButton->setEnabled(flag);
    this->ui->intDownButton->setEnabled(flag);
    this->ui->recordTherapyButton->setEnabled(flag);
    this->ui->replayTherapyButton->setEnabled(flag);
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
            this->setGraph(1, 2, true, "#98ff98");
        }
        else if(newBatteryState == BatteryState::CriticallyLow){
            qDebug() << "crit low";
            this->setGraph(1, 1, true, "#98ff98");
        }
    }
}

void MainWindow::setWavelength(QString wavelength, bool blink, QString colour)
{
    if (wavelength == "small" || wavelength == "big"){
        auto icon = wavelength == "small" ? this->ui->cesSmallWaveIcon : this->ui->cesBigWaveIcon;
        if (blink){
            qDebug() <<"icon blinking...";
            this->wavelengthBlinkTimer.setInterval(1000);
            this->wavelengthBlinkTimer.callOnTimeout([wavelength, this](){this->wavelengthBlink(wavelength);});
            this->wavelengthBlinkTimer.start();
        } else{
            icon->setStyleSheet("color: " + colour + ";");
        }
    } else {
        this->ui->cesSmallWaveIcon->setStyleSheet("color: " + colour + ";");
        this->ui->cesBigWaveIcon->setStyleSheet("color: " + colour + ";");
    }
}

void MainWindow::wavelengthBlink(QString wavelength)
{
    static bool isWavelengthOn = true;

    if(isWavelengthOn){
        this->setWavelength(wavelength);
    }
    else {
        this->setWavelength(wavelength, false, "red");
    }

    isWavelengthOn = !isWavelengthOn;
}

void MainWindow::updateWavelength()
{
    this->wavelengthBlinkTimer.stop();
}

void MainWindow::setGraph(int start, int end, bool blink, QString colour){
    // stop the graph timer so that it doesn't reset changes
    this->graphTimer.stop();
    // set the lights
    this->setGraphLights(start, end, colour);
    if(blink){
        // graph timer can be applied to more than just blinking so disconnect other slots
        disconnect(&this->graphTimer, &QTimer::timeout, 0, 0);
        // qt signal slot using lambda expression to run graphBlink with the specified values
        connect(&this->graphTimer, &QTimer::timeout, this, [start, end, colour, this](){this->graphBlink(start, end, colour);});
        this->graphTimer.setInterval(1000);
        this->graphTimer.start();
    }
}

void MainWindow::setGraphLights(int start, int end, QString colour){
    for(int i = 1; i < 9; ++i){
        if(i >= start && i <= end){
            this->graph.at(i-1)->setStyleSheet("background-color: " + colour + ";");
        } else {
            this->graph.at(i-1)->setStyleSheet("background-color: black;");
        }
    }
}

void MainWindow::graphBlink(int start, int end, QString colour){
    static int numBlinks = 0;
    static bool isOn = true;

    // if on, turn off
    if(isOn){
        this->setGraphLights(0,0);
    }
    // if off, turn on
    else {
        this->setGraphLights(start, end, colour);
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


