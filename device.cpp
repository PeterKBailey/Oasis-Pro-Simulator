#include "device.h"

Device::Device(QObject *parent) : QObject(parent),
    batteryLevel(29), activeWavelength("none"), intensity(0), state(State::Off), remainingSessionTime(-1), connectionStatus(ConnectionStatus::No), selectedSessionGroup(0), selectedSessionType(0), toggleRecord(false)
{
    // set up the powerButtonTimer, tell it not to repeat, tell it to stop after 1s
    this->powerButtonTimer.setSingleShot(true);
    this->powerButtonTimer.setInterval(1000);
    connect(&powerButtonTimer, SIGNAL(timeout()), this, SLOT(PowerButtonHeld()));

    this->sessionTimer.setSingleShot(true);
    connect(&sessionTimer, SIGNAL(timeout()), this, SLOT(SessionComplete()));

    this->softOffTimer.setInterval(1000);
    connect(&softOffTimer, SIGNAL(timeout()), this, SLOT(CesReduction()));

    //
    this->batteryLevelTimer.setInterval(2000);
    connect(&batteryLevelTimer, SIGNAL(timeout()), this, SLOT(DepleteBattery()));

    this->lowBatteryTriggered = false;
    this->criticalBatteryTriggered = false;

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

BatteryState Device::getBatteryState() {
    if(this->batteryLevel <= 12){
        return BatteryState::Critical;
    }
    else if(this->batteryLevel <= 25){
        return BatteryState::Low;
    }
    return BatteryState::High;
}

QVector<Therapy *> Device::getRecordedTherapies() const
{
    return recordedTherapies;
}

bool Device::getRunBatteryAnimation() const
{
    return runBatteryAnimation;
}

int Device::getSelectedSessionGroup() const
{
    return selectedSessionGroup;
}

int Device::getSelectedSessionType() const
{
    return selectedSessionType;
}

bool Device::getToggleRecord() const
{
    return toggleRecord;
}

QString Device::getInputtedName() const
{
    return inputtedName;
}

void Device::powerOn(){
    this->state = State::ChoosingSession;
    this->batteryLevelTimer.start();
    emit this->deviceUpdated();
}
void Device::powerOff() {
    qDebug() << "Powering off";
    this->state = State::Off;
    this->batteryLevelTimer.stop();
    this->softOffTimer.stop();
    this->intensity = 0;
    this->lowBatteryTriggered = false;
    this->criticalBatteryTriggered = false;
    emit this->deviceUpdated();
}

void Device::softOff() {
    qDebug() << "Soft Off initiated";

    //if curr session not done, disable active session?

    this->state = State::SoftOff;
    softOffTimer.start();
}

void Device::CesReduction() {
    adjustIntensity(-1);

    if(intensity <= 1) {
        softOffTimer.stop();
        powerOff();
    }
}

// SLOTS
// person presses mouse
void Device::PowerButtonPressed(){
    // timer starts
    this->powerButtonTimer.start();
    qDebug() << "power pressed";
}

// if they let it go before 1s, timer stops (i.e. clicked not held)
void Device::PowerButtonReleased(){
    this->powerButtonTimer.stop();
    // depending on state do something (cycle through groups or whatever)
    if(this->state == State::InSession) { //And not held?
        softOff();
    }
    qDebug() << "power released";
}

// else they didnt let it go within 1s, this happens
void Device::PowerButtonHeld(){
    if(this->state == State::Off && this->batteryLevel > 0){
        this->powerOn();
    } else {
        this->powerOff();
    }
    qDebug() << "power held\n";
}

void Device::INTArrowButtonClicked(QAbstractButton* directionButton){
    QString buttonText = directionButton->objectName();
    if(this->state == State::InSession){
        if(QString::compare(buttonText,"intUpButton") == 0) {
            adjustIntensity(1);
        } else if(QString::compare(buttonText,"intDownButton") == 0) {
            adjustIntensity(-1);
        }
        qDebug() << "intensity: " << intensity;
    }
    else if(this->state == State::ChoosingSavedTherapy){
        if(QString::compare(buttonText,"intUpButton") == 0) {
            adjustIntensity(1);
        } else if(QString::compare(buttonText,"intDownButton") == 0) {
            adjustIntensity(-1);
        }
    }

}

void Device::adjustIntensity(int change) {
    int newIntensity = intensity+change;

    if(newIntensity >= 1 && newIntensity <=8) {
        intensity+= change;
        emit this->deviceUpdated();
    }
}

void Device::StartSessionButtonClicked(){
    this->activeWavelength = sessionTypes[selectedSessionType]->wavelength;
    enterTestMode();
}

void Device::ResetBattery(){
    this->SetBattery(100);
}
void Device::SetBattery(int batteryLevel){
    qDebug() << "Battery set to " << batteryLevel;
    if(batteryLevel < 0 || batteryLevel > 100)
        return;

    this->batteryLevel = 1.0*batteryLevel;
    // when battery is set, we'll replay low battery animations as needed
    this->lowBatteryTriggered = false;
    this->criticalBatteryTriggered = false;

    if(this->state == State::Paused){
        this->resumeSession();
    }
    emit this->deviceUpdated();
}

void Device::SetConnectionStatus(int status)
{
    connectionStatus = status == 0 ? ConnectionStatus::No : status == 1 ? ConnectionStatus::Okay : ConnectionStatus::Excellent;
    qDebug("connection: %d", connectionStatus);
}
void Device::SessionComplete(){
    softOff();
}

void Device::pauseSession(){
    // only works for singlshot (?)
    this->remainingSessionTime = this->sessionTimer.remainingTime();
    this->sessionTimer.stop();
    this->state = State::Paused;
    emit this->deviceUpdated();
}

void Device::resumeSession(){
    if(this->getBatteryState() == BatteryState::Critical){
        return; // session can not be resumed with low battery
    }
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
    prevWholeLevel = this->batteryLevel;

    switch(this->state){
        case State::ChoosingSession: case State::ChoosingSavedTherapy: case State::TestingConnection:
            this->batteryLevel -= 0.1;
            break;
        case State::InSession:
            this->batteryLevel -= 0.2 + 0.1*this->intensity; // assuming intensity 0 or 1-8
            break;
        case State::Paused:
            this->batteryLevel -= 0.05;
            break;
    }

    BatteryState currentBatteryState = this->getBatteryState();

    // no battery, device powers off
    if(this->batteryLevel <= 0){
        this->batteryLevel = 0;
        this->powerOff();
    }
    // battery is critical
    else if(currentBatteryState == BatteryState::Critical && !criticalBatteryTriggered){
        this->criticalBatteryTriggered = true;
        this->runBatteryAnimation = true;
        this->pauseSession();
        this->runBatteryAnimation = false;

    }
    // battery is low
    else if(currentBatteryState == BatteryState::Low && !lowBatteryTriggered){
        this->lowBatteryTriggered = true;
        this->runBatteryAnimation = true;
        emit this->deviceUpdated();
        this->runBatteryAnimation = false;
    }

    // otherwise only update the display when at least 1% is lost
    else if(prevWholeLevel - (int)this->batteryLevel >= 1){
        emit this->deviceUpdated();
    }
}

void Device::startSession()
{
    if(this->getBatteryState() == BatteryState::Critical){
        return; // session can not be started with low battery
    }

    //Test session time
    //sessionTimer.setInterval(5000);
    //sessionTimer.start();

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
        qDebug() << "connection confirmed, starting session...";
        startSession();
    } else {
        qDebug() << "No connection. Waiting for connection to start session...";
    }
}

