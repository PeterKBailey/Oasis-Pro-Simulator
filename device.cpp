#include "device.h"

Device::Device(QObject *parent) : QObject(parent),
    batteryLevel(26), state(State::Off), remainingSessionTime(-1)
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
}

State Device::getState() const
{
    return state;
}

double Device::getBatteryLevel() const
{
    return batteryLevel;
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
}
void Device::ResetBattery(){
    this->batteryLevel = 100;
    this->batteryState = BatteryState::High;
    emit this->deviceUpdated();
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
