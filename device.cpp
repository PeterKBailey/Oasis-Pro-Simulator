#include "device.h"

Device::Device(QObject *parent) : QObject(parent),
    batteryLevel(26), activeWavelength("none"), intensity(1), state(State::Off), remainingSessionTime(-1), connectionStatus(ConnectionStatus::No), selectedSessionGroup(0), selectedSessionType(0)
{
    // set up the powerButtonTimer, tell it not to repeat, tell it to stop after 1s
    this->powerButtonTimer.setSingleShot(true);
    this->powerButtonTimer.setInterval(1000);
    connect(&powerButtonTimer, SIGNAL(timeout()), this, SLOT(PowerButtonHeld()));

    this->sessionTimer.setSingleShot(true);
    connect(&sessionTimer, SIGNAL(timeout()), this, SLOT(StartSessionButtonClicked()));

    //
    this->batteryLevelTimer.setInterval(2000);
    connect(&batteryLevelTimer, SIGNAL(timeout()), this, SLOT(DepleteBattery()));
    if(this->batteryLevel > 25){
        this->batteryState = BatteryState::High;
    }
    else if(this->batteryLevel > 12){
        this->batteryState = BatteryState::Low;
    }
    else {
        this->batteryState = BatteryState::CriticallyLow;
    }

    configureDevice();
}

Device::~Device()
{
    for (SessionGroup *group : sessionGroups) delete group;
    for (SessionType *type : sessionTypes) delete type;
}

//Initialize session groups and types
void Device::configureDevice()
{
    //Add all session groups
    sessionGroups.append(new SessionGroup("20 Min", 20));
    sessionGroups.append(new SessionGroup("45 Min", 45));
    sessionGroups.append(new SessionGroup("User Designed", 0));

    //Add all session types
    sessionTypes.append(new SessionType("MET", "small"));
    sessionTypes.append(new SessionType("Sub-Delta", "big"));
    sessionTypes.append(new SessionType("Delta", "small"));
    sessionTypes.append(new SessionType("Theta", "small"));
}


State Device::getState() const
{
    return state;
}

double Device::getBatteryLevel() const
{
    return batteryLevel;
}

ConnectionStatus Device::getConnectionStatus() const
{
    return connectionStatus;
}

int Device::getIntensity() const
{
    return intensity;
}

QString Device::getActiveWavelength() const
{
    return activeWavelength;
}

BatteryState Device::getBatteryState() const
{
    return batteryState;
}

void Device::powerOn(){
    this->state = State::ChoosingSession;
    this->batteryLevelTimer.start();
}
void Device::powerOff(bool softOff) {
    if(softOff){
        // ?
    }
    this->state = State::Off;
    this->batteryLevelTimer.stop();
}

// SLOTS
// person presses mouse
void Device::PowerButtonPressed(){
    // timer starts
    this->powerButtonTimer.start();
    qDebug() << "power pressed\n";

}
// if they let it go before 1s, timer stops (i.e. clicked not held)
void Device::PowerButtonReleased(){
    this->powerButtonTimer.stop();
    // depending on state do something (cycle through groups or whatever)
    qDebug() << "power released\n";
}
// else they didnt let it go within 1s, this happens
void Device::PowerButtonHeld(){
    if(this->state == State::Off){
        this->powerOn();
    } else {
        this->powerOff(false);
    }
    qDebug() << "power held\n";
    emit this->deviceUpdated();
}
void Device::INTArrowButtonClicked(QAbstractButton* directionButton){

}
void Device::StartSessionButtonClicked(){
    //    sessionTimer->setInterval(5000);
    this->activeWavelength = sessionTypes[selectedSessionType]->wavelength;
    enterTestMode();

}
void Device::ResetBattery(){
    this->batteryLevel = 100;
    this->batteryState = BatteryState::High;
    emit this->deviceUpdated();
}

void Device::SetConnectionStatus(int status)
{
    auto prevStatus = this->connectionStatus;
    this->connectionStatus = status == 0 ? ConnectionStatus::No : status == 1 ? ConnectionStatus::Okay : ConnectionStatus::Excellent;
    if (state == State::TestingConnection && prevStatus == ConnectionStatus::No){
        this->confirmConnection();
    } else if (state == State::InSession && connectionStatus == ConnectionStatus::No){
        //disconnected during session
        this->pauseSession();
    }
}
void Device::SessionComplete(){

}

void Device::pauseSession(){
    // only works for singlshot (?)
    this->remainingSessionTime = this->sessionTimer.remainingTime();
    this->sessionTimer.stop();
    this->state = State::Paused;
    emit this->deviceUpdated();
}

void Device::resumeSession(){
    // if there is a session to resume
    if(remainingSessionTime > -1){
        this->sessionTimer.setInterval(remainingSessionTime);
        this->state = State::InSession;
    }
    // otherwise
    else {
        this->state = State::ChoosingSession; // maybe?
    }
    emit this->deviceUpdated();
}

void Device::DepleteBattery(){
    static int prevWholeLevel;
    // these don't make sense because once the device is recharged theyre still false
    static bool batteryLow = false;
    static bool batteryCriticallyLow = false;

    prevWholeLevel = this->batteryLevel;

    switch(this->state){
        case State::ChoosingSession: case State::ChoosingSavedTherapy:
            this->batteryLevel -= 0.1;
            break;
        case State::InSession:
            this->batteryLevel -= 0.2*this->intensity; // assuming intensity 1-8
            break;
        case State::Paused:
            this->batteryLevel -= 0.05;
    }

    // battery reaches 25% or lower
    if(this->batteryLevel <= 25 && batteryState == BatteryState::High){
        this->batteryState = BatteryState::Low;
        emit this->deviceUpdated();
    }

    // battery reaches 12% or lower
    if(this->batteryLevel <= 12 && batteryState != BatteryState::CriticallyLow){
        this->batteryState = BatteryState::CriticallyLow;
        this->pauseSession();
        emit this->deviceUpdated();
    }

    // only update the display when at least 1% is lost
    if(prevWholeLevel - (int)this->batteryLevel >= 1 || this->batteryLevel <= 12){
        emit this->deviceUpdated();
    }
}

void Device::startSession()
{
    this->state = State::InSession;
    emit this->deviceUpdated();
}

//performs connection test at the start of each session
void Device::enterTestMode()
{
    qDebug() << "testing connection...";
    this->state = State::TestingConnection;
    emit this->deviceUpdated();
    QTimer::singleShot(5000, this, SLOT(confirmConnection()));//let the display show connection status for 5 seconds and then start session if there is a connection
}

//start session if there is a connection
void Device::confirmConnection()
{
    if (connectionStatus != ConnectionStatus::No){
        emit this->endConnectionTest();
        qDebug() << "connection confirmed, starting session...";
        startSession();
    } else {
        qDebug() << "No connection. Waiting for connection to start session...";
    }
}
