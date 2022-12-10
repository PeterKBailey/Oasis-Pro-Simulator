#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(Device* d, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setupGraph();
    this->device = d;
    this->displayBatteryInfo();

    //Icons
    this->ui->intUpButton->setIcon(style.standardIcon(QStyle::SP_ArrowUp));
    this->ui->intDownButton->setIcon(style.standardIcon(QStyle::SP_ArrowDown));
    this->ui->checkMarkButton->setIcon(style.standardIcon(QStyle::SP_DialogApplyButton));
    this->ui->checkMarkButton->setIconSize(QSize(40,40));

    // observe
    connect(d, SIGNAL(deviceUpdated()), this, SLOT(updateDisplay()));
    connect(d, SIGNAL(connectionTest(bool)), this, SLOT(updateWavelengthBlinker(bool)));

    // session timer timer
    this->sessionTimerChecker.setInterval(1000);
    connect(&this->sessionTimerChecker, SIGNAL(timeout()), this, SLOT(displaySessionTime()));

    // setup ui
    connect(ui->powerButton, SIGNAL(pressed()), this->device, SLOT(PowerButtonPressed()));
    connect(ui->powerButton, SIGNAL(released()), this->device, SLOT(PowerButtonReleased()));
    connect(ui->intArrowButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this->device, SLOT(INTArrowButtonClicked(QAbstractButton*)));
    connect(ui->checkMarkButton, SIGNAL(pressed()), this->device, SLOT(StartSessionButtonClicked()));
    connect(ui->batteryLevelSlider, SIGNAL(valueChanged(int)), this->device, SLOT(SetBattery(int)));
    connect(ui->replaceBatteryButton, SIGNAL(pressed()), this->device, SLOT(ResetBattery()));
    connect(ui->connectionStrengthSlider, SIGNAL(valueChanged(int)), this->device, SLOT(SetConnectionStatus(int)));

    connect(ui->usernameInput, SIGNAL(textEdited(QString)), this->device, SLOT(UsernameInputted(QString)));
    connect(ui->recordTherapyButton, SIGNAL(pressed()), this->device, SLOT(RecordButtonClicked()));
    connect(ui->replayTherapyButton, SIGNAL(pressed()), this->device, SLOT(ReplayButtonClicked()));

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
    this->displayBatteryInfo();
    this->displaySessionTime();

    auto state = device->getState();

    this->ui->replayTherapyButton->setEnabled(state == State::ChoosingSession && device->getRecordedTherapies().count() > 0);
    if(state == State::Off){
        stopAllTimers();
        this->clearDisplay();
        return;
    }
    else if (state == State::TestingConnection){
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
    }
    else if (state == State::InSession){
        if(!this->sessionTimerChecker.isActive())
            this->sessionTimerChecker.start();
        auto intensity = this->device->getIntensity();
        // if the graph is animating then don't overwrite with intensity
        if(!this->graphTimer.isActive())
            this->setGraph(intensity, intensity, false, "green");
    }
    else if(state == State::SoftOff) {
        auto intensity = this->device->getIntensity();
        this->setGraph(intensity, intensity, false, "green");
    }
    else if(state == State::ChoosingSession){

    }
    else if(state == State::ChoosingSavedTherapy){
        this->ui->treatmentHistoryList->setCurrentRow(device->getSelectedRecordedTherapy());

    }

    auto wavelength = this->device->getActiveWavelength();
    this->setWavelength(wavelength, false, "red");

    setDeviceButtonsEnabled(state != State::Paused);
    this->ui->powerButton->setStyleSheet("border: 5px solid green;");
    displayRecordedSessions();
    highlightSession();
}

void MainWindow::stopAllTimers() {
    graphTimer.stop();
    wavelengthBlinkTimer.stop();
}

void MainWindow::clearDisplay(){
    this->ui->powerButton->setStyleSheet("");

    // clear recorded therapy items
//    this->ui->treatmentHistoryList->clearSelection();
    this->ui->treatmentHistoryList->clear();
    this->ui->usernameInput->clear();

    // turn off graph
    this->setGraph(0,0);

    // turn off CES mode wavelengths
    this->setWavelength("none");

    //disable device buttons
    setDeviceButtonsEnabled(false);

    unHighlightSession();
}

void MainWindow::setDeviceButtonsEnabled(bool flag)
{
    this->ui->checkMarkButton->setEnabled(device->getState() == State::ChoosingSession || device->getState() == State::ChoosingSavedTherapy);
    this->ui->intUpButton->setEnabled(flag);
    this->ui->intDownButton->setEnabled(flag);

    auto state = device->getState();
    if(state == State::InSession){
        this->ui->usernameInput->setEnabled(true);
    } else{
        this->ui->usernameInput->setEnabled(false);
    }
    this->toggleRecordButton();
}

void MainWindow::displayBatteryInfo(){
    auto newBatteryLevel = (int)device->getBatteryLevel();
    BatteryState newBatteryState = device->getBatteryState();

    this->ui->batteryDisplay->display(newBatteryLevel);
    this->ui->batteryLevelSlider->blockSignals(true);
    this->ui->batteryLevelSlider->setValue(newBatteryLevel);
    this->ui->batteryLevelSlider->blockSignals(false);

    if(device->getRunBatteryAnimation()){
        if(newBatteryState == BatteryState::Low){
            qDebug() << "Battery Low";
            this->setGraph(1, 2, true, "yellow");
        }
        else if(newBatteryState == BatteryState::Critical){
            qDebug() << "Battery Critically Low";
            this->setGraph(1, 1, true, "red");
        }
    }
}

