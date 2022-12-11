#include "mainwindow.h"

#include "ui_mainwindow.h"

MainWindow::MainWindow(Device* d, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setupGraph();
    this->device = d;
    this->displayBatteryInfo();

    // Icons
    this->ui->intUpButton->setIcon(style.standardIcon(QStyle::SP_ArrowUp));
    this->ui->intDownButton->setIcon(style.standardIcon(QStyle::SP_ArrowDown));
    this->ui->checkMarkButton->setIcon(style.standardIcon(QStyle::SP_DialogApplyButton));
    this->ui->checkMarkButton->setIconSize(QSize(40, 40));

    // observe
    connect(d, SIGNAL(deviceUpdated()), this, SLOT(updateDisplay()));
    connect(d, SIGNAL(connectionTest(bool)), this, SLOT(updateWavelengthBlinker(bool)));
    connect(d, SIGNAL(safeVoltage(bool)), this, SLOT(setScrollGraph(bool)));

    this->scrollGraphTimer.setInterval(500);
    connect(&this->scrollGraphTimer, SIGNAL(timeout()), this, SLOT(scrollGraph()));

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

void MainWindow::setupGraph() {
    auto gWidget = this->ui->graphWidget;
    for (int i = 1; i < 9; ++i) {
        QString graphLightName = "graphLight";
        graphLightName.append(QString::number(i));
        this->graph.append(gWidget->findChild<QLabel*>(graphLightName));
    }
}

MainWindow::~MainWindow() {
    delete device;
    delete ui;
}

// update ui elements based on state of device
void MainWindow::updateDisplay() {
    this->displayBatteryInfo();

    auto state = device->getState();

    this->ui->replayTherapyButton->setEnabled(state == State::ChoosingSession && device->getRecordedTherapies().count() > 0);
    if (state == State::Off) {
        stopAllTimers();
        this->clearDisplay();
        return;
    } else if (state == State::TestingConnection) {
        auto status = this->device->getConnectionStatus();
        switch (status) {
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
    } else if (state == State::InSession) {
        this->displaySessionTime();

        if (!this->sessionTimerChecker.isActive())
            this->sessionTimerChecker.start();
        auto intensity = this->device->getIntensity();
        // if the graph is animating then don't overwrite with intensity
        if (!this->graphTimer.isActive())
            this->setGraph(intensity, intensity, false, "green");
    } else if (state == State::SoftOff) {
        auto intensity = this->device->getIntensity();
        this->setGraph(intensity, intensity, false, "green");
    } else if (state == State::ChoosingSession) {
        if (this->device->getSelectedSessionGroup() == 2) { //User designed session
            int selectedUserSession = this->device->getSelectedUserSession();
            unHighlightSessionType();
            this->setGraph(selectedUserSession + 1, selectedUserSession + 1, false, "green"); //Highlight graph
        } else {
            if (!this->graphTimer.isActive()) {
                this->setGraph(0, 0); //Reset graph when inactive
            }
        }
    } else if (state == State::ChoosingRecordedTherapy) {
        this->ui->treatmentHistoryList->setCurrentRow(device->getSelectedRecordedTherapy());
    } else if (state == State::Paused){
        if (this->device->getDisconnected() && !this->device->getReturningToSafeVoltage()){
            this->setGraph(7, 8, true, "red");
        }
    }
    auto wavelength = this->device->getActiveWavelength();
    this->setWavelength(wavelength, false, "red");

    setDeviceButtonsEnabled(state != State::Paused);
    toggleLRChannels(this->device->getConnectionStatus() != ConnectionStatus::No);
    this->ui->powerButton->setStyleSheet("border: 5px solid green;");
    displayRecordedSessions();
    highlightSession();
}

// handler to start/stop scroll animation of graph during connection lost
void MainWindow::setScrollGraph(bool isStart) {
    if (isStart){
        scrollGraphTimer.start();
    } else {
        scrollGraphTimer.stop();
    }
}

// handler to perform scroll animation of graph during connection lost
void MainWindow::scrollGraph(){
    static int currScrollNum = 1;
    static bool upDir = true;
    if (this->device->getDisconnected()){
        setGraph(currScrollNum, currScrollNum, false, "green");
        if (upDir){
            ++currScrollNum;
            if (currScrollNum == 8) {
                upDir = false;
            }
        } else {
            --currScrollNum;
            if (currScrollNum == 1) {
                upDir = true;
            }
        }
    }
}

//Stops all UI timers
void MainWindow::stopAllTimers() {
    graphTimer.stop();
    wavelengthBlinkTimer.stop();
    scrollGraphTimer.stop();
}

// sets all ui elements back to default values
void MainWindow::clearDisplay() {
    this->ui->powerButton->setStyleSheet("");

    // clear recorded therapy items
    //    this->ui->treatmentHistoryList->clearSelection();
    this->ui->treatmentHistoryList->clear();
    this->ui->usernameInput->clear();

    // turn off graph
    this->setGraph(0, 0);

    // turn off CES mode wavelengths
    this->setWavelength("none");

    // disable device buttons
    setDeviceButtonsEnabled(false);

    unHighlightSession();

    toggleLRChannels(false);
}

// turns device buttons on/off
void MainWindow::setDeviceButtonsEnabled(bool flag) {
    this->ui->checkMarkButton->setEnabled(device->getState() == State::ChoosingSession || device->getState() == State::ChoosingRecordedTherapy);
    this->ui->intUpButton->setEnabled(flag);
    this->ui->intDownButton->setEnabled(flag);

    auto state = device->getState();
    if (state == State::InSession && this->device->getSelectedSessionGroup() != 2) {
        this->ui->usernameInput->setEnabled(true);
    } else {
        this->ui->usernameInput->setEnabled(false);
    }
    this->toggleRecordButton();
}

void MainWindow::displayBatteryInfo() {
    auto newBatteryLevel = (int)device->getBatteryLevel();
    BatteryState newBatteryState = device->getBatteryState();

    this->ui->batteryDisplay->display(newBatteryLevel);
    this->ui->batteryLevelSlider->blockSignals(true);
    this->ui->batteryLevelSlider->setValue(newBatteryLevel);
    this->ui->batteryLevelSlider->blockSignals(false);

    if (device->getRunBatteryAnimation()) {
        if (newBatteryState == BatteryState::Low) {
            qDebug() << "Battery Low";
            this->setGraph(1, 2, true, "yellow");
        } else if (newBatteryState == BatteryState::Critical) {
            qDebug() << "Battery Critically Low";
            this->setGraph(1, 1, true, "red");
        }
    }
}

// turns wavelength icons on/off.
void MainWindow::setWavelength(QString wavelength, bool blink, QString colour) {
    if (wavelength == "small" || wavelength == "big") {
        QLabel *activeIcon, *inactiveIcon;
        activeIcon = wavelength == "small" ? this->ui->cesSmallWaveIcon : this->ui->cesBigWaveIcon;
        inactiveIcon = wavelength == "small" ? this->ui->cesBigWaveIcon : this->ui->cesSmallWaveIcon;

        if (blink && !this->wavelengthBlinkTimer.isActive()) {
            this->isWavelengthBlinkOn = true;
            this->wavelengthBlinkTimer.setInterval(1000);
            disconnect(&this->wavelengthBlinkTimer, &QTimer::timeout, 0, 0);
            connect(&this->wavelengthBlinkTimer, &QTimer::timeout, this, [wavelength, this]() { this->wavelengthBlink(wavelength); });
            this->wavelengthBlinkTimer.start();
        } else {
            activeIcon->setStyleSheet("color: " + colour + ";");
            inactiveIcon->setStyleSheet("color: black;");
        }
    } else if (wavelength == "both") {
        if (blink && !this->wavelengthBlinkTimer.isActive()) {
            this->isWavelengthBlinkOn = true;
            this->wavelengthBlinkTimer.setInterval(1000);
            disconnect(&this->wavelengthBlinkTimer, &QTimer::timeout, 0, 0);
            connect(&this->wavelengthBlinkTimer, &QTimer::timeout, this, [wavelength, this]() { this->wavelengthBlink(wavelength); });
            this->wavelengthBlinkTimer.start();
        } else {
            this->ui->cesSmallWaveIcon->setStyleSheet("color: " + colour + ";");
            this->ui->cesBigWaveIcon->setStyleSheet("color: " + colour + ";");
        }
    } else {
        this->ui->cesSmallWaveIcon->setStyleSheet("color: black;");
        this->ui->cesBigWaveIcon->setStyleSheet("color: black;");
    }
}

// handler to perform blink animation of wavelength icon
void MainWindow::wavelengthBlink(QString wavelength) {
    if (isWavelengthBlinkOn) {
        this->setWavelength(wavelength);
    } else {
        this->setWavelength(wavelength, false, "red");
    }

    isWavelengthBlinkOn = !isWavelengthBlinkOn;
}

// handler to start/stop blink animation of wavelength icons
void MainWindow::updateWavelengthBlinker(bool isStart) {
    if (isStart) {
        auto wavelength = this->device->getActiveWavelength();
        this->setWavelength(wavelength, true, "red");
    } else {
        this->wavelengthBlinkTimer.stop();
    }
}

// turns L/R icons on/off
void MainWindow::toggleLRChannels(bool flag) {
    if (flag) {
        ui->cesLeftIcon->setStyleSheet("color: green;");
        ui->cesRightIcon->setStyleSheet("color: green;");
    } else {
        ui->cesLeftIcon->setStyleSheet("color: black;");
        ui->cesRightIcon->setStyleSheet("color: black;");
    }
}

void MainWindow::setGraph(int start, int end, bool blink, QString colour) {
    // stop the graph timer so that it doesn't reset changes
    this->graphTimer.stop();
    // set the lights
    this->setGraphLights(start, end, colour);
    if (blink) {
        this->numGraphBlinks = 0;
        this->isGraphBlinkOn = true;
        // graph timer can be applied to more than just blinking so disconnect other slots
        disconnect(&this->graphTimer, &QTimer::timeout, 0, 0);
        // qt signal slot using lambda expression to run graphBlink with the specified values
        connect(&this->graphTimer, &QTimer::timeout, this, [start, end, colour, this]() { this->graphBlink(start, end, colour); });
        this->graphTimer.setInterval(1000);
        this->graphTimer.start();
    }
}

void MainWindow::setGraphLights(int start, int end, QString colour) {
    for (int i = 1; i < 9; ++i) {
        if (i >= start && i <= end) {
            this->graph.at(i - 1)->setStyleSheet("background-color: " + colour + ";");
        } else {
            this->graph.at(i - 1)->setStyleSheet("background-color: black;");
        }
    }
}

void MainWindow::graphBlink(int start, int end, QString colour) {
    // if on, turn off
    if (this->isGraphBlinkOn) {
        this->setGraphLights(0, 0);
    }
    // if off, turn on
    else {
        this->setGraphLights(start, end, colour);
    }

    this->isGraphBlinkOn = !this->isGraphBlinkOn;
    this->numGraphBlinks++;
    if (this->numGraphBlinks == 5) {
        this->numGraphBlinks = 0;
        // by default blink assumes the lights are on and should be turned off
        this->isGraphBlinkOn = true;
        this->graphTimer.stop();
    }
}

/*
 * Function: toggleRecordButton
 * Purpose: Function for enabling/disabling the "Record Therapy" button on the UI.
 * Input: N/A
 * Return: N/A
 */
void MainWindow::toggleRecordButton() {
    // Record Button only visible when there is valid text in the username input box and when device is in a session
    auto toggleRecord = device->getToggleRecord();
    if (device->getState() == State::InSession) {
        this->ui->recordTherapyButton->setEnabled(toggleRecord);
    } else {
        // If device not in session state then ever show the record therapy button
        this->ui->recordTherapyButton->setEnabled(false);
    }
}

/*
 * Function: displayRecordedSessions
 * Purpose: Function for updating the UI with the list of recorded therapies/treatments
 * Input: N/A
 * Return: N/A
 */
void MainWindow::displayRecordedSessions() {
    // only update treatment list when necessary
    if (device->getRecordedTherapies().length() > this->ui->treatmentHistoryList->count()) {
        ui->treatmentHistoryList->clear();

        int widgetLength = ui->treatmentHistoryList->count();
        auto list = device->getRecordedTherapies();
        for (int i = 0; i < list.length(); ++i) {
            QListWidgetItem* item = new QListWidgetItem;
            item->setText(list[i]->username + " | " + list[i]->group.name + " | " + list[i]->type.name + " | " + QString::number(list[i]->intensity));
            item->setData(Qt::UserRole, QString::number(i));
            ui->treatmentHistoryList->addItem(item);
        }
    }
}

/*
 * Function: highlightSession
 * Purpose: Function for highlighting the selected session group and session type icons on the UI.
 * Input: N/A
 * Return: N/A
 */
void MainWindow::highlightSession() {
    unHighlightSession();
    auto currSessionType = this->device->getSelectedSessionType();
    auto currSessionGroup = this->device->getSelectedSessionGroup();

    // Highlighting for selected sessiong group
    auto sessionGroupParent = this->ui->groupLayout;
    sessionGroupParent->itemAt(currSessionGroup)->widget()->setStyleSheet("background-color: green;");

    if (this->device->getSelectedSessionGroup() != 2) {
        // Highlighting for selected sessiong type
        auto sessionTypeParent = this->ui->typeLayout;
        sessionTypeParent->itemAt(currSessionType)->widget()->setStyleSheet("background-color: green;");
    } else {
        highlightUserSessionTypes(this->device->getUserSessionTypes());
    }
}

//Highlight the corresponding session types for a session group
void MainWindow::highlightUserSessionTypes(QVector<SessionType*> types) {
    //Widget containing types
    auto sessionTypeParent = this->ui->typeLayout;

    //Loop through the user designed session group's session types
    for(SessionType* t : types) {
        for(int i = 0; i < sessionTypeParent->count(); i++) {
            auto currUiType = qobject_cast<QLabel *>(sessionTypeParent->itemAt(i)->widget());

            //Highlight the UI type(s) matching the parameter
            if(QString::compare(t->name, currUiType->text()) == 0) {
                currUiType->setStyleSheet("background-color: green;");
                break;
            }
        }
    }
}

/*
 * Function: unHighlightSession
 * Purpose: Function for unhighlighting the selected session group and session type icons on the UI.
 * Input: N/A
 * Return: N/A
 */
void MainWindow::unHighlightSession() {
    unHighlightSessionGroup();
    unHighlightSessionType();
}

/*
 * Function: unHighlightSessionGroup
 * Purpose: Function for unhighlighting all the session group icons on the UI
 * Input: N/A
 * Return: N/A
 */
void MainWindow::unHighlightSessionGroup() {
    // Highlighting for selected sessiong group
    auto sessionGroupParent = this->ui->groupLayout;
    for (int i = 0; i < sessionGroupParent->count(); ++i) {
        sessionGroupParent->itemAt(i)->widget()->setStyleSheet("");
    }
}

/*
 * Function: unHighlightSessionGroup
 * Purpose: Function for unhighlighting all the session type icons on the UI
 * Input: N/A
 * Return: N/A
 */
void MainWindow::unHighlightSessionType() {
    // Highlighting for selected sessiong group
    auto sessionTypeParent = this->ui->typeLayout;
    for (int i = 0; i < sessionTypeParent->count(); ++i) {
        sessionTypeParent->itemAt(i)->widget()->setStyleSheet("");
    }
}

void MainWindow::displaySessionTime() {
    int remainingTime = this->device->getRemainingSessionTime();
    qDebug() << "Session time remaining: " << remainingTime;
    if (remainingTime <= 0) {
        this->sessionTimerChecker.stop();
        this->ui->therapyTime->display(0);
        return;
    }
    this->ui->therapyTime->display(remainingTime / 1000);
}
