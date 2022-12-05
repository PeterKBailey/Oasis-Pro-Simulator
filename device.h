#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QString>
#include <QAbstractButton>
#include <QTimer>
#include <QDebug>
#include <QVector>

#include "defs.h"

enum State {Off, ChoosingSession, ChoosingSavedTherapy, InSession, Paused, TestingConnection};
enum BatteryState { High, Low, CriticallyLow };
enum ConnectionStatus {No, Okay, Excellent};

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(QObject *parent = nullptr);
    ~Device();

    // getters
    State getState() const;
    BatteryState getBatteryState() const;
    double getBatteryLevel() const;
    ConnectionStatus getConnectionStatus() const;

    int getIntensity() const;

    QString getActiveWavelength() const;

private:
    State state;
    BatteryState batteryState;

    QTimer sessionTimer;
    int remainingSessionTime; // time left after pause, ms

    QTimer powerButtonTimer;

    QTimer batteryLevelTimer;
    double batteryLevel;

    QString activeWavelength;
    int intensity;
    ConnectionStatus connectionStatus;

    // this is peter guessing at how this will work
    // highlighted / currently selected
    int selectedSessionGroup; // (time) 0, 1, 2
    int selectedSessionType; // (frequency) 0, 1, 2, 3

    // stored data of the groups and sessions we have, set up in ctor
    QVector<SessionGroup*> sessionGroups;
    QVector<SessionType*> sessionTypes;

    void powerOn();
    void powerOff(bool);
    void pauseSession();
    void resumeSession();
    void enterTestMode();
    void configureDevice();
    void startSession();
    void adjustIntensity(int);

public slots:
    void PowerButtonPressed();
    void PowerButtonReleased();
    void INTArrowButtonClicked(QAbstractButton*);
    void StartSessionButtonClicked();
    void ResetBattery();
    void SetConnectionStatus(int);

private slots:
    void SessionComplete(); // for session timer
    void PowerButtonHeld(); // for powerbutton timer
    void DepleteBattery(); // for battery timer
    void confirmConnection();

signals:
    void deviceUpdated();
};

#endif // DEVICE_H
