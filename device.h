#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QString>
#include <QAbstractButton>
#include <QTimer>
#include <QDebug>
#include <QVector>
#include <QListWidgetItem>

#include "defs.h"

enum State {Off, ChoosingSession, ChoosingSavedTherapy, InSession, Paused, TestingConnection, SoftOff};
enum BatteryState {High, Low, Critical};
enum ConnectionStatus {No=3, Okay=2, Excellent=1}; // ints used in battery drain

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = nullptr);
    ~Device();

    // getters
    State getState() const;
    double getBatteryLevel() const;
    ConnectionStatus getConnectionStatus() const;
    int getIntensity() const;
    QString getActiveWavelength() const;
    bool getRunBatteryAnimation() const;
    int getSelectedSessionGroup() const;
    int getSelectedSessionType() const;
    int getSelectedUserSession() const;
    bool getToggleRecord() const;
    QString getInputtedName() const;
    // pseudo-getters
    int getRemainingSessionTime();
    BatteryState getBatteryState();

    QVector<Therapy *> getRecordedTherapies() const;
    QVector<SessionType*> getUserSessionTypes() const;

    int getSelectedRecordedTherapy() const;

    bool getDisconnected() const;


    bool getReturningToSafeVoltage() const;

private:
    State state;
    bool toggleRecord;

    QTimer softOffTimer;

    QTimer sessionTimer;
    int remainingSessionTime; // time left after pause, ms

    QTimer powerButtonTimer;

    QTimer testConnectionTimer;
    QTimer safeVoltageTimer;
    QTimer voltageTimer;

    QTimer batteryLevelTimer;
    double batteryLevel;
    bool lowBatteryTriggered;
    bool criticalBatteryTriggered;
    bool runBatteryAnimation;
    bool disconnected;
    bool returningToSafeVoltage;

    QString activeWavelength;
    int intensity;
    ConnectionStatus connectionStatus;

    // this is peter guessing at how this will work
    // highlighted / currently selected
    int selectedSessionGroup; // (time) 0, 1, 2
    int selectedSessionType; // (frequency) 0, 1, 2, 3
    int selectedUserSession;

    // stored data of the groups and sessions we have, set up in ctor
    QVector<SessionGroup*> sessionGroups;
    QVector<SessionType*> sessionTypes;
    QVector<UserDesignedSession*> userDesignedSessions;

    // Data Structure for recorded therapies saved by user
    int selectedRecordedTherapy;
    QVector<Therapy*> recordedTherapies;
    QString inputtedName; // Holds the text value in the username textbox

    void powerOn();
    void powerOff();
    void stopAllTimers();
    void softOff();
    void pauseSession();
    void resumeSession();
    void enterTestMode();
    void configureDevice();
    void startSession();
    void recordTherapy(QString);
    void adjustIntensity(int);
    void adjustSelectedRecordedTherapy(int);
    void replayTherapy(QListWidgetItem*);
    void userSessionWaveLength();

public slots:
    void PowerButtonPressed();
    void PowerButtonReleased();
    void CesReduction();
    void INTArrowButtonClicked(QAbstractButton*);
    void StartSessionButtonClicked();
    void SetBattery(int);
    void ResetBattery();
    void SetConnectionStatus(int);
    void UsernameInputted(QString);
    void RecordButtonClicked();
    void ReplayButtonClicked();

private slots:
    void SessionComplete(); // for session timer
    void PowerButtonHeld(); // for powerbutton timer
    void DepleteBattery(); // for battery timer
    void confirmConnection();
    void returnToSafeVoltage();

signals:
    void deviceUpdated();
    void connectionTest(bool);
    void safeVoltage(bool);
};

#endif // DEVICE_H