void MainWindow::setWavelength(QString wavelength, bool blink, QString colour)
{

    if (wavelength == "small" || wavelength == "big"){
        QLabel *activeIcon, *inactiveIcon;
        activeIcon = wavelength == "small" ? this->ui->cesSmallWaveIcon : this->ui->cesBigWaveIcon;
        inactiveIcon = wavelength == "small" ? this->ui->cesBigWaveIcon : this->ui->cesSmallWaveIcon;

        if (blink && !this->wavelengthBlinkTimer.isActive()){
            this->isWavelengthBlinkOn = true;
            this->wavelengthBlinkTimer.setInterval(1000);
            disconnect(&this->wavelengthBlinkTimer, &QTimer::timeout, 0, 0);
            connect(&this->wavelengthBlinkTimer, &QTimer::timeout, this, [wavelength, this](){this->wavelengthBlink(wavelength);});
            this->wavelengthBlinkTimer.start();
        } else{
            activeIcon->setStyleSheet("color: " + colour + ";");
            inactiveIcon->setStyleSheet("color: black;");
        }
    } else {
        this->ui->cesSmallWaveIcon->setStyleSheet("color: black;");
        this->ui->cesBigWaveIcon->setStyleSheet("color: black;");
    }
}

void MainWindow::wavelengthBlink(QString wavelength)
{
    if(isWavelengthBlinkOn){
//        qDebug()<<"setting black";
        this->setWavelength(wavelength);
    }
    else {
//        qDebug()<<"setting red";
        this->setWavelength(wavelength, false, "red");
    }

    isWavelengthBlinkOn = !isWavelengthBlinkOn;
}

void MainWindow::updateWavelengthBlinker(bool isStart)
{
    if (isStart){
        auto wavelength = this->device->getActiveWavelength();
        this->setWavelength(wavelength, true, "red");
    } else{
        this->wavelengthBlinkTimer.stop();
    }
}

void MainWindow::setGraph(int start, int end, bool blink, QString colour){
    // stop the graph timer so that it doesn't reset changes
    this->graphTimer.stop();
    // set the lights
    this->setGraphLights(start, end, colour);
    if(blink){
        this->numGraphBlinks = 0;
        this->isGraphBlinkOn = true;
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
    // if on, turn off
    if(this->isGraphBlinkOn){
        this->setGraphLights(0,0);
    }
    // if off, turn on
    else {
        this->setGraphLights(start, end, colour);
    }

    this->isGraphBlinkOn = !this->isGraphBlinkOn;
    this->numGraphBlinks++;
    if(this->numGraphBlinks == 5){
        this->numGraphBlinks = 0;
        // by default blink assumes the lights are on and should be turned off
        this->isGraphBlinkOn = true;
        this->graphTimer.stop();
    }
}

void MainWindow::toggleRecordButton(){
    // Record Button only visible when there is valid text in the username input box
    auto toggleRecord = device->getToggleRecord();
    if(device->getState() == State::InSession){
        this->ui->recordTherapyButton->setEnabled(toggleRecord);
    } else {
        // If device not in session state then ever show the record therapy button
        this->ui->recordTherapyButton->setEnabled(false);
    }
}

void MainWindow::displayRecordedSessions(){
    // only update treatment list when necessary
    if (device->getRecordedTherapies().length() > this->ui->treatmentHistoryList->count()){
        ui->treatmentHistoryList->clear();

        int widgetLength = ui->treatmentHistoryList->count();
        auto list = device->getRecordedTherapies();
        int listLength = list.length();
        for (int i = 0; i<list.length(); ++i){
             QListWidgetItem* item = new QListWidgetItem;
             item->setText("Username: " + list[i]->username + " Group: " + list[i]->group.name + " Type: " + list[i]->type.name + " Intensity: " + QString::number(list[i]->intensity));
             item->setData(Qt::UserRole, QString::number(i));
             ui->treatmentHistoryList->addItem(item);
        }
    }
}

void MainWindow::toggleReplayButton(){
    // Record Button only visible when there is valid text in the username input box
    auto toggleRecord = device->getToggleRecord();
    this->ui->replayTherapyButton->setEnabled(true);
}

void MainWindow::highlightSession(){
    unHighlightSession();
    auto currSessionType = this->device->getSelectedSessionType();
    auto currSessionGroup = this->device->getSelectedSessionGroup();

    // Highlighting for selected sessiong group
    auto sessionGroupParent = this->ui->groupLayout;
    sessionGroupParent->itemAt(currSessionGroup)->widget()->setStyleSheet("background-color: green;");

    // Highlighting for selected sessiong type
    auto sessionTypeParent = this->ui->typeLayout;
    sessionTypeParent->itemAt(currSessionType)->widget()->setStyleSheet("background-color: green;");
}
void MainWindow::unHighlightSession(){
    // Highlighting for selected sessiong group
    auto sessionGroupParent = this->ui->groupLayout;
    for(int i = 0; i < sessionGroupParent->count(); ++i){
        sessionGroupParent->itemAt(i)->widget()->setStyleSheet("");
    }

    // Highlighting for selected sessiong group
    auto sessionTypeParent = this->ui->typeLayout;
    for(int i = 0; i < sessionTypeParent->count(); ++i){
        sessionTypeParent->itemAt(i)->widget()->setStyleSheet("");
    }
}

void MainWindow::displaySessionTime(){
    int remainingTime = this->device->getRemainingSessionTime();
    qDebug() << "Session time remaining: " << remainingTime;
    if (remainingTime <= 0) {
        this->sessionTimerChecker.stop();
        this->ui->therapyTime->display(0);
        return;
    }
    this->ui->therapyTime->display(remainingTime/1000);
}


