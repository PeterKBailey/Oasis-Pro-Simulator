#include "device.h"

Device::Device(QObject *parent) : QObject(parent),
    batteryLevel(100), state(State::Off)
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

}

State Device::getState() const
{
    return state;
}

double Device::getBatteryLevel() const
{
    return batteryLevel;
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

}
void Device::SessionComplete(){

}

void Device::DepleteBattery(){
    static int prevWholeLevel;
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

    // prevent the battery from requiring display update constantly
    if(prevWholeLevel - (int)this->batteryLevel == 1)
        emit this->deviceUpdated();
}