// SLOT for Input box for recording a therapy
void Device::UsernameInputted(QString username){
    this->inputtedName = username;
    // qDebug() << "Inputted Name: " << this->getInputtedName();
    if(username.length() == 0){
        this->toggleRecord = false;
    } else {
        this->toggleRecord = true;
    }
    emit this->deviceUpdated();
}
// SLOT for Record Button clicked
void Device::RecordButtonClicked(){
    QString username = this->getInputtedName();
    qDebug() << "Record Therapy button clicked... with username: " << username;
    this->recordTherapy(username);
}
// Record Therapy Session (Save current session group, type, intensity and username to list of recorded therapies)
void Device::recordTherapy(QString username)
{
    qDebug() << "In recordTherapy()...";

    auto sessionGroup = this->sessionGroups[this->getSelectedSessionGroup()];
    auto sessionType = this->sessionTypes[this->getSelectedSessionType()];

    bool flag = true;
    for (int i = 0; i < recordedTherapies.length(); ++i){
        qDebug() << i;
        qDebug() << recordedTherapies;
        if (recordedTherapies[i]->username == username && recordedTherapies[i]->group.name == sessionGroup->name && recordedTherapies[i]->type.name == sessionType->name && recordedTherapies[i]->intensity == this->getIntensity()){
            qDebug() << "THE SAME";
            flag = false;
            break;
        }
    }
    qDebug() << flag;
    if (flag == true){
        auto new_therapy = new Therapy(*sessionGroup, *sessionType, this->getIntensity(), username);
        qDebug() << "New Therapy: " << new_therapy->group.name << new_therapy->type.name << new_therapy->intensity << new_therapy->username;
        recordedTherapies.append(new_therapy);
    }
    qDebug() << "Recorded Therapies: " << recordedTherapies;
    emit this->deviceUpdated();
}

//void Device::replayTherapy(QListWidgetItem* therapy){
//    selectedSessionGroup = therapy->group;
//    selectedSessionType = therapy->type;
//    this->intensity = therapy->intensity;
//    qDebug() << "Replaying Therapy: Session Group: " + selectedSessionGroup + " Session Type: selectedSessionType" + selectedSessionType + " Intensity: " + this->intensity;
//    //StartSessionButtonClicked();
//    startSession();
//    emit this->deviceUpdated();
//}
