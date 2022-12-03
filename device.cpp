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
}

State Device::getState() const
{
    return state;
}

int Device::getBatteryLevel() const
{
    return batteryLevel;
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
        this->state = State::ChoosingSession;
    } else {
        this->state = State::Off;
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
